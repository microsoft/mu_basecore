/** @file
  Interface-level Unit Tests for PeCoffLib aimed at BasePeCoffLib.
  Hopefully are applicable to any implementation of PeCoffLib

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>

#include <Library/PeCoffLib.h>
#include <IndustryStandard/PeImage.h>

#include <stdio.h>

#include "RngDxe64Efi.h"

#define UNIT_TEST_APP_NAME     "PeCoffLib UnitTest"
#define UNIT_TEST_APP_VERSION  "1.0"

typedef struct _TEST_FILE {
  CONST CHAR8   *FileId;
  // CONST CHAR8   *Filename;
  CONST UINTN   Filesize;
  CONST UINT8   *Filedata;
} TEST_FILE;

TEST_FILE mTestFiles[] = {
  // {"RngDxe.efi", "X64/RngDxe.efi", 0, NULL}
  {"RngDxe.efi", sizeof (gRngDxe64EfiBytes), gRngDxe64EfiBytes}   // Using Bin2H for now, for simplicity.
};
UINTN     mTestFilesCount = sizeof (mTestFiles) / sizeof (mTestFiles[0]);

STATIC
TEST_FILE*
GetTestFile (
  IN CONST CHAR8   *FileId
  )
{
  TEST_FILE   *Result;
  UINTN       Index;

  Result = NULL;
  for (Index = 0; Index < mTestFilesCount; Index++) {
    if (AsciiStrCmp(mTestFiles[Index].FileId, FileId) == 0) {
      Result = &mTestFiles[Index];
      break;
    }
  }

  return Result;
}

STATIC
BOOLEAN
InitTestFiles (
  VOID
  )
{
  BOOLEAN   Result;
  UINTN     Index;

  Result = TRUE;
  for (Index = 0; Index < mTestFilesCount; Index++) {
    if (mTestFiles[Index].Filesize == 0 && mTestFiles[Index].Filedata == NULL) {
      // Attempt to open, allocate, and read.
      // NOTE: Using Bin2H for now, for simplicity.
    }
    // else {
    //   DEBUG ((DEBUG_VERBOSE, "%a - Found previously initialized file! '%a'\n", __FUNCTION__, mTestFiles[Index].FileId));
    // }
  }

  return Result;
}

STATIC
VOID
DeinitTestFiles(
  VOID
  )
{
  UINTN     Index;

  for (Index = 0; Index < mTestFilesCount; Index++) {
    if (mTestFiles[Index].Filesize > 0) {
      // Deallocate file contents.
      // NOTE: Using Bin2H for now, for simplicity.
      // FreePool (mTestFiles[Index].Filedata);
      // mTestFiles[Index].Filesize = 0;
      // mTestFiles[Index].Filedata = NULL;
    }
  }

  return;
}

/**
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
GetImageInfoShouldFailOnNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_TRUE (EFI_ERROR (PeCoffLoaderGetImageInfo (NULL)));
  return UNIT_TEST_PASSED;
}

/**
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
GetImageInfoShouldFailOnZeroBuffer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  PE_COFF_LOADER_IMAGE_CONTEXT    TestContext;
  EFI_IMAGE_DOS_HEADER            DummyHeader;

  ZeroMem (&TestContext, sizeof (TestContext));
  ZeroMem (&DummyHeader, sizeof (DummyHeader));

  // Use the libs read function, for consistency.
  TestContext.ImageRead = PeCoffLoaderImageReadFromMemory;
  TestContext.Handle    = &DummyHeader;

  UT_ASSERT_TRUE (EFI_ERROR (PeCoffLoaderGetImageInfo (&TestContext)));
  return UNIT_TEST_PASSED;
}

/**
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
GetImageInfoShouldFailWithoutImageRead (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  PE_COFF_LOADER_IMAGE_CONTEXT    TestContext;
  TEST_FILE                       *TestFile;

  UT_ASSERT_NOT_NULL (Context);
  TestFile = (TEST_FILE*)Context;

  ZeroMem (&TestContext, sizeof (TestContext));

  // Set an invalid ImageRead function.
  TestContext.ImageRead = NULL;
  TestContext.Handle    = (VOID*)TestFile->Filedata;

  UT_ASSERT_TRUE (EFI_ERROR (PeCoffLoaderGetImageInfo (&TestContext)));
  return UNIT_TEST_PASSED;
}

/**
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
GetImageInfoShouldPassOnLegitPeImage (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  PE_COFF_LOADER_IMAGE_CONTEXT    TestContext;
  TEST_FILE                       *TestFile;

  UT_ASSERT_NOT_NULL (Context);
  TestFile = (TEST_FILE*)Context;

  ZeroMem (&TestContext, sizeof (TestContext));

  // Use the libs read function, for consistency.
  TestContext.ImageRead = PeCoffLoaderImageReadFromMemory;
  TestContext.Handle    = (VOID*)TestFile->Filedata;

  UT_ASSERT_NOT_EFI_ERROR (PeCoffLoaderGetImageInfo (&TestContext));
  return UNIT_TEST_PASSED;
}

/**
  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
GetImageInfoShouldPopulateHeaderMetadata (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  PE_COFF_LOADER_IMAGE_CONTEXT    TestContext;
  TEST_FILE                       *TestFile;

  UT_ASSERT_NOT_NULL (Context);
  TestFile = (TEST_FILE*)Context;

  ZeroMem (&TestContext, sizeof (TestContext));

  // Use the libs read function, for consistency.
  TestContext.ImageRead = PeCoffLoaderImageReadFromMemory;
  TestContext.Handle    = (VOID*)TestFile->Filedata;

  PeCoffLoaderGetImageInfo (&TestContext);

  // Check for the data we expect.
  // This is specific to the 64-Bit RngDxe.efi test binary.
  UT_ASSERT_EQUAL (TestContext.ImageAddress, 0);
  UT_ASSERT_EQUAL (TestContext.ImageSize, TestFile->Filesize);

  UT_ASSERT_EQUAL (TestContext.DestinationAddress, 0);
  UT_ASSERT_EQUAL (TestContext.EntryPoint, 0);
  UT_ASSERT_FALSE (TestContext.RelocationsStripped);
  UT_ASSERT_FALSE (TestContext.IsTeImage);

  UT_ASSERT_EQUAL (TestContext.Machine, IMAGE_FILE_MACHINE_X64);
  UT_ASSERT_EQUAL (TestContext.ImageType, EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER);

  return UNIT_TEST_PASSED;
}

/**
  Initialze the unit test framework, suite, and unit tests for the
  Base64 conversion APIs of BaseLib and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw;
  UNIT_TEST_SUITE_HANDLE      GetImageInfoSuite;

  Fw = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  InitTestFiles ();

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
      goto EXIT;
  }

  //
  // Populate the GetImageInfo Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&GetImageInfoSuite, Fw, "PeCoffLoaderGetImageInfo Tests", "PeCoffLib.GetImageInfo", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for GetImageInfo\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // --------------Suite-----------Description--------------Class Name----------Function--------Pre---Post-------------------Context-----------
  AddTestCase (GetImageInfoSuite, "GetImageInfoShouldFailOnNull", "FailOnNull", GetImageInfoShouldFailOnNull, NULL, NULL, NULL);
  AddTestCase (GetImageInfoSuite, "GetImageInfoShouldFailOnZeroBuffer", "FailOnEmptyBuffer", GetImageInfoShouldFailOnZeroBuffer, NULL, NULL, NULL);
  AddTestCase (GetImageInfoSuite, "GetImageInfoShouldFailWithoutImageRead", "FailWithoutImageRead", GetImageInfoShouldFailWithoutImageRead, NULL, NULL, GetTestFile ("RngDxe.efi"));
  AddTestCase (GetImageInfoSuite, "GetImageInfoShouldPassOnLegitPeImage", "PassOnLegitImage", GetImageInfoShouldPassOnLegitPeImage, NULL, NULL, GetTestFile ("RngDxe.efi"));
  AddTestCase (GetImageInfoSuite, "GetImageInfoShouldPopulateHeaderMetadata", "PopulateMetadata", GetImageInfoShouldPopulateHeaderMetadata, NULL, NULL, GetTestFile ("RngDxe.efi"));

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Fw);

EXIT:
  if (Fw) {
    FreeUnitTestFramework (Fw);
  }

  DeinitTestFiles ();

  return Status;
}

/**
  Standard UEFI entry point for target based unit test execution from UEFI Shell.
**/
EFI_STATUS
EFIAPI
BaseLibUnitTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UnitTestingEntry ();
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int argc,
  char *argv[]
  )
{
  return UnitTestingEntry ();
}
