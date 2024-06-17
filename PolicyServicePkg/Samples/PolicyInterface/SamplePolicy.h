/** @file
  Defines a sample policy as a C struct

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SAMPLE_POLICY_H_
#define _SAMPLE_POLICY_H_

// Sample GUID for a policy passed from PEI to DXE. In production code, this
// should be defined in the .dec file.
#define POLICY_SAMPLE_PEI_TO_DXE_GUID \
  { 0x64e1437f, 0x58f8, 0x4392, { 0xb8, 0xe3, 0x12, 0x02, 0xe4, 0x66, 0x01, 0xc8 } }

// Example of a basic policy C struct. Policies may take any binary format, but
// A C struct is used for this example for simplicity.
typedef struct _SAMPLE_POLICY {
  UINT32    Signature;

  UINT32    Revision;

  UINT32    Value;

  // Etc...
} SAMPLE_POLICY;

#define SAMPLE_POLICY_SIGNATURE  SIGNATURE_32 ('S','P','O','L')
#define SAMPLE_POLICY_REVISION   (10)
#define SAMPLE_POLICY_VALUE      (12345)

#endif
