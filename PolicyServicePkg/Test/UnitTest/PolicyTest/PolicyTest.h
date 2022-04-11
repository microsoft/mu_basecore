/** @file
  Definitions for the policy unit tests.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

extern POLICY_INTERFACE  *mPolicyInterface;

//
// Test GUIDs
//

#define PEI_TO_DXE_TEST_GUID            { 0x90c07ec2, 0x731a, 0x48a7, { 0x80, 0x0a, 0xa2, 0x81, 0x17, 0x1d, 0xa4, 0xdb } }
#define PEI_TO_DXE_TEST_GUID_FINALIZED  { 0x50c7e443, 0x7060, 0x46f9, { 0xbf, 0xdf, 0xa7, 0x00, 0x6b, 0x29, 0xa8, 0x8c } }

//
// Test structures.
//

#define PEI_TO_DXE_POLICY_SIZE       (10)
#define PEI_TO_DXE_POLICY            {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09}
#define PEI_TO_DXE_POLICY_FINALIZED  {0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9}

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
