/** @file
  Definitions for the policy unit tests.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

extern POLICY_INTERFACE  *mPolicyInterface;

#define PEI_TO_DXE_POLICY_COUNT  (5)
#define PEI_TO_DXE_POLICY_SIZE   (10)

//
// Test GUIDs
//

extern CONST EFI_GUID  gPeiToDxePolicyGuids[PEI_TO_DXE_POLICY_COUNT];
#define PEI_TO_DXE_TEST_GUID_FINALIZED  { 0x50c7e443, 0x7060, 0x46f9, { 0xbf, 0xdf, 0xa7, 0x00, 0x6b, 0x29, 0xa8, 0x8c } }

//
// Test structures.
//

extern UINT8  gPeiToDxePolicyData[PEI_TO_DXE_POLICY_COUNT][PEI_TO_DXE_POLICY_SIZE];
#define PEI_TO_DXE_POLICY_FINALIZED  {0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9}

//
// Test names.
//

extern CONST CHAR16  *gPeiToDxePolicyNames[PEI_TO_DXE_POLICY_COUNT];

//
// Common service tests.
//

EFI_STATUS
PolicyServiceCommonCreateTests (
  UNIT_TEST_FRAMEWORK_HANDLE  Framework
  );

//
// Common lib tests.
//

EFI_STATUS
PolicyLibCommonCreateTests (
  UNIT_TEST_FRAMEWORK_HANDLE  Framework
  );
