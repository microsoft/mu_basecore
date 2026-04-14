/** @file
  // MU_CHANGE
  This library is the PeilessSec version of the HashLib. It will
  initiate a hash on each supported hash algorithm via the TPM or
  TransferList.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  Copyright (c), Microsoft Corporation // MU_CHANGE

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Guid/TcgEventHob.h>
#include <Guid/TpmInstance.h>
#include <Guid/TransferListHob.h>

#include <IndustryStandard/UefiTcgPlatform.h>

#include <Library/ArmTransferListLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/HashLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
// MU_CHANGE - [BEGIN]
#include <Library/Tpm2HelpLib.h>

#if 0
#define HASH_ALG_ERROR   0x00
#define BAD_SEQ_HANDLE   0xFFFFFFFF
#define BAD_HASH_HANDLE  0x00

typedef struct {
  TPM_ALG_ID        AlgoId;
  UINT32            Mask;
  TPMI_DH_OBJECT    SequenceHandle;
} TPM2_HASH_MASK;

STATIC TPM2_HASH_MASK  mTpm2HashMask[] = {
  { TPM_ALG_SHA1,   HASH_ALG_SHA1,   BAD_SEQ_HANDLE },
  { TPM_ALG_SHA256, HASH_ALG_SHA256, BAD_SEQ_HANDLE },
  { TPM_ALG_SHA384, HASH_ALG_SHA384, BAD_SEQ_HANDLE },
  { TPM_ALG_SHA512, HASH_ALG_SHA512, BAD_SEQ_HANDLE },
};

STATIC UINT32   mSupportedHashBitmap;
STATIC BOOLEAN  mHashLibDisabled;
#endif
// MU_CHANGE - [END]

/**
  Get transfer list header.

  @param[out] TransferList  Transfer list header

  @retval EFI_SUCCESS      Transfer list is found.
  @retval EFI_NOT_FOUND    Transfer list is not found.

**/
STATIC
EFI_STATUS
EFIAPI
GetTransferList (
  OUT TRANSFER_LIST_HEADER  **TransferList
  )
{
  VOID               *HobList;
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINTN              *GuidHobData;

  *TransferList = NULL;

  HobList = GetHobList ();
  if (HobList == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidHob = GetNextGuidHob (&gArmTransferListHobGuid, HobList);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidHobData = GET_GUID_HOB_DATA (GuidHob);

  *TransferList = (TRANSFER_LIST_HEADER *)(*GuidHobData);

  return EFI_SUCCESS;
}

// MU_CHANGE - [BEGIN]

/**
  Get supported hash bitmap

  @param[out] SupportedHashBitmap

  @retval EFI_SUCCESS            Bitmap populated
  @retval EFI_INVALID_PARAMETER  Invalid pointer
  @retval EFI_NOT_FOUND          Error accessing data
  @retval EFI_DEVICE_ERROR       TPM device error

**/
STATIC
EFI_STATUS
EFIAPI
GetSupportedHashBitmap (
  OUT UINT32  *SupportedHashBitmap
  )
{
  EFI_STATUS                       Status;
  TRANSFER_LIST_HEADER             *TransferList;
  VOID                             *EventLog;
  UINTN                            EventLogSize;
  TCG_PCR_EVENT                    *TcgPcrEvent;
  TCG_EfiSpecIDEventStruct         *TcgEfiSpecIdEventStruct;
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINTN                            Idx;
  UINT32                           NumberOfAlgorithms;
  UINT32                           TpmHashBitmap;
  UINT32                           PcrHashBitmap;
  BOOLEAN                          UseTlHashBitmap;

  if (SupportedHashBitmap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: TPM2 not detected!\n", __func__));
    return Status;
  }

  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashBitmap, &PcrHashBitmap);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Tpm capability... Status: %r\n", __func__, Status));
    return Status;
  }

  // NOTE: TpmHashBitmap is what the TPM supports, PcrHashBitmap is what is currently active
  DEBUG ((DEBUG_INFO, "TpmHashBitmap: %x, PcrHashBitmap: %x\n", TpmHashBitmap, PcrHashBitmap));

  UseTlHashBitmap = FALSE;
  Status          = GetTransferList (&TransferList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Unable to acquire Transfer list...\n", __func__));
    goto Exit;
  }

  if (TransferListCheckHeader (TransferList) == TRANSFER_LIST_OPS_INVALID) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Transfer list...\n", __func__));
    goto Exit;
  }

  Status = TransferListGetEventLog (TransferList, &EventLog, &EventLogSize, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: No data for TPM event log...\n", __func__));
    goto Exit;
  }

  UseTlHashBitmap         = TRUE;
  TcgPcrEvent             = (TCG_PCR_EVENT *)EventLog;
  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)
                            (EventLog + OFFSET_OF (TCG_PCR_EVENT, Event));

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  DEBUG ((DEBUG_INFO, "%a: Transfer list TPM event log available\n", __func__));

  // Update the supported hash bitmap based on the info from the TCG event log
  for (Idx = 0; Idx < NumberOfAlgorithms; Idx++) {
    *SupportedHashBitmap |= GetHashMaskFromAlgo (DigestSize[Idx].algorithmId);
  }

  // The active PCR banks should match what is reported in the TCG event log
  if (PcrHashBitmap != *SupportedHashBitmap) {
    DEBUG ((DEBUG_ERROR, "Active PCRs & Transfer List mismatch!\n"));
    UseTlHashBitmap = FALSE;
  }

Exit:
  if (!UseTlHashBitmap) {
    // Use the information from the TPM to update the supported hash bitmap
    *SupportedHashBitmap = TpmHashBitmap;
    DEBUG ((DEBUG_INFO, "%a: No Transfer List or TPM event log available\n", __func__));
  }

  *SupportedHashBitmap &= PcrHashBitmap;
  if (*SupportedHashBitmap == 0x00) {
    DEBUG ((DEBUG_ERROR, "%a: No supported Hash algorithm with event log Spec...!\n", __func__));
  }

  return EFI_SUCCESS;
}

#if 0

/**
  The function get algorithm from hash mask info.

  @param[in]  HashMask

  @return Hash algorithm

**/
STATIC
TPM_ALG_ID
EFIAPI
Tpm2GetAlgoFromHashMask (
  IN UINT32  HashMask
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    if (mTpm2HashMask[Idx].Mask == HashMask) {
      return mTpm2HashMask[Idx].AlgoId;
    }
  }

  return TPM_ALG_ERROR;
}

/**
  The function get hashmask from algorithm info.

  @param[in]  AlgoId

  @return Hash mask

**/
STATIC
UINT32
EFIAPI
Tpm2GetHashMaskFromAlgo (
  TPM_ALG_ID  AlgoId
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    if (mTpm2HashMask[Idx].AlgoId == AlgoId) {
      return mTpm2HashMask[Idx].Mask;
    }
  }

  return HASH_ALG_ERROR;
}

/**
  Validate hash handle.

  @param[in]   HashHandle        HashHandle

  @return EFI_SUCCESS
  @return EFI_INVALID_PARAMETER  Invalidate HashHandle

**/
STATIC
EFI_STATUS
EFIAPI
ValidateHashHandle (
  IN HASH_HANDLE  HashHandle
  )
{
  UINT32  Idx;
  UINT32  HashMask;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    HashMask = 1 << Idx;

    if ((HashHandle & HashMask) == 0x00) {
      continue;
    }

    if (((mSupportedHashBitmap & HashMask) == 0x00) ||
        (mTpm2HashMask[Idx].SequenceHandle == BAD_SEQ_HANDLE))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Clear Sequence Handles.

**/
STATIC
VOID
ClearSequenceHandles (
  IN VOID
  )
{
  UINT32  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mTpm2HashMask); Idx++) {
    mTpm2HashMask[Idx].SequenceHandle = BAD_SEQ_HANDLE;
  }
}

#endif
// MU_CHANGE - [END]

/**
  Start hash sequence.

  @param HashHandle Hash handle.

  @retval EFI_SUCCESS          Hash sequence start and HandleHandle returned.
  @retval EFI_OUT_OF_RESOURCES No enough resource to start hash.

**/
EFI_STATUS
EFIAPI
HashStart (
  OUT HASH_HANDLE  *HashHandle
  )
{
  EFI_STATUS  Status;
  TPM_ALG_ID  AlgoId;
  UINT32      Idx;
  // MU_CHANGE - [BEGIN]
  UINT32       SupportedHashBitmap;
  HASH_HANDLE  *HashCtx;
  UINTN        HashInfoSize;

  SupportedHashBitmap = 0;
  Status              = GetSupportedHashBitmap (&SupportedHashBitmap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SupportedHashBitmap == 0) {
    return EFI_DEVICE_ERROR;
  }

  HashInfoSize = Tpm2GetHashInfoSize ();
  HashCtx      = AllocatePool (HashInfoSize * sizeof (HASH_HANDLE));
  if (HashCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Idx = 0; Idx < HashInfoSize; Idx++) {
    if ((Tpm2GetHashMaskAtIndex (Idx) & SupportedHashBitmap) == 0) {
      continue;
    }

    AlgoId = Tpm2GetHashAlgoFromMask (Tpm2GetHashMaskAtIndex (Idx));
    if (AlgoId == TPM_ALG_ERROR) {
      return EFI_UNSUPPORTED;
    }

    Status = Tpm2HashSequenceStart (AlgoId, (TPMI_DH_OBJECT *)&HashCtx[Idx]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  *HashHandle = (HASH_HANDLE)HashCtx;
  // MU_CHANGE - [END]

  return Status;
}

/**
  Update hash sequence data.

  @param HashHandle    Hash handle.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.

  @retval EFI_SUCCESS     Hash sequence updated.

**/
EFI_STATUS
EFIAPI
HashUpdate (
  IN HASH_HANDLE  HashHandle,
  IN VOID         *DataToHash,
  IN UINTN        DataToHashLen
  )
{
  EFI_STATUS  Status;
  UINT32      Idx;
  // UINT32            HashMask; // MU_CHANGE
  UINT8             *Buffer;
  UINT64            HashLen;
  TPM2B_MAX_BUFFER  HashBuffer;
  // MU_CHANGE - [BEGIN]
  UINT32       SupportedHashBitmap;
  HASH_HANDLE  *HashCtx;
  UINTN        HashInfoSize;

  SupportedHashBitmap = 0;
  Status              = GetSupportedHashBitmap (&SupportedHashBitmap);
  // MU_CHANGE - [END]
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // MU_CHANGE - [BEGIN]
  if (SupportedHashBitmap == 0) {
    return EFI_DEVICE_ERROR;
  }

  HashCtx = (HASH_HANDLE *)HashHandle;

  HashInfoSize = Tpm2GetHashInfoSize ();
  for (Idx = 0; Idx < HashInfoSize; Idx++) {
    if ((Tpm2GetHashMaskAtIndex (Idx) & SupportedHashBitmap) == 0) {
      // MU_CHANGE - [END]
      continue;
    }

    Buffer = (UINT8 *)(UINTN)DataToHash;
    for (HashLen = DataToHashLen; HashLen > sizeof (HashBuffer.buffer); HashLen -= sizeof (HashBuffer.buffer)) {
      HashBuffer.size = sizeof (HashBuffer.buffer);
      CopyMem (HashBuffer.buffer, Buffer, sizeof (HashBuffer.buffer));
      Buffer += sizeof (HashBuffer.buffer);

      Status = Tpm2SequenceUpdate ((TPMI_DH_OBJECT)HashCtx[Idx], &HashBuffer); // MU_CHANGE
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    }

    // Last one
    HashBuffer.size = (UINT16)HashLen;
    CopyMem (HashBuffer.buffer, Buffer, (UINTN)HashLen);
    Status = Tpm2SequenceUpdate ((TPMI_DH_OBJECT)HashCtx[Idx], &HashBuffer); // MU_CHANGE
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
  }

  return Status;
}

/**
  Hash sequence complete and extend to PCR.

  @param HashHandle    Hash handle.
  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash sequence complete and DigestList is returned.

**/
EFI_STATUS
EFIAPI
HashCompleteAndExtend (
  IN HASH_HANDLE          HashHandle,
  IN TPMI_DH_PCR          PcrIndex,
  IN VOID                 *DataToHash,
  IN UINTN                DataToHashLen,
  OUT TPML_DIGEST_VALUES  *DigestList
  )
{
  EFI_STATUS  Status;
  UINT32      Idx;
  UINT32      DigestIdx;
  // UINT32            HashMask; // MU_CHANGE
  UINT8             *Buffer;
  UINT64            HashLen;
  TPM2B_MAX_BUFFER  HashBuffer;
  TPM_ALG_ID        AlgoId;
  TPM2B_DIGEST      Result;
  // MU_CHANGE - [BEGIN]
  UINT32       SupportedHashBitmap;
  HASH_HANDLE  *HashCtx;
  UINTN        HashInfoSize;

  SupportedHashBitmap = 0;
  Status              = GetSupportedHashBitmap (&SupportedHashBitmap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SupportedHashBitmap == 0) {
    return EFI_DEVICE_ERROR;
  }

  // MU_CHANGE - [END]

  ZeroMem (DigestList, sizeof (*DigestList));
  DigestList->count = HASH_COUNT;
  DigestIdx         = 0;
  HashCtx           = (HASH_HANDLE *)HashHandle; // MU_CHANGE

  HashInfoSize = Tpm2GetHashInfoSize ();
  for (Idx = 0; Idx < HashInfoSize; Idx++) {
    // MU_CHANGE
    if ((Tpm2GetHashMaskAtIndex (Idx) & SupportedHashBitmap) == 0) {
      // MU_CHANGE
      continue;
    }

    Buffer = (UINT8 *)(UINTN)DataToHash;
    for (HashLen = DataToHashLen; HashLen > sizeof (HashBuffer.buffer); HashLen -= sizeof (HashBuffer.buffer)) {
      HashBuffer.size = sizeof (HashBuffer.buffer);
      CopyMem (HashBuffer.buffer, Buffer, sizeof (HashBuffer.buffer));
      Buffer += sizeof (HashBuffer.buffer);

      Status = Tpm2SequenceUpdate ((TPMI_DH_OBJECT)HashCtx[Idx], &HashBuffer); // MU_CHANGE
      if (EFI_ERROR (Status)) {
        goto Error; // MU_CHANGE
      }
    }

    // Last one
    HashBuffer.size = (UINT16)HashLen;
    CopyMem (HashBuffer.buffer, Buffer, (UINTN)HashLen);

    Status = Tpm2SequenceComplete ((TPMI_DH_OBJECT)HashCtx[Idx], &HashBuffer, &Result); // MU_CHANGE
    if (EFI_ERROR (Status)) {
      goto Error; // MU_CHANGE
    }

    // MU_CHANGE - [BEGIN]
    AlgoId = Tpm2GetHashAlgoFromMask (Tpm2GetHashMaskAtIndex (Idx));
    if (AlgoId == TPM_ALG_ERROR) {
      Status = EFI_UNSUPPORTED;
      goto Error;
    }

    // MU_CHANGE - [END]

    // Copy the result of hash.
    CopyMem (&DigestList->digests[DigestIdx].digest, Result.buffer, Result.size);
    DigestList->digests[DigestIdx].hashAlg = AlgoId;
    DigestIdx++;
  }

  DigestList->count = DigestIdx;

  // MU_CHANGE - [BEGIN]
  Status = Tpm2PcrExtend (PcrIndex, DigestList);

Error:
  FreePool (HashCtx);

  return Status;
  // MU_CHANGE - [END]
}

/**
  Hash data and extend to PCR.

  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash data and DigestList is returned.

**/
EFI_STATUS
EFIAPI
HashAndExtend (
  IN TPMI_DH_PCR          PcrIndex,
  IN VOID                 *DataToHash,
  IN UINTN                DataToHashLen,
  OUT TPML_DIGEST_VALUES  *DigestList
  )
{
  EFI_STATUS        Status;
  UINT8             *Buffer;
  UINT64            HashLen;
  TPMI_DH_OBJECT    SequenceHandle;
  TPM2B_MAX_BUFFER  HashBuffer;
  TPM2B_DIGEST      Result;
  TPM_ALG_ID        AlgoId;
  UINT32            Idx;
  UINT32            DigestIdx;
  UINT32            SupportedHashBitmap; // MU_CHANGE
  UINT32            HashInfoSize;        // MU_CHANGE

  DEBUG ((DEBUG_VERBOSE, "\n HashAndExtend Entry \n"));

  // MU_CHANGE - [BEGIN]
  SupportedHashBitmap = 0;
  Status              = GetSupportedHashBitmap (&SupportedHashBitmap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SupportedHashBitmap == 0x00) {
    return EFI_DEVICE_ERROR;
  }

  // MU_CHANGE - [END]

  ZeroMem (DigestList, sizeof (*DigestList));
  DigestList->count = HASH_COUNT;
  DigestIdx         = 0;

  HashInfoSize = Tpm2GetHashInfoSize (); // MU_CHANGE
  for (Idx = 0; Idx < HashInfoSize; Idx++) {
    // MU_CHANGE
    if ((Tpm2GetHashMaskAtIndex (Idx) & SupportedHashBitmap) == 0) {
      // MU_CHANGE
      continue;
    }

    // MU_CHANGE - [BEGIN]
    DEBUG ((DEBUG_INFO, "Hashing with Mask: %x\n", Tpm2GetHashMaskAtIndex (Idx)));
    AlgoId = Tpm2GetHashAlgoFromMask (Tpm2GetHashMaskAtIndex (Idx));
    if (AlgoId == TPM_ALG_ERROR) {
      return EFI_UNSUPPORTED;
    }

    // MU_CHANGE - [END]

    Status = Tpm2HashSequenceStart (AlgoId, &SequenceHandle);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    DEBUG ((DEBUG_VERBOSE, "\n Tpm2HashSequenceStart Success \n"));
    DEBUG ((DEBUG_INFO, "Hashing %d bytes of data\n", DataToHashLen)); // MU_CHANGE

    Buffer = (UINT8 *)(UINTN)DataToHash;
    for (HashLen = DataToHashLen; HashLen > sizeof (HashBuffer.buffer); HashLen -= sizeof (HashBuffer.buffer)) {
      HashBuffer.size = sizeof (HashBuffer.buffer);
      CopyMem (HashBuffer.buffer, Buffer, sizeof (HashBuffer.buffer));
      Buffer += sizeof (HashBuffer.buffer);

      Status = Tpm2SequenceUpdate (SequenceHandle, &HashBuffer);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    }

    DEBUG ((DEBUG_VERBOSE, "\n Tpm2SequenceUpdate Success \n"));

    HashBuffer.size = (UINT16)HashLen;
    CopyMem (HashBuffer.buffer, Buffer, (UINTN)HashLen);

    Status = Tpm2SequenceComplete (SequenceHandle, &HashBuffer, &Result); // MU_CHANGE
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    DEBUG ((DEBUG_VERBOSE, "\n Tpm2SequenceComplete Success \n"));

    CopyMem (&DigestList->digests[DigestIdx].digest, Result.buffer, Result.size);
    DigestList->digests[DigestIdx].hashAlg = AlgoId;
    DigestIdx++;
  }

  DigestList->count = DigestIdx;

  DEBUG ((DEBUG_INFO, "Extending to PCR%d\n", PcrIndex)); // MU_CHANGE
  Status = Tpm2PcrExtend (PcrIndex, DigestList);          // MU_CHANGE
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_VERBOSE, "\n Tpm2PcrExtend Success \n"));

  return EFI_SUCCESS;
}

/**
  This service register Hash.

  @param HashInterface  Hash interface

  @retval EFI_SUCCESS          This hash interface is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this interface.
  @retval EFI_ALREADY_STARTED  System already register this interface.

**/
EFI_STATUS
EFIAPI
RegisterHashInterfaceLib (
  IN HASH_INTERFACE  *HashInterface
  )
{
  return EFI_UNSUPPORTED;
}

// MU_CHANGE - [BEGIN]
#if 0

/**
  Constructor of HashLibTpm2PeilessSecLibConstructor.

**/
EFI_STATUS
EFIAPI
HasLibTpm2PeilessSecLibConstructor (
  VOID
  )
{
  EFI_STATUS                       Status;
  TRANSFER_LIST_HEADER             *TransferList;
  VOID                             *EventLog;
  UINTN                            EventLogSize;
  TCG_PCR_EVENT                    *TcgPcrEvent;
  TCG_EfiSpecIDEventStruct         *TcgEfiSpecIdEventStruct;
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINTN                            Idx;
  UINT32                           NumberOfAlgorithms;
  UINT32                           TpmHashBitmap;
  UINT32                           PcrHashBitmap;

  mHashLibDisabled = TRUE;

  Status = GetTransferList (&TransferList);
  if (EFI_ERROR (Status)) {
    goto DisableHandler;
  }

  if (TransferListCheckHeader (TransferList) == TRANSFER_LIST_OPS_INVALID) {
    DEBUG ((DEBUG_ERROR, "Invalid Transfer list..\n"));
    goto DisableHandler;
  }

  Status = TransferListGetEventLog (TransferList, &EventLog, &EventLogSize, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: No data for Tpm event log...\n", __func__));
    goto DisableHandler;
  }

  TcgPcrEvent             = (TCG_PCR_EVENT *)EventLog;
  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)
                            (EventLog + OFFSET_OF (TCG_PCR_EVENT, Event));

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));

  for (Idx = 0; Idx < NumberOfAlgorithms; Idx++) {
    mSupportedHashBitmap |= Tpm2GetHashMaskFromAlgo (DigestSize[Idx].algorithmId);
  }

  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: TPM2 not detected!\n", __func__));
    BuildGuidHob (&gTpmErrorHobGuid, 0);
    goto DisableHandler;
  }

  Status = Tpm2GetCapabilitySupportedAndActivePcrs (&TpmHashBitmap, &PcrHashBitmap);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Tpm capability... Status: %r\n", __func__, Status));
    goto DisableHandler;
  }

  mSupportedHashBitmap &= PcrHashBitmap;
  if (mSupportedHashBitmap == 0x00) {
    DEBUG ((DEBUG_ERROR, "%a: No supported Hash algorithm with event log Spec...!\n", __func__));
    BuildGuidHob (&gTpmErrorHobGuid, 0);
    goto DisableHandler;
  }

  mHashLibDisabled = FALSE;

DisableHandler:
  return EFI_SUCCESS;
}

#endif
// MU_CHANGE - [END]
