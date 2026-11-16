/** @file
  Unit tests for EcitEncodingLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/EcitEncodingLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME     "EcitEncodingLib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

/**
  Verify that an empty set produces a full alignment unit of zero bytes.

  @param[in] Context  Unused.
  @return Unit test result.
**/
static
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

/**
  Check first-occurrence ordering, CSV termination, and zero-filled padding.

  @param[in] Context  Unused.
  @return Unit test result.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestOidSetEncoding (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  static CONST CHAR8        Oid1[]         = "1.2.840.113549.1.1.11";
  static CONST CHAR8        Oid2[]         = "2.16.840.1.101.3.4.2.1";
  BASE_CRYPT_OP_CAPABILITY  Capabilities[] = {
    { Oid1, sizeof (Oid1) },
    { Oid1, sizeof (Oid1) },
    { Oid2, sizeof (Oid2) }
  };
  static CONST CHAR8        Expected[] =
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

/**
  Verify that invalid OID buffers and arcs are rejected and both outputs cleared.

  @param[in] Context  Unused.
  @return Unit test result.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestInvalidOid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  static CONST BASE_CRYPT_OP_CAPABILITY  InvalidOids[] = {
    { NULL,                      sizeof ("1.2")                     },
    { "",                        0                                  },
    { "1.2",                     sizeof ("1.2") - 1                 },
    { "1..2",                    sizeof ("1..2")                    },
    { "3.1",                     sizeof ("3.1")                     },
    { "1.40",                    sizeof ("1.40")                    },
    { "1.184467440737095516160", sizeof ("1.184467440737095516160") }
  };
  EFI_STATUS                             Status;
  VOID                                   *EntryData;
  UINTN                                  EntryDataSize;
  UINTN                                  Index;

  for (Index = 0; Index < ARRAY_SIZE (InvalidOids); Index++) {
    EntryData     = (VOID *)(UINTN)1;
    EntryDataSize = MAX_UINTN;
    Status        = EcitEncodeOidSet (&InvalidOids[Index], 1, &EntryData, &EntryDataSize);
    UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    UT_ASSERT_TRUE (EntryData == NULL);
    UT_ASSERT_EQUAL (EntryDataSize, 0);
  }

  return UNIT_TEST_PASSED;
}

/**
  Verify that a missing required pointer does not leave any supplied output stale.

  @param[in] Context  Unused.
  @return Unit test result.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestInvalidParameters (
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

  EntryData     = (VOID *)(UINTN)1;
  EntryDataSize = MAX_UINTN;
  Status        = EcitEncodeOidSet (NULL, 1, &EntryData, &EntryDataSize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (EntryData == NULL);
  UT_ASSERT_EQUAL (EntryDataSize, 0);

  return UNIT_TEST_PASSED;
}

/**
  Register and run the ECIT OID-set encoding suite.

  @return Status from framework setup or test execution.
**/
EFI_STATUS
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
  AddTestCase (Suite, "Missing required pointers are rejected", "InvalidOutputs", TestInvalidParameters, NULL, NULL, NULL);

  Status = RunAllTestSuites (Framework);

Exit:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Run the ECIT encoding tests as a host application.

  @param[in] Argc  Unused argument count.
  @param[in] Argv  Unused argument vector.
  @return Test runner status as the process exit code.
**/
int
main (
  int   Argc,
  char  *Argv[]
  )
{
  return (int)UefiTestMain ();
}
