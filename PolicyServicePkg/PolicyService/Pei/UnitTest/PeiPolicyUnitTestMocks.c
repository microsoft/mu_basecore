/** @file
  Mock implementations for PeiPolicyUnitTest

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <PolicyPei.h>

//
// HOB mocking, this needs to actually work for this test work operate. This is
// implemented as a super simple hob list, supporting only GUID hobs.
//

#define HOB_BUFFER_SIZE  (10 * 0x1000)

UINTN                  HobByteIndex = 0;
UINT8                  HobBuffer[HOB_BUFFER_SIZE];
PEI_POLICY_NOTIFY_HOB  *CurrentPolicyList;

VOID *
EFIAPI
GetNextGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN CONST VOID      *HobStart
  )
{
  EFI_HOB_GUID_TYPE  *Hob;

  Hob = (EFI_HOB_GUID_TYPE *)HobStart;
  while ((UINT8 *)Hob < &HobBuffer[HobByteIndex]) {
    ASSERT (Hob->Header.HobType == EFI_HOB_TYPE_GUID_EXTENSION);
    if (CompareGuid (&Hob->Name, Guid)) {
      if (CompareGuid (Guid, &gPolicyCallbackHobGuid)) {
        CurrentPolicyList = GET_GUID_HOB_DATA (Hob);
      }

      return Hob;
    }

    Hob = GET_NEXT_HOB (Hob);
  }

  return NULL;
}

VOID *
EFIAPI
GetFirstGuidHob (
  IN CONST EFI_GUID  *Guid
  )
{
  return GetNextGuidHob (Guid, &HobBuffer[0]);
}

VOID *
EFIAPI
BuildGuidHob (
  IN CONST EFI_GUID  *Guid,
  IN UINTN           DataLength
  )
{
  EFI_HOB_GUID_TYPE  *Hob;

  ASSERT (HobByteIndex + sizeof (EFI_HOB_GUID_TYPE) + DataLength < HOB_BUFFER_SIZE);

  ZeroMem (&HobBuffer[HobByteIndex], sizeof (EFI_HOB_GUID_TYPE) + DataLength);
  Hob                   = (EFI_HOB_GUID_TYPE *)&HobBuffer[HobByteIndex];
  Hob->Header.HobType   = EFI_HOB_TYPE_GUID_EXTENSION;
  Hob->Header.HobLength = (UINT16)(sizeof (EFI_HOB_GUID_TYPE) + DataLength);
  Hob->Name             = *Guid;
  HobByteIndex         += (sizeof (EFI_HOB_GUID_TYPE) + DataLength);
  if (CompareGuid (Guid, &gPolicyCallbackHobGuid)) {
    CurrentPolicyList = GET_GUID_HOB_DATA (Hob);
  }

  return GET_GUID_HOB_DATA (Hob);
}

//
// Other functions.
//

EFI_STATUS
EFIAPI
PeiServicesAllocatePool (
  IN UINTN  Size,
  OUT VOID  **Buffer
  )
{
  ASSERT (Buffer != NULL);
  *Buffer = AllocatePool (Size);
  ASSERT (*Buffer != NULL);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeiServicesInstallPpi (
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *PpiList
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeiServicesReInstallPpi (
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *OldPpi,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *NewPpi
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeiServicesLocatePpi (
  IN CONST EFI_GUID              *Guid,
  IN UINTN                       Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **PpiDescriptor  OPTIONAL,
  IN OUT VOID                    **Ppi
  )
{
  return EFI_NOT_FOUND;
}
