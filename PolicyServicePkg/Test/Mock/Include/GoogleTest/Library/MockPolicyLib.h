/** @file
  Mock definitions for PolicyLib for Google Test.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MOCK_POLICY_LIB_H_
#define MOCK_POLICY_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
#include <Uefi.h>
#include <Library/PolicyLib.h>
}

struct MockPolicyLib {
  MOCK_INTERFACE_DECLARATION (MockPolicyLib);

  //
  // Standard policy access routines.
  //

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetPolicy,
    (IN CONST EFI_GUID  *PolicyGuid,
     IN UINT64          Attributes,
     IN VOID            *Policy,
     IN UINT16          PolicySize)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetPolicy,
    (IN CONST EFI_GUID  *PolicyGuid,
     OUT UINT64         *Attributes OPTIONAL,
     OUT VOID           *Policy,
     IN OUT UINT16      *PolicySize)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RemovePolicy,
    (IN CONST EFI_GUID  *PolicyGuid)
    );

  //
  // Verified policy routines.
  //

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetVerifiedPolicy,
    (IN CONST EFI_GUID                    *PolicyGuid,
     IN CONST VERIFIED_POLICY_DESCRIPTOR  *Descriptor,
     OUT UINT64                           *Attributes OPTIONAL,
     OUT EFI_HANDLE                       *DataHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CreateVerifiedPolicy,
    (IN CONST VERIFIED_POLICY_DESCRIPTOR  *Descriptor,
     OUT EFI_HANDLE                       *DataHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetVerifiedPolicy,
    (IN CONST EFI_GUID  *PolicyGuid,
     IN UINT64          Attributes,
     IN EFI_HANDLE      DataHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CloseVerifiedPolicy,
    (IN EFI_HANDLE  DataHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReportVerifiedPolicyAccess,
    (IN EFI_HANDLE      DataHandle,
     IN CONST EFI_GUID  *CallerGuid,
     IN UINT32          Offset,
     IN UINT32          Size,
     IN BOOLEAN         Write)
    );
};

#endif
