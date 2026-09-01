/** @file
  Definitions for TPM 2.0 startup and initialization.

  A single library instance consolidates the TPM startup and pre-DXE
  measurement work that previously lived in PEI so it can be driven
  from either PEI or from SEC on PEI-less platforms.

Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
Copyright (c), Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Ppi/FirmwareVolumeInfoMeasurementExcluded.h>

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Guid/TcgEventHob.h>
#include <Guid/TpmInstance.h>
#include <Guid/MigratedFvInfo.h>
#include <Guid/ExcludedFvHob.h>
#include <Guid/MeasuredFvHob.h>
#include <Guid/PrehashedFvHob.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DeviceStateLib.h>
#include <Library/HashLib.h>
#include <Library/HobLib.h>
#include <Library/OemTpm2InitLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/Tcg2PreUefiEventLogLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/Tpm2HelpLib.h>
#include <Library/Tpm2StartupLib.h>

#define FV_HANDOFF_TABLE_DESC  "Fv(XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)"

// Flag for InternalHashLogExtend. When set, HashData points to a
// caller-supplied TPML_DIGEST_VALUES and the PCR is extended with those
// digests; otherwise HashData is hashed and extended via HashLib.
#define TPM2_STARTUP_FLAG_PRE_HASH  BIT0

#pragma pack (1)

// FV_HANDOFF_TABLE_POINTERS2 per TCG PC Client PFP 10.2.5.
typedef struct {
  UINT8                   BlobDescriptionSize;
  UINT8                   BlobDescription[sizeof (FV_HANDOFF_TABLE_DESC)];
  EFI_PHYSICAL_ADDRESS    BlobBase;
  UINT64                  BlobLength;
} FV_HANDOFF_TABLE_POINTERS2;

#pragma pack ()

// Private HOB GUID used to persist one EFI_PLATFORM_FIRMWARE_BLOB per FV
// that has been measured (or recorded as covered by a parent's
// measurement) in this phase. Tpm2StartupPublishMeasuredFvHob consolidates
// the entries into a single gMeasuredFvHobGuid HOB for downstream consumers.
STATIC CONST EFI_GUID  mTpm2StartupMeasuredFvHobGuid = {
  0x8dbc7a5e, 0x2b31, 0x4a4f, { 0x9c, 0x0a, 0x36, 0x7f, 0xa4, 0x1b, 0x7d, 0xe5 }
};

// Table used by LogHashEvent to iterate over supported log formats.
typedef struct {
  EFI_GUID                     *EventGuid;
  EFI_TCG2_EVENT_LOG_FORMAT    LogFormat;
} TCG2_EVENT_INFO_STRUCT;

STATIC CONST TCG2_EVENT_INFO_STRUCT  mTcg2EventInfo[] = {
  { &gTcgEventEntryHobGuid,  EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2 },
  { &gTcgEvent2EntryHobGuid, EFI_TCG2_EVENT_LOG_FORMAT_TCG_2   },
};

/**
  Produce gTpmErrorHobGuid (if not already present) and report an interface
  error status code. Downstream consumers use presence of the HOB to skip
  further TPM interaction.
**/
STATIC
VOID
ReportTpmErrorHob (
  VOID
  )
{
  if (GetFirstGuidHob (&gTpmErrorHobGuid) == NULL) {
    BuildGuidHob (&gTpmErrorHobGuid, 0);
  }

  REPORT_STATUS_CODE (
    EFI_ERROR_CODE | EFI_ERROR_MINOR,
    (PcdGet32 (PcdStatusCodeSubClassTpmDevice) | EFI_P_EC_INTERFACE_ERROR)
    );
}

/**
  Common checks that ensure the platform enables valid TPM instance, no prior
  TPM error was reported, and the device is successfully requested. On failure,
  produces an ERROR HOB so later phases exit early.

  @retval EFI_SUCCESS       Valid TPM instance, no prior error, and the device
                            was acquired.
  @retval EFI_UNSUPPORTED   Invalid TPM instance.
  @retval EFI_DEVICE_ERROR  Prior TPM error detected or device error.
  @retval EFI_NOT_FOUND     TPM2 not found.
**/
STATIC
EFI_STATUS
Tpm2StartupCheckPrereqs (
  VOID
  )
{
  EFI_STATUS  Status;

  if (CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceNoneGuid) ||
      CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm12Guid))
  {
    DEBUG ((DEBUG_INFO, "%a - No TPM2 instance required.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    DEBUG ((DEBUG_ERROR, "%a - gTpmErrorHobGuid present.\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Tpm2RequestUseTpm failed: %r\n", __func__, Status));
    ReportTpmErrorHob ();
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Walk gExcludedFvHobGuid HOB list for a matching FV.

  @param[in]  FvBase    Base address of the FV to check.
  @param[in]  FvLength  Length of the FV to check.

  @retval TRUE   The FV is present and must not be measured.
  @retval FALSE  The FV is not present and may be measured.
**/
STATIC
BOOLEAN
IsFvMeasurementExcluded (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  EXCLUDED_HOB_DATA  *ExcludedHobData;
  UINT32             Index;

  GuidHob = GetFirstGuidHob (&gExcludedFvHobGuid);
  while (GuidHob != NULL) {
    ExcludedHobData = GET_GUID_HOB_DATA (GuidHob);
    for (Index = 0; Index < ExcludedHobData->Num; Index++) {
      if ((ExcludedHobData->ExcludedFvs[Index].FvBase == FvBase) &&
          (ExcludedHobData->ExcludedFvs[Index].FvLength == FvLength))
      {
        return TRUE;
      }
    }

    GuidHob = GetNextGuidHob (&gExcludedFvHobGuid, GET_NEXT_HOB (GuidHob));
  }

  return FALSE;
}

/**
  Walk gPrehashedFvHobGuid HOBs for a matching FV; populate DigestList with
  digests whose algorithms intersect the mask provided. The mask bits are
  cleared when found. When the mask reaches 0 the caller can skip hashing.

  @param[in]      FvBase                 Base address of the FV to look up.
  @param[in]      FvLength               Length of the FV to look up.
  @param[out]     DigestList             Populated with matching digests.
  @param[in,out]  RemainingTpm2HashMask  Hash to acquire, cleared when found.

  @retval EFI_SUCCESS    Found a matching FV and hash algorithm.
  @retval EFI_NOT_FOUND  No pre-hashed HOB found.
**/
STATIC
EFI_STATUS
GetPrehashedFvDigests (
  IN     EFI_PHYSICAL_ADDRESS  FvBase,
  IN     UINT64                FvLength,
  OUT    TPML_DIGEST_VALUES    *DigestList,
  IN OUT UINT32                *RemainingTpm2HashMask
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  PREHASHED_FV_HOB   *FvHob;
  HASH_INFO          *PreHashInfo;
  UINT8              *HashInfoBuffer;
  UINTN              HashInfoBufferSize;
  UINTN              HobDataSize;
  UINT32             HashAlgoMask;
  UINT32             RemainingMask;
  UINT32             Index;
  UINT32             DigestCount;
  UINT16             HashAlgoId;
  UINT16             HashSize;

  GuidHob = GetFirstGuidHob (&gPrehashedFvHobGuid);
  while (GuidHob != NULL) {
    FvHob = GET_GUID_HOB_DATA (GuidHob);
    // Only continue if the current FV matches the FV being requested.
    if ((FvHob->FvBase == FvBase) && (FvHob->FvLength == FvLength)) {
      // Verify the HobDataSize matches what is expected.
      HobDataSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
      if (HobDataSize < sizeof (PREHASHED_FV_HOB)) {
        return EFI_NOT_FOUND;
      }

      HashInfoBuffer     = (UINT8 *)(FvHob + 1);
      HashInfoBufferSize = HobDataSize - sizeof (PREHASHED_FV_HOB);
      RemainingMask      = *RemainingTpm2HashMask;
      DigestCount        = 0;
      for (Index = 0; Index < FvHob->Count; Index++) {
        // Verify that HashInfoBufferSize contains at least HASH_INFO.
        if (HashInfoBufferSize < sizeof (HASH_INFO)) {
          return EFI_NOT_FOUND;
        }

        PreHashInfo = (HASH_INFO *)HashInfoBuffer;
        HashAlgoId  = ReadUnaligned16 (&PreHashInfo->HashAlgoId);
        HashSize    = ReadUnaligned16 (&PreHashInfo->HashSize);
        // Validate HashSize fits within the digest and that HashInfoBufferSize
        // is sufficiently large enough to contain HashSize.
        if ((HashSize > sizeof (DigestList->digests[0].digest)) ||
            (HashInfoBufferSize - sizeof (HASH_INFO) < HashSize))
        {
          return EFI_NOT_FOUND;
        }

        // Only copy data from hash algorithms requested via RemainingTpm2HashMask.
        HashAlgoMask = GetHashMaskFromAlgo (HashAlgoId);
        if ((RemainingMask & HashAlgoMask) != 0) {
          if (DigestCount >= HASH_COUNT) {
            return EFI_NOT_FOUND;
          }

          WriteUnaligned16 (&(DigestList->digests[DigestCount].hashAlg), HashAlgoId);
          CopyMem (&DigestList->digests[DigestCount].digest, PreHashInfo + 1, HashSize);
          DigestCount++;
          RemainingMask &= ~HashAlgoMask;
        }

        HashInfoBuffer     += sizeof (HASH_INFO) + HashSize;
        HashInfoBufferSize -= sizeof (HASH_INFO) + HashSize;
      }

      WriteUnaligned32 (&DigestList->count, DigestCount);
      *RemainingTpm2HashMask = RemainingMask;
      return EFI_SUCCESS;
    }

    GuidHob = GetNextGuidHob (&gPrehashedFvHobGuid, GET_NEXT_HOB (GuidHob));
  }

  return EFI_NOT_FOUND;
}

/**
  Emit event-log HOBs (TCG 1.2 and TCG 2.0 formats) for a completed extend.
  Both formats are always emitted; downstream consumers pick the one they
  need.

  @param[in]      DigestList    Event digest list.
  @param[in,out]  NewEventHdr   Event header.
  @param[in]      NewEventData  Event data.

  @retval EFI_SUCCESS           HOBs were built successfully.
  @retval EFI_OUT_OF_RESOURCES  A HOB allocation failed.
  @retval EFI_DEVICE_ERROR      Device communication failed.
**/
STATIC
EFI_STATUS
LogHashEvent (
  IN     TPML_DIGEST_VALUES  *DigestList,
  IN OUT TCG_PCR_EVENT_HDR   *NewEventHdr,
  IN     UINT8               *NewEventData
  )
{
  VOID            *HobData;
  EFI_STATUS      Status;
  UINTN           Index;
  EFI_STATUS      RetStatus;
  TCG_PCR_EVENT2  *TcgPcrEvent2;
  UINT8           *DigestBuffer;
  UINT32          HashAlgorithmBitmap;
  UINT32          ActivePcrBanks;

  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&HashAlgorithmBitmap, &ActivePcrBanks);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  RetStatus = EFI_SUCCESS;

  for (Index = 0; Index < ARRAY_SIZE (mTcg2EventInfo); Index++) {
    switch (mTcg2EventInfo[Index].LogFormat) {
      case EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2:
        Status = GetDigestFromDigestList (TPM_ALG_SHA1, DigestList, &NewEventHdr->Digest);
        if (!EFI_ERROR (Status)) {
          HobData = BuildGuidHob (
                      &gTcgEventEntryHobGuid,
                      sizeof (*NewEventHdr) + NewEventHdr->EventSize
                      );
          if (HobData == NULL) {
            RetStatus = EFI_OUT_OF_RESOURCES;
            break;
          }

          CopyMem (HobData, NewEventHdr, sizeof (*NewEventHdr));
          HobData = (VOID *)((UINT8 *)HobData + sizeof (*NewEventHdr));
          CopyMem (HobData, NewEventData, NewEventHdr->EventSize);
        }

        break;

      case EFI_TCG2_EVENT_LOG_FORMAT_TCG_2:
        HobData = BuildGuidHob (
                    &gTcgEvent2EntryHobGuid,
                    sizeof (TcgPcrEvent2->PCRIndex) + sizeof (TcgPcrEvent2->EventType) +
                    GetDigestListSize (DigestList) +
                    sizeof (TcgPcrEvent2->EventSize) + NewEventHdr->EventSize
                    );
        if (HobData == NULL) {
          RetStatus = EFI_OUT_OF_RESOURCES;
          break;
        }

        TcgPcrEvent2            = HobData;
        TcgPcrEvent2->PCRIndex  = NewEventHdr->PCRIndex;
        TcgPcrEvent2->EventType = NewEventHdr->EventType;
        DigestBuffer            = (UINT8 *)&TcgPcrEvent2->Digest;
        DigestBuffer            = CopyDigestListToBuffer (DigestBuffer, DigestList, ActivePcrBanks);
        CopyMem (DigestBuffer, &NewEventHdr->EventSize, sizeof (TcgPcrEvent2->EventSize));
        DigestBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);
        CopyMem (DigestBuffer, NewEventData, NewEventHdr->EventSize);
        break;
    }
  }

  return RetStatus;
}

/**
  Hash (or accept a pre-hashed digest for) a buffer, extend the target PCR,
  and emit the corresponding event-log HOBs. When TPM2_STARTUP_FLAG_PRE_HASH
  is set, HashData points to a caller-supplied TPML_DIGEST_VALUES and the PCR
  is extended with those digests. Otherwise HashData is hashed and extended
  via the HashLib. On failure, produces an ERROR HOB so later phases exit
  early.

  @param[in]  Flags         Combination of TPM2_STARTUP_FLAG_* bits.
  @param[in]  HashData      Buffer to hash, or a TPML_DIGEST_VALUES when
                            TPM2_STARTUP_FLAG_PRE_HASH is set.
  @param[in]  HashDataLen   Length of HashData in bytes (ignored on the
                            pre-hash path).
  @param[in]  NewEventHdr   Event header for logging (PCR, type, size).
  @param[in]  NewEventData  Event data buffer for logging.

  @retval EFI_SUCCESS           PCR extended and event logged.
  @retval EFI_DEVICE_ERROR      Prior TPM error detected or device error.
  @retval EFI_OUT_OF_RESOURCES  A HOB allocation failed.
**/
STATIC
EFI_STATUS
InternalHashLogExtend (
  IN     UINT64             Flags,
  IN     UINT8              *HashData,
  IN     UINTN              HashDataLen,
  IN     TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN     UINT8              *NewEventData
  )
{
  EFI_STATUS          Status;
  TPML_DIGEST_VALUES  DigestList;

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    return EFI_DEVICE_ERROR;
  }

  if ((Flags & TPM2_STARTUP_FLAG_PRE_HASH) != 0) {
    ZeroMem (&DigestList, sizeof (DigestList));
    CopyMem (&DigestList, HashData, sizeof (DigestList));
    Status = Tpm2PcrExtend (NewEventHdr->PCRIndex, &DigestList);
  } else {
    Status = HashAndExtend (
               NewEventHdr->PCRIndex,
               HashData,
               HashDataLen,
               &DigestList
               );
  }

  if (!EFI_ERROR (Status)) {
    Status = LogHashEvent (&DigestList, NewEventHdr, NewEventData);
  }

  if (Status == EFI_DEVICE_ERROR) {
    DEBUG ((DEBUG_ERROR, "%a - %r. Disable TPM.\n", __func__, Status));
    ReportTpmErrorHob ();
  }

  return Status;
}

/**
  Measure PcdFirmwareVersionString into PCR 0 as an EV_S_CRTM_VERSION event.

  @retval EFI_SUCCESS           Measurement extended and logged.
  @retval EFI_DEVICE_ERROR      Prior TPM error detected or device error.
  @retval EFI_OUT_OF_RESOURCES  A HOB allocation failed.
**/
STATIC
EFI_STATUS
MeasureCRTMVersion (
  VOID
  )
{
  TCG_PCR_EVENT_HDR  TcgEventHdr;

  TcgEventHdr.PCRIndex  = 0;
  TcgEventHdr.EventType = EV_S_CRTM_VERSION;
  TcgEventHdr.EventSize = (UINT32)StrSize ((CHAR16 *)PcdGetPtr (PcdFirmwareVersionString));

  return InternalHashLogExtend (
           0,
           (UINT8 *)PcdGetPtr (PcdFirmwareVersionString),
           TcgEventHdr.EventSize,
           &TcgEventHdr,
           (UINT8 *)PcdGetPtr (PcdFirmwareVersionString)
           );
}

/**
  Measure the FIRMWARE_DEBUGGER_EVENT_STRING marker into PCR 7 as an
  EV_EFI_ACTION event.

  @retval EFI_SUCCESS           Measurement extended and logged.
  @retval EFI_DEVICE_ERROR      Prior TPM error detected or device error.
  @retval EFI_OUT_OF_RESOURCES  A HOB allocation failed.
**/
STATIC
EFI_STATUS
MeasureFirmwareDebuggerEnabled (
  VOID
  )
{
  TCG_PCR_EVENT_HDR  TcgEventHdr;

  TcgEventHdr.PCRIndex  = 7;
  TcgEventHdr.EventType = EV_EFI_ACTION;
  TcgEventHdr.EventSize = sizeof (FIRMWARE_DEBUGGER_EVENT_STRING) - 1;

  DEBUG ((DEBUG_INFO, "Measuring Device State: Firmware Debugger Enabled\n"));
  return InternalHashLogExtend (
           0,
           (UINT8 *)FIRMWARE_DEBUGGER_EVENT_STRING,
           sizeof (FIRMWARE_DEBUGGER_EVENT_STRING) - 1,
           &TcgEventHdr,
           (UINT8 *)FIRMWARE_DEBUGGER_EVENT_STRING
           );
}

/**
  Emit an EV_SEPARATOR event on the specified PCR with the error data
  value defined by TCG PC Client PFP (0x00000001). Used after S3 startup
  failure to record that the pre-boot state is untrustworthy.

  @param[in]  PCRIndex  PCR to extend with the separator event.

  @retval EFI_SUCCESS           Separator extended and logged.
  @retval EFI_DEVICE_ERROR      Prior TPM error detected or device error.
  @retval EFI_OUT_OF_RESOURCES  A HOB allocation failed.
**/
STATIC
EFI_STATUS
MeasureSeparatorEventWithError (
  IN TPM_PCRINDEX  PCRIndex
  )
{
  TCG_PCR_EVENT_HDR  TcgEvent;
  UINT32             EventData;

  EventData          = 0x1;
  TcgEvent.PCRIndex  = PCRIndex;
  TcgEvent.EventType = EV_SEPARATOR;
  TcgEvent.EventSize = (UINT32)sizeof (EventData);
  return InternalHashLogExtend (0, (UINT8 *)&EventData, TcgEvent.EventSize, &TcgEvent, (UINT8 *)&EventData);
}

/**
  Locate the FV name GUID (from the extended header) inside an FV image,
  with bounds checks so a malformed or truncated FV cannot cause an
  out-of-bounds read.

  @param[in]  FvBase    Base address of the FV image.
  @param[in]  FvLength  Length of the FV image.

  @return Pointer to the FvName GUID inside the FV extended header, or
          NULL when the FV has no extended header or fails bounds checks.
**/
STATIC
VOID *
GetFvName (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  EFI_FIRMWARE_VOLUME_HEADER      *FvHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FvExtHeader;

  if ((FvBase >= MAX_ADDRESS) ||
      (FvLength >= MAX_ADDRESS - FvBase) ||
      (FvLength < sizeof (EFI_FIRMWARE_VOLUME_HEADER)))
  {
    return NULL;
  }

  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvBase;
  if (FvHeader->ExtHeaderOffset < sizeof (EFI_FIRMWARE_VOLUME_HEADER)) {
    return NULL;
  }

  if (FvHeader->ExtHeaderOffset + sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER) > FvLength) {
    return NULL;
  }

  FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)(UINTN)(FvBase + FvHeader->ExtHeaderOffset);
  return &FvExtHeader->FvName;
}

/**
  If FvBase/FvLength matches a migrated FV entry in gEdkiiMigratedFvInfoGuid,
  return the original pre-migration base (for event-log correctness) and
  the current in-memory data base (for hashing). Otherwise both output
  addresses fall back to FvBase.

  @param[in]   FvBase      Current base address the caller sees.
  @param[in]   FvLength    Length of the FV.
  @param[out]  FvOrgBase   Original pre-migration base, or FvBase when the
                           FV was not migrated.
  @param[out]  FvDataBase  Current in-memory data base to hash from, or
                           FvBase when the FV was not migrated.
**/
STATIC
VOID
ResolveMigratedFvBases (
  IN  EFI_PHYSICAL_ADDRESS  FvBase,
  IN  UINT64                FvLength,
  OUT EFI_PHYSICAL_ADDRESS  *FvOrgBase,
  OUT EFI_PHYSICAL_ADDRESS  *FvDataBase
  )
{
  EFI_PEI_HOB_POINTERS    Hob;
  EDKII_MIGRATED_FV_INFO  *MigratedFvInfo;

  *FvOrgBase  = FvBase;
  *FvDataBase = FvBase;

  Hob.Raw = GetFirstGuidHob (&gEdkiiMigratedFvInfoGuid);
  while (Hob.Raw != NULL) {
    MigratedFvInfo = GET_GUID_HOB_DATA (Hob);
    if ((MigratedFvInfo->FvNewBase == (UINT32)FvBase) &&
        (MigratedFvInfo->FvLength == (UINT32)FvLength))
    {
      *FvOrgBase  = (EFI_PHYSICAL_ADDRESS)(UINTN)MigratedFvInfo->FvOrgBase;
      *FvDataBase = (EFI_PHYSICAL_ADDRESS)(UINTN)MigratedFvInfo->FvDataBase;
      return;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextGuidHob (&gEdkiiMigratedFvInfoGuid, Hob.Raw);
  }
}

/**
  Walk the private measured-FV HOB store to determine whether an entry for
  the given FV already exists.

  @param[in]  FvBase    Base address of the FV.
  @param[in]  FvLength  Length of the FV.

  @retval TRUE   A matching entry is already present.
  @retval FALSE  No matching entry; the FV has not been measured or
                 recorded in this phase.
**/
STATIC
BOOLEAN
IsFvAlreadyRecorded (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  EFI_HOB_GUID_TYPE           *GuidHob;
  EFI_PLATFORM_FIRMWARE_BLOB  *Blob;

  GuidHob = GetFirstGuidHob (&mTpm2StartupMeasuredFvHobGuid);
  while (GuidHob != NULL) {
    Blob = GET_GUID_HOB_DATA (GuidHob);
    if ((Blob->BlobBase == FvBase) && (Blob->BlobLength == FvLength)) {
      return TRUE;
    }

    GuidHob = GetNextGuidHob (&mTpm2StartupMeasuredFvHobGuid, GET_NEXT_HOB (GuidHob));
  }

  return FALSE;
}

/**
  Record an FV in the private measured-FV HOB store.

  @param[in]  FvBase    Base address of the FV.
  @param[in]  FvLength  Length of the FV.

  @retval EFI_SUCCESS           Entry was already present, or a new HOB
                                was appended.
  @retval EFI_OUT_OF_RESOURCES  BuildGuidHob failed.
**/
STATIC
EFI_STATUS
RecordMeasuredFv (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  EFI_PLATFORM_FIRMWARE_BLOB  *Blob;

  if (IsFvAlreadyRecorded (FvBase, FvLength)) {
    return EFI_SUCCESS;
  }

  Blob = BuildGuidHob (&mTpm2StartupMeasuredFvHobGuid, sizeof (EFI_PLATFORM_FIRMWARE_BLOB));
  if (Blob == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Blob->BlobBase   = FvBase;
  Blob->BlobLength = FvLength;
  return EFI_SUCCESS;
}

/**
  See Tpm2StartupLib.h. Public API.
**/
EFI_STATUS
EFIAPI
Tpm2StartupMeasureFvImage (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  EFI_STATUS                  Status;
  EFI_PLATFORM_FIRMWARE_BLOB  FvBlob;
  FV_HANDOFF_TABLE_POINTERS2  FvBlob2;
  VOID                        *EventData;
  VOID                        *FvName;
  TCG_PCR_EVENT_HDR           TcgEventHdr;
  TPML_DIGEST_VALUES          DigestList;
  EFI_PHYSICAL_ADDRESS        FvOrgBase;
  EFI_PHYSICAL_ADDRESS        FvDataBase;
  UINT32                      HashAlgorithmBitmap;
  UINT32                      ActivePcrBanks;

  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&HashAlgorithmBitmap, &ActivePcrBanks);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (IsFvMeasurementExcluded (FvBase, FvLength)) {
    DEBUG ((DEBUG_INFO, "FV excluded from measurement: base=0x%lx len=0x%lx\n", FvBase, FvLength));
    return EFI_SUCCESS;
  }

  if (IsFvAlreadyRecorded (FvBase, FvLength)) {
    DEBUG ((DEBUG_INFO, "FV already measured: base=0x%lx len=0x%lx\n", FvBase, FvLength));
    return EFI_SUCCESS;
  }

  ZeroMem (&DigestList, sizeof (DigestList));
  (VOID)GetPrehashedFvDigests (FvBase, FvLength, &DigestList, &ActivePcrBanks);

  ResolveMigratedFvBases (FvBase, FvLength, &FvOrgBase, &FvDataBase);

  if (PcdGet32 (PcdTcgPfpMeasurementRevision) >= TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_105) {
    FvBlob2.BlobDescriptionSize = sizeof (FvBlob2.BlobDescription);
    CopyMem (FvBlob2.BlobDescription, FV_HANDOFF_TABLE_DESC, sizeof (FvBlob2.BlobDescription));
    FvName = GetFvName (FvBase, FvLength);
    if (FvName != NULL) {
      AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "Fv(%g)", FvName);
    }

    FvBlob2.BlobBase      = FvOrgBase;
    FvBlob2.BlobLength    = FvLength;
    TcgEventHdr.PCRIndex  = 0;
    TcgEventHdr.EventType = EV_EFI_PLATFORM_FIRMWARE_BLOB2;
    TcgEventHdr.EventSize = sizeof (FvBlob2);
    EventData             = &FvBlob2;
  } else {
    FvBlob.BlobBase       = FvOrgBase;
    FvBlob.BlobLength     = FvLength;
    TcgEventHdr.PCRIndex  = 0;
    TcgEventHdr.EventType = EV_EFI_PLATFORM_FIRMWARE_BLOB;
    TcgEventHdr.EventSize = sizeof (FvBlob);
    EventData             = &FvBlob;
  }

  if (ActivePcrBanks == 0) {
    // Pre-hashed digests satisfy the full TPM hash mask; skip hashing.
    Status = InternalHashLogExtend (
               TPM2_STARTUP_FLAG_PRE_HASH,
               (UINT8 *)&DigestList,
               (UINTN)sizeof (DigestList),
               &TcgEventHdr,
               EventData
               );
    DEBUG ((DEBUG_INFO, "Pre-hashed FV extended & logged: base=0x%lx len=0x%lx\n", FvBase, FvLength));
  } else {
    Status = InternalHashLogExtend (
               0,
               (UINT8 *)(UINTN)FvDataBase,
               (UINTN)FvLength,
               &TcgEventHdr,
               EventData
               );
    DEBUG ((DEBUG_INFO, "FV hashed & measured: base=0x%lx len=0x%lx\n", FvBase, FvLength));
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FV failed measurement: base=0x%lx status=%r\n", FvBase, Status));
    return Status;
  }

  return RecordMeasuredFv (FvBase, FvLength);
}

/**
  See Tpm2StartupLib.h. Public API.
**/
VOID
EFIAPI
Tpm2StartupRecordChildFv (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  (VOID)RecordMeasuredFv (FvBase, FvLength);
}

/**
  Run the pre-Tcg2 measurement sequence: pre-UEFI event log seeding,
  firmware debugger state (when source-debug is enabled), and CRTM
  version (when PcdTpm2ScrtmPolicy selects it). Callers measure their
  FVs separately via Tpm2StartupMeasureFvImage.

  @retval EFI_SUCCESS  All enabled measurements were extended and logged.
  @retval other        Propagated from the first failing measurement.
**/
STATIC
EFI_STATUS
RunMeasurementPhase (
  VOID
  )
{
  EFI_STATUS    Status;
  DEVICE_STATE  CurrentDeviceState;

  CreateTcg2PreUefiEventLogEntries ();

  CurrentDeviceState = GetDeviceState ();
  if ((CurrentDeviceState & DEVICE_STATE_SOURCE_DEBUG_ENABLED) != 0) {
    Status = MeasureFirmwareDebuggerEnabled ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to measure Firmware Debugger Enabled: %r\n", __func__, Status));
      return Status;
    }
  }

  if (PcdGet8 (PcdTpm2ScrtmPolicy) == 1) {
    Status = MeasureCRTMVersion ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - MeasureCRTMVersion failed: %r\n", __func__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  See Tpm2StartupLib.h. Public API.
**/
EFI_STATUS
EFIAPI
Tpm2StartupInitializeTpm (
  IN BOOLEAN  IsS3Resume
  )
{
  EFI_STATUS     Status;
  BOOLEAN        S3ErrorReport;
  TPM_PCRINDEX   PcrIndex;
  EFI_BOOT_MODE  OemBootMode;

  DEBUG ((DEBUG_INFO, "%a - Entry (IsS3Resume=%d)\n", __func__, IsS3Resume));

  Status = Tpm2StartupCheckPrereqs ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OemBootMode = IsS3Resume ? BOOT_ON_S3_RESUME : BOOT_WITH_FULL_CONFIGURATION;

  Status = OemTpm2InitPeiPreStartup (OemBootMode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OemTpm2InitPeiPreStartup returned %r. Aborting init.\n", Status));
    goto ErrorHob;
  }

  S3ErrorReport = FALSE;
  if (PcdGet8 (PcdTpm2InitializationPolicy) == 1) {
    if (IsS3Resume) {
      Status = Tpm2Startup (TPM_SU_STATE);
      if (EFI_ERROR (Status)) {
        Status = Tpm2Startup (TPM_SU_CLEAR);
        if (!EFI_ERROR (Status)) {
          S3ErrorReport = TRUE;
        }
      }
    } else {
      Status = Tpm2Startup (TPM_SU_CLEAR);
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Tpm2Startup failed: %r\n", __func__, Status));
      ASSERT_EFI_ERROR (Status);
      goto ErrorHob;
    }
  }

  if (S3ErrorReport) {
    for (PcrIndex = 0; PcrIndex < 8; PcrIndex++) {
      Status = MeasureSeparatorEventWithError (PcrIndex);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Separator Event with Error not measured on PCR %d\n", PcrIndex));
      }
    }
  }

  if (!IsS3Resume && (PcdGet8 (PcdTpm2SelfTestPolicy) == 1)) {
    Status = Tpm2SelfTest (NO);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Tpm2SelfTest failed: %r\n", __func__, Status));
      goto ErrorHob;
    }
  }

  Status = OemTpm2InitPeiPostSelfTest (OemBootMode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OemTpm2InitPeiPostSelfTest returned %r. Aborting init.\n", Status));
    goto ErrorHob;
  }

  DEBUG_CODE_BEGIN ();
  Tpm2PcrReadForActiveBank (00, NULL);
  DEBUG_CODE_END ();

  DEBUG ((DEBUG_INFO, "%a - Exit (Success)\n", __func__));
  return EFI_SUCCESS;

ErrorHob:
  ReportTpmErrorHob ();
  DEBUG ((DEBUG_INFO, "%a - Exit (Status=%r)\n", __func__, Status));
  return Status;
}

/**
  See Tpm2StartupLib.h. Public API.
**/
EFI_STATUS
EFIAPI
Tpm2StartupMeasureCoreEvents (
  VOID
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - Entry\n", __func__));

  Status = Tpm2StartupCheckPrereqs ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = OemTpm2InitPeiPreMeasurements ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OemTpm2InitPeiPreMeasurements returned %r. Aborting measurement.\n", Status));
    goto ErrorHob;
  }

  Status = RunMeasurementPhase ();
  if (EFI_ERROR (Status)) {
    goto ErrorHob;
  }

  DEBUG ((DEBUG_INFO, "%a - Exit (Success)\n", __func__));
  return EFI_SUCCESS;

ErrorHob:
  ReportTpmErrorHob ();
  DEBUG ((DEBUG_INFO, "%a - Exit (Status=%r)\n", __func__, Status));
  return Status;
}

/**
  See Tpm2StartupLib.h. Public API.
**/
VOID
EFIAPI
Tpm2StartupPublishMeasuredFvHob (
  VOID
  )
{
  MEASURED_HOB_DATA           *MeasuredHobData;
  EFI_HOB_GUID_TYPE           *GuidHob;
  EFI_PLATFORM_FIRMWARE_BLOB  *Blob;
  UINT32                      Count;
  UINT32                      Index;

  if (GetFirstGuidHob (&gMeasuredFvHobGuid) != NULL) {
    return;
  }

  Count   = 0;
  GuidHob = GetFirstGuidHob (&mTpm2StartupMeasuredFvHobGuid);
  while (GuidHob != NULL) {
    Count++;
    GuidHob = GetNextGuidHob (&mTpm2StartupMeasuredFvHobGuid, GET_NEXT_HOB (GuidHob));
  }

  MeasuredHobData = BuildGuidHob (
                      &gMeasuredFvHobGuid,
                      OFFSET_OF (MEASURED_HOB_DATA, MeasuredFvBuf) + sizeof (EFI_PLATFORM_FIRMWARE_BLOB) * Count
                      );
  if (MeasuredHobData == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to allocate MeasuredFvHob\n", __func__));
    return;
  }

  MeasuredHobData->Num = Count;

  Index   = 0;
  GuidHob = GetFirstGuidHob (&mTpm2StartupMeasuredFvHobGuid);
  while (GuidHob != NULL) {
    Blob                                             = GET_GUID_HOB_DATA (GuidHob);
    MeasuredHobData->MeasuredFvBuf[Index].BlobBase   = Blob->BlobBase;
    MeasuredHobData->MeasuredFvBuf[Index].BlobLength = Blob->BlobLength;
    Index++;
    GuidHob = GetNextGuidHob (&mTpm2StartupMeasuredFvHobGuid, GET_NEXT_HOB (GuidHob));
  }
}
