/** @file
  Secure Boot authority measurement for the DXE Image Verification Library.

  When an image is authorized by an entry in the `db` signature database, the
  EFI_SIGNATURE_DATA entry that authorized it is measured into PCR 7 as an
  EV_EFI_VARIABLE_AUTHORITY event. This records which trust anchor authorized
  the image (not the image content). Each unique authority entry is measured
  at most once per boot; the module-global MEASURED_AUTHORITIES state, obtained
  through GetMeasuredAuthorities (), provides the de-duplication.

  Caution: This file consumes external input (the signature databases that the
  authority entries are copied from). All inputs must be treated as
  attacker-controlled.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeImageVerificationLib.h"

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TpmMeasurementLib.h>

//
// Number of MEASURED_VARIABLE slots to grow the tracking list by each time it
// fills up.
//
#define MEASURED_AUTHORITY_COUNT_INCREMENT  0x100

//
// Mapping of (VariableName, VendorGuid) pairs that are treated as Secure Boot
// authority variables. Only entries authorized by these variables are measured
// into PCR 7.
//
typedef struct {
  CHAR16      *VariableName;
  EFI_GUID    *VendorGuid;
} AUTHORITY_VARIABLE;

STATIC AUTHORITY_VARIABLE  mAuthorityVariables[] = {
  { EFI_IMAGE_SECURITY_DATABASE, &gEfiImageSecurityDatabaseGuid },
};

//
// Module-global authority measurement state. The only consumer that touches
// this global directly is GetMeasuredAuthorities (); every other function
// operates on a caller-supplied MEASURED_AUTHORITIES pointer so it can be unit
// tested without global state.
//
STATIC MEASURED_AUTHORITIES  mMeasuredAuthorities = { NULL, 0, 0 };

/**
  Return the module-global authority measurement state.

  The returned pointer references storage that persists for the lifetime of
  the module so that measurement de-duplication is maintained across every
  image the verification handler processes.

  @return  Pointer to the module-global MEASURED_AUTHORITIES instance.
**/
MEASURED_AUTHORITIES *
GetMeasuredAuthorities (
  VOID
  )
{
  return &mMeasuredAuthorities;
}

/**
  Return the canonical storage for a Secure Boot authority variable name.

  Measured records reference canonical name storage owned by this module so
  that the records remain valid independent of the caller's buffers.

  @param[in]  VariableName  A Null-terminated authority variable name.

  @return  The canonical pointer for VariableName, or NULL if VariableName is
           not a known authority variable.
**/
CHAR16 *
AssignVariableName (
  IN CHAR16  *VariableName
  )
{
  UINTN  Index;

  if (VariableName == NULL) {
    return NULL;
  }

  for (Index = 0; Index < ARRAY_SIZE (mAuthorityVariables); Index++) {
    if (StrCmp (VariableName, mAuthorityVariables[Index].VariableName) == 0) {
      return mAuthorityVariables[Index].VariableName;
    }
  }

  return NULL;
}

/**
  Return the canonical storage for a Secure Boot authority vendor GUID.

  Measured records reference canonical GUID storage owned by this module so
  that the records remain valid independent of the caller's buffers.

  @param[in]  VendorGuid  A vendor GUID to canonicalize.

  @return  The canonical pointer for VendorGuid, or NULL if VendorGuid is not a
           known authority vendor GUID.
**/
EFI_GUID *
AssignVendorGuid (
  IN EFI_GUID  *VendorGuid
  )
{
  UINTN  Index;

  if (VendorGuid == NULL) {
    return NULL;
  }

  for (Index = 0; Index < ARRAY_SIZE (mAuthorityVariables); Index++) {
    if (CompareGuid (VendorGuid, mAuthorityVariables[Index].VendorGuid)) {
      return mAuthorityVariables[Index].VendorGuid;
    }
  }

  return NULL;
}

/**
  Determine whether a variable is a Secure Boot authority variable.

  @param[in]  VariableName  A Null-terminated variable name.
  @param[in]  VendorGuid    The variable's vendor GUID.

  @retval TRUE   The variable is a Secure Boot authority variable.
  @retval FALSE  The variable is not a Secure Boot authority variable, or a
                 required pointer was NULL.
**/
BOOLEAN
IsSecureAuthorityVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  UINTN  Index;

  if ((VariableName == NULL) || (VendorGuid == NULL)) {
    return FALSE;
  }

  for (Index = 0; Index < ARRAY_SIZE (mAuthorityVariables); Index++) {
    if ((StrCmp (VariableName, mAuthorityVariables[Index].VariableName) == 0) &&
        CompareGuid (VendorGuid, mAuthorityVariables[Index].VendorGuid))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Determine whether an authority entry has already been measured this boot.

  @param[in]  Measured      Authority measurement state to consult.
  @param[in]  VariableName  Name of the variable that authorized the image.
  @param[in]  VendorGuid    Vendor GUID of the variable that authorized the image.
  @param[in]  Data          The EFI_SIGNATURE_DATA entry that authorized the image.
  @param[in]  Size          Size, in bytes, of Data.

  @retval TRUE   The entry has already been measured.
  @retval FALSE  The entry has not been measured, or a required pointer was NULL.
**/
BOOLEAN
IsDataMeasured (
  IN CONST MEASURED_AUTHORITIES  *Measured,
  IN CHAR16                      *VariableName,
  IN EFI_GUID                    *VendorGuid,
  IN VOID                        *Data,
  IN UINTN                       Size
  )
{
  UINTN  Index;

  if ((Measured == NULL) || (VariableName == NULL) || (VendorGuid == NULL) || (Data == NULL)) {
    return FALSE;
  }

  for (Index = 0; Index < Measured->Count; Index++) {
    if ((Measured->List[Index].VariableName != NULL) &&
        (Measured->List[Index].VendorGuid != NULL) &&
        (Measured->List[Index].Data != NULL) &&
        (Size == Measured->List[Index].Size) &&
        (StrCmp (VariableName, Measured->List[Index].VariableName) == 0) &&
        CompareGuid (VendorGuid, Measured->List[Index].VendorGuid) &&
        (CompareMem (Data, Measured->List[Index].Data, Size) == 0))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Record an authority entry as measured, growing the tracking list as needed.

  A private copy of Data is allocated so the record remains valid independent
  of the caller's buffer.

  @param[in,out]  Measured      Authority measurement state to update.
  @param[in]      VariableName  Name of the variable that authorized the image.
  @param[in]      VendorGuid    Vendor GUID of the variable that authorized the image.
  @param[in]      Data          The EFI_SIGNATURE_DATA entry that authorized the image.
  @param[in]      Size          Size, in bytes, of Data.

  @retval EFI_SUCCESS            The entry was recorded.
  @retval EFI_INVALID_PARAMETER  A required pointer was NULL or Size was 0.
  @retval EFI_OUT_OF_RESOURCES   A required allocation failed.
**/
EFI_STATUS
AddDataMeasured (
  IN OUT MEASURED_AUTHORITIES  *Measured,
  IN     CHAR16                *VariableName,
  IN     EFI_GUID              *VendorGuid,
  IN     VOID                  *Data,
  IN     UINTN                 Size
  )
{
  MEASURED_VARIABLE  *NewList;

  if ((Measured == NULL) || (VariableName == NULL) || (VendorGuid == NULL) ||
      (Data == NULL) || (Size == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (Measured->Count <= Measured->Max);

  if (Measured->Count == Measured->Max) {
    NewList = AllocateZeroPool (
                sizeof (MEASURED_VARIABLE) * (Measured->Max + MEASURED_AUTHORITY_COUNT_INCREMENT)
                );
    if (NewList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (Measured->List != NULL) {
      CopyMem (NewList, Measured->List, sizeof (MEASURED_VARIABLE) * Measured->Count);
      FreePool (Measured->List);
    }

    Measured->List = NewList;
    Measured->Max += MEASURED_AUTHORITY_COUNT_INCREMENT;
  }

  Measured->List[Measured->Count].Data = AllocateCopyPool (Size, Data);
  if (Measured->List[Measured->Count].Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Measured->List[Measured->Count].VariableName = AssignVariableName (VariableName);
  Measured->List[Measured->Count].VendorGuid   = AssignVendorGuid (VendorGuid);
  Measured->List[Measured->Count].Size         = Size;
  Measured->Count++;

  return EFI_SUCCESS;
}

/**
  Measure and log an EFI variable, and extend the measurement into PCR 7.

  @param[in]  VarName     A Null-terminated authority variable name.
  @param[in]  VendorGuid  The variable's vendor GUID.
  @param[in]  VarData     The EFI_SIGNATURE_DATA entry that authorized the image.
  @param[in]  VarSize     Size, in bytes, of VarData.

  @retval EFI_SUCCESS            The measurement completed successfully.
  @retval EFI_INVALID_PARAMETER  A required pointer was NULL.
  @retval EFI_OUT_OF_RESOURCES   A required allocation failed.
  @retval EFI_DEVICE_ERROR       The measurement was unsuccessful.
**/
EFI_STATUS
MeasureVariable (
  IN CHAR16    *VarName,
  IN EFI_GUID  *VendorGuid,
  IN VOID      *VarData,
  IN UINTN     VarSize
  )
{
  EFI_STATUS          Status;
  UINTN               VarNameLength;
  UEFI_VARIABLE_DATA  *VarLog;
  UINT32              VarLogSize;

  if ((VarName == NULL) || (VendorGuid == NULL) || (VarData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The UEFI_VARIABLE_DATA.VariableData value shall be the EFI_SIGNATURE_DATA
  // value from the EFI_SIGNATURE_LIST that contained the authority that was
  // used to validate the image.
  //
  VarNameLength = StrLen (VarName);
  VarLogSize    = (UINT32)(sizeof (*VarLog) + VarNameLength * sizeof (*VarName) + VarSize
                           - sizeof (VarLog->UnicodeName) - sizeof (VarLog->VariableData));

  VarLog = (UEFI_VARIABLE_DATA *)AllocateZeroPool (VarLogSize);
  if (VarLog == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&VarLog->VariableName, VendorGuid, sizeof (VarLog->VariableName));
  VarLog->UnicodeNameLength  = VarNameLength;
  VarLog->VariableDataLength = VarSize;
  CopyMem (
    VarLog->UnicodeName,
    VarName,
    VarNameLength * sizeof (*VarName)
    );
  CopyMem (
    (CHAR16 *)VarLog->UnicodeName + VarNameLength,
    VarData,
    VarSize
    );

  DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: MeasureVariable (Pcr - %x, EventType - %x, ", (UINTN)7, (UINTN)EV_EFI_VARIABLE_AUTHORITY));
  DEBUG ((DEBUG_INFO, "VariableName - %s, VendorGuid - %g)\n", VarName, VendorGuid));

  Status = TpmMeasureAndLogData (
             7,
             EV_EFI_VARIABLE_AUTHORITY,
             VarLog,
             VarLogSize,
             VarLog,
             VarLogSize
             );
  FreePool (VarLog);

  return Status;
}

/**
  Measure a Secure Boot authority entry into PCR 7, skipping entries that have
  already been measured this boot.

  If VariableName / VendorGuid do not identify a Secure Boot authority
  variable, or the supplied data has already been measured, the call is a
  no-op. Otherwise the data is measured via TpmMeasureAndLogData and recorded
  in Measured so subsequent identical entries are not measured again.

  @param[in,out]  Measured      Authority measurement state to consult and update.
  @param[in]      VariableName  Name of the variable that authorized the image.
  @param[in]      VendorGuid    Vendor GUID of the variable that authorized the image.
  @param[in]      Authority     The `db` authority that authorized the image, whose Data / Size
                                identify the EFI_SIGNATURE_DATA entry to measure.
**/
VOID
SecureBootHook (
  IN OUT MEASURED_AUTHORITIES   *Measured,
  IN     CHAR16                 *VariableName,
  IN     EFI_GUID               *VendorGuid,
  IN     CONST IMAGE_AUTHORITY  *Authority
  )
{
  EFI_STATUS  Status;
  VOID        *Data;
  UINTN       DataSize;

  if ((Measured == NULL) || (Authority == NULL) ||
      (Authority->Data == NULL) || (Authority->Size == 0))
  {
    return;
  }

  Data     = (VOID *)Authority->Data;
  DataSize = Authority->Size;

  if (!IsSecureAuthorityVariable (VariableName, VendorGuid)) {
    return;
  }

  if (IsDataMeasured (Measured, VariableName, VendorGuid, Data, DataSize)) {
    DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: authority already measured.\n"));
    return;
  }

  Status = MeasureVariable (VariableName, VendorGuid, Data, DataSize);
  DEBUG ((DEBUG_INFO, "DxeImageVerificationLib: MeasureVariable - %r\n", Status));

  if (!EFI_ERROR (Status)) {
    AddDataMeasured (Measured, VariableName, VendorGuid, Data, DataSize);
  }
}
