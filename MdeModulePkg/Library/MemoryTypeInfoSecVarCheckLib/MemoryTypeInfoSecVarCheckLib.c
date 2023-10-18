/** @file
  Implementation functions and structures for MemoryTypeInformation VarCheck library.

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
Copyright (C) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// MU_CHANGE TCBZ1086 [WHOLE FILE] - Mitigate potential system brick due to uefi MemoryTypeInformation var changes

#include <Uefi/UefiBaseType.h>
#include <Library/VarCheckLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SafeIntLib.h>
#include <Library/PcdLib.h>

#include <Guid/MemoryTypeInformation.h>

//
// The following 6 runtime visible memory types are currently expected in the
// memory type information variable:
//
//   1. EfiACPIMemoryNVS
//   2. EfiACPIReclaimMemory
//   3. EfiReservedMemoryType
//   4. EfiRuntimeServicesCode
//   5. EfiRuntimeServicesData
//   6. EfiMaxMemoryType
//
#define   EFI_MEMORY_TYPE_INFORMATION_VARIABLE_INFO_COUNT  6
#define   EFI_MEMORY_TYPE_INFORMATION_VARIABLE_SIZE        (sizeof(EFI_MEMORY_TYPE_INFORMATION) * EFI_MEMORY_TYPE_INFORMATION_VARIABLE_INFO_COUNT)

VAR_CHECK_VARIABLE_PROPERTY  mMemoryTypeInfoVariableProperty = {
  VAR_CHECK_VARIABLE_PROPERTY_REVISION,
  0,
  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
  sizeof (EFI_MEMORY_TYPE_INFORMATION),
  sizeof (EFI_MEMORY_TYPE_INFORMATION) * (EfiMaxMemoryType + 1)
};

/**
  SetVariable check handler for MemoryTypeInformation variable.

  @param[in] VariableName       Name of Variable to set.
  @param[in] VendorGuid         Variable vendor GUID.
  @param[in] Attributes         Attribute value of the variable.
  @param[in] DataSize           Size of Data to set.
  @param[in] Data               Data pointer.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER An invalid combination of attribute bits, name, GUID,
                                DataSize and Data value was supplied.

**/
EFI_STATUS
EFIAPI
MemoryTypeInfoVarCheckHandler (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInfo;
  UINTN                        Count;
  UINTN                        Index;
  UINTN                        Index2;
  UINTN                        TotalPages = 0;

  if ((StrCmp (VariableName, EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME) != 0) ||
      !CompareGuid (VendorGuid, &gEfiMemoryTypeInformationGuid))
  {
    //
    // It is not MemoryTypeInformation variable.
    //
    return EFI_SUCCESS;
  }

  if ((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) ||
      (Attributes == 0))
  {
    //
    // Do not check variable deletion.
    //
    return EFI_SUCCESS;
  }

  //
  // Check DataSize.
  //
  if (DataSize != EFI_MEMORY_TYPE_INFORMATION_VARIABLE_SIZE) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a() - DataSize = 0x%x Expected = 0x%x. Check actual memory types against expected memory types.\n",
      __FUNCTION__,
      DataSize,
      EFI_MEMORY_TYPE_INFORMATION_VARIABLE_SIZE
      ));
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Get entry Count.
  //
  Count          = EFI_MEMORY_TYPE_INFORMATION_VARIABLE_INFO_COUNT;
  MemoryTypeInfo = (EFI_MEMORY_TYPE_INFORMATION *)Data;

  //
  // Check last entry's Type.
  //
  if (MemoryTypeInfo[Count - 1].Type != EfiMaxMemoryType) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a() - Last(0x%x) entry's Type(0x%x) != EfiMaxMemoryType\n",
      __FUNCTION__,
      Count - 1,
      MemoryTypeInfo[Count - 1].Type
      ));
    return EFI_SECURITY_VIOLATION;
  }

  if (Count >= 2) {
    for (Index = 0; Index < Count - 1; Index++) {
      //
      // Check the Type.
      //
      if (MemoryTypeInfo[Index].Type > EfiMaxMemoryType) {
        //
        // The Type is invalid.
        //
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: %a() - 0x%x entry's Type(0x%x) is invalid\n",
          __FUNCTION__,
          Index,
          MemoryTypeInfo[Index].Type
          ));
        return EFI_SECURITY_VIOLATION;
      }

      for (Index2 = Index + 1; Index2 < Count; Index2++) {
        if (MemoryTypeInfo[Index].Type == MemoryTypeInfo[Index2].Type) {
          //
          // Two entries have same Type.
          //
          DEBUG ((
            DEBUG_ERROR,
            "ERROR: %a() - 0x%x entry and 0x%x entry have same Type(0x%x)\n",
            __FUNCTION__,
            Index,
            Index2,
            MemoryTypeInfo[Index].Type
            ));
          return EFI_SECURITY_VIOLATION;
        }
      }

      if (EFI_ERROR (SafeUintnAdd (TotalPages, MemoryTypeInfo[Index].NumberOfPages, &TotalPages))) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: %a() - Math error at entry 0x%x\n",
          __FUNCTION__,
          Index
          ));
        return EFI_SECURITY_VIOLATION;
      }
    }
  }

  //
  // Compare the pages to the upper bound and reject if too large.
  if (TotalPages > PcdGet32 (PcdMaxMemoryTypeInfoPages)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a() - Total pages requested 0x%x exceeds max allowed(0x%x)\n",
      __FUNCTION__,
      TotalPages,
      PcdGet32 (PcdMaxMemoryTypeInfoPages)
      ));
    return EFI_SECURITY_VIOLATION;
  }

  return EFI_SUCCESS;
}

/**
  Constructor function of MemoryTypeInfoSecVarCheckLib to set property and
  register SetVariable check handler for MemoryTypeInformation variable.

  @retval RETURN_SUCCESS       The constructor executed correctly.

**/
RETURN_STATUS
EFIAPI
MemoryTypeInfoSecVarCheckLibConstructor (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "%a()\n", __FUNCTION__));
  EFI_STATUS  Status;

  Status = VarCheckLibVariablePropertySet (
             EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
             &gEfiMemoryTypeInformationGuid,
             &mMemoryTypeInfoVariableProperty
             );
  ASSERT_EFI_ERROR (Status);
  Status = VarCheckLibRegisterSetVariableCheckHandler (
             MemoryTypeInfoVarCheckHandler
             );
  ASSERT_EFI_ERROR (Status);

  return RETURN_SUCCESS;
}
