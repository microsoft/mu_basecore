/** @file
  Unit tests for EcitEncodingLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/EcitEncodingLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME     "EcitEncodingLib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

STATIC
UNIT_TEST_STATUS
EFIAPI
TestEmptyOidSet (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  VOID        *EntryData;
  UINTN       EntryDataSize;
  UINT8       *Bytes;
  UINTN       Index;

  EntryData     = NULL;
  EntryDataSize = 0;
  Status        = EcitEncodeOidSet (NULL, 0, &EntryData, &EntryDataSize);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (EntryData);
  UT_ASSERT_EQUAL (EntryDataSize, 8);

  Bytes = EntryData;
  for (Index = 0; Index < EntryDataSize; Index++) {
    UT_ASSERT_EQUAL (Bytes[Index], 0);
  }

  FreePool (EntryData);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestOidSetEncoding (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST CHAR8        Oid1[]         = "1.2.840.113549.1.1.11";
  STATIC CONST CHAR8        Oid2[]         = "2.16.840.1.101.3.4.2.1";
  BASE_CRYPT_OP_CAPABILITY  Capabilities[] = {
    { Oid1, sizeof (Oid1) },
    { Oid1, sizeof (Oid1) },
    { Oid2, sizeof (Oid2) }
  };
  STATIC CONST CHAR8        Expected[] =
    "1.2.840.113549.1.1.11,2.16.840.1.101.3.4.2.1";
  EFI_STATUS  Status;
  VOID        *EntryData;
  UINTN       EntryDataSize;
  UINT8       *Bytes;
  UINTN       Index;

  EntryData     = NULL;
  EntryDataSize = 0;
  Status        = EcitEncodeOidSet (
                    Capabilities,
                    ARRAY_SIZE (Capabilities),
                    &EntryData,
                    &EntryDataSize
                    );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (EntryData);
  UT_ASSERT_EQUAL (EntryDataSize % 8, 0);
  UT_ASSERT_TRUE (EntryDataSize >= sizeof (Expected));
  UT_ASSERT_EQUAL (CompareMem (EntryData, Expected, sizeof (Expected)), 0);

  Bytes = EntryData;
  for (Index = sizeof (Expected); Index < EntryDataSize; Index++) {
    UT_ASSERT_EQUAL (Bytes[Index], 0);
  }

  FreePool (EntryData);
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestInvalidOid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST CHAR8        *InvalidOids[] = {
    "1..2",
    "3.1",
    "1.40",
    "1.184467440737095516160"
  };
  BASE_CRYPT_OP_CAPABILITY  Capability;
  EFI_STATUS                Status;
  VOID                      *EntryData;
  UINTN                     EntryDataSize;
  UINTN                     Index;

  for (Index = 0; Index < ARRAY_SIZE (InvalidOids); Index++) {
    Capability.AlgorithmOid     = InvalidOids[Index];
    Capability.AlgorithmOidSize = AsciiStrSize (InvalidOids[Index]);
    EntryData                   = (VOID *)(UINTN)1;
    EntryDataSize               = MAX_UINTN;
    Status                      = EcitEncodeOidSet (&Capability, 1, &EntryData, &EntryDataSize);
    UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    UT_ASSERT_TRUE (EntryData == NULL);
    UT_ASSERT_EQUAL (EntryDataSize, 0);
  }

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestInvalidOutputs (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  VOID        *EntryData;
  UINTN       EntryDataSize;

  EntryDataSize = MAX_UINTN;
  Status        = EcitEncodeOidSet (NULL, 0, NULL, &EntryDataSize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_EQUAL (EntryDataSize, 0);

  EntryData = (VOID *)(UINTN)1;
  Status    = EcitEncodeOidSet (NULL, 0, &EntryData, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (EntryData == NULL);

  return UNIT_TEST_PASSED;
}

INT32
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Suite;

  Framework = NULL;
  Suite     = NULL;

  Status = InitUnitTestFramework (
             &Framework,
             UNIT_TEST_APP_NAME,
             gEfiCallerBaseName,
             UNIT_TEST_APP_VERSION
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = CreateUnitTestSuite (
             &Suite,
             Framework,
             "ECIT OID-set encoding",
             "SecurityPkg.EcitEncodingLib",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  AddTestCase (Suite, "Empty set is eight zero bytes", "Empty", TestEmptyOidSet, NULL, NULL, NULL);
  AddTestCase (Suite, "OIDs are deduplicated and padded", "Encode", TestOidSetEncoding, NULL, NULL, NULL);
  AddTestCase (Suite, "Invalid OID is rejected", "InvalidOid", TestInvalidOid, NULL, NULL, NULL);
  AddTestCase (Suite, "Invalid outputs are rejected", "InvalidOutputs", TestInvalidOutputs, NULL, NULL, NULL);

  Status = RunAllTestSuites (Framework);

Exit:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

int
main (
  int   Argc,
  char  *Argv[]
  )
{
  return UefiTestMain ();
}
