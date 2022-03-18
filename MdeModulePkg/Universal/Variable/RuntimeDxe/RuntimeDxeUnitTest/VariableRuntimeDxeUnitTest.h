/** @file
Host-based unit test for the VariableRuntimeDxe driver. Will
use mocks for all external interfaces.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef VARIABLE_RUNTIME_DXE_UNIT_TEST_H_
#define VARIABLE_RUNTIME_DXE_UNIT_TEST_H_

#include <Uefi.h>
#include <Uefi/UefiMultiPhase.h>
#include <Guid/VariableFormat.h>
#include <Guid/GlobalVariable.h>

#define VAR_TYPE_STANDARD   0x00
#define VAR_TYPE_TIME_AUTH  0x01

#define DATA_ENC_HEX     0x00
#define DATA_ENC_BASE64  0x01

typedef struct _TEST_VARIABLE_HEADER {
  CHAR8       *TestName;
  CHAR16      *Name;
  EFI_GUID    VendorGuid;
  UINT32      Attributes;
  UINT32      VarType;
  CHAR8       *Data;
  UINT32      DataEnc;
} TEST_VARIABLE_HEADER;

typedef struct _TEST_VARIABLE_AUTH {
  TEST_VARIABLE_HEADER    Header;
  EFI_TIME                Timestamp;
  CHAR8                   *SigData;
  UINT32                  SigDataEnc;
} TEST_VARIABLE_AUTH;

typedef struct _TEST_VARIABLE_MODEL {
  CHAR8       *TestName;
  CHAR16      *Name;
  EFI_GUID    VendorGuid;
  UINT32      Attributes;
  UINT32      VarType;
  UINT8       *Data;            // ALLOCATED
  UINT32      DataSize;
  UINT8       *SigData;         // ALLOCATED, OPTIONAL
  UINT32      SigDataSize;      // OPTIONAL
  EFI_TIME    Timestamp;        // OPTIONAL
} TEST_VARIABLE_MODEL;
#define     T_VAR  TEST_VARIABLE_MODEL

TEST_VARIABLE_MODEL *
LoadTestVariable (
  IN CONST    CHAR8  *TestName
  );

VOID
FreeTestVariable (
  IN OUT      TEST_VARIABLE_MODEL  *VarModel
  );

#endif // VARIABLE_RUNTIME_DXE_UNIT_TEST_H_
