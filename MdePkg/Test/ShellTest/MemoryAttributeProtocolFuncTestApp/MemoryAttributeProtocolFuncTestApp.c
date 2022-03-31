/** @file
TCBZ3519
UEFI Shell based application for unit testing the Memory Attribute Protocol.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
***/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MemoryAttribute.h>

#define UNIT_TEST_APP_NAME     "Memory Attribute Protocol Unit Test Application"
#define UNIT_TEST_APP_VERSION  "0.1"

/**
  Make sure test environment has protocol

  @param        Context
  **/
UNIT_TEST_STATUS
EFIAPI
ProtocolExistsTestCase (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                     Status;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttribute);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (MemoryAttribute);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
GetClearSetAttributesRunningCodeTestCase (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                     Status;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;
  EFI_PHYSICAL_ADDRESS           BaseAddress;
  UINT64                         Length;
  UINT64                         Attributes;
  UINT64                         CachedAttributes;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttribute);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (MemoryAttribute);

  BaseAddress = (UINTN)GetClearSetAttributesRunningCodeTestCase & ~(EFI_PAGE_SIZE - 1);  // Page align address of this page
  Attributes  = 0;
  Length      = EFI_PAGE_SIZE;

  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_LOG_INFO ("Attributes for Memory at base: 0x%llx 0x%llx\n", BaseAddress, Attributes);

  CachedAttributes = Attributes;             // save them for later to set
  UT_ASSERT_NOT_EQUAL (0, CachedAttributes); // If attributes are zero this is unexpected and test will not work

  Status = MemoryAttribute->ClearMemoryAttributes (MemoryAttribute, BaseAddress, Length, CachedAttributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_LOG_INFO ("Attributes after clear for Memory at base: 0x%llX 0x%llx\n", BaseAddress, Attributes);
  UT_ASSERT_NOT_EQUAL (Attributes, CachedAttributes);  // should not be equal since we just cleared them

  Status = MemoryAttribute->SetMemoryAttributes (MemoryAttribute, BaseAddress, Length, CachedAttributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_LOG_INFO ("Attributes after set for Memory at base: 0x%llX 0x%llx\n", BaseAddress, Attributes);
  UT_ASSERT_EQUAL (Attributes, CachedAttributes);  // should be equal since we just cleared them

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
GetAttributesNewBufferEfiLoaderCodeTestCase (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                     Status;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;
  EFI_PHYSICAL_ADDRESS           BaseAddress;
  UINTN                          Length;
  UINT64                         Attributes;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttribute);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (MemoryAttribute);

  Attributes  = 0;
  BaseAddress = 0;
  Length      = EFI_PAGE_SIZE;

  Status = gBS->AllocatePages (AllocateAnyPages, EfiLoaderCode, Length, &BaseAddress);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL ((VOID *)((UINTN)BaseAddress));

  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);
  gBS->FreePages (BaseAddress, Length); // Free the page in case of any failures
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_LOG_INFO ("Attributes: 0x%llx\n", Attributes);

  // newly allocated efi loader code page should be RW-
  UT_ASSERT_TRUE ((Attributes & EFI_MEMORY_XP) == EFI_MEMORY_XP);  // should be No Execute
  UT_ASSERT_FALSE ((Attributes & EFI_MEMORY_RO) == EFI_MEMORY_RO); // Should be not read only

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
GetAttributesRunningCodeTestCase (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                     Status;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;
  EFI_PHYSICAL_ADDRESS           BaseAddress;
  UINT64                         Length;
  UINT64                         Attributes;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttribute);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (MemoryAttribute);

  BaseAddress = (UINTN)GetAttributesRunningCodeTestCase & ~(EFI_PAGE_SIZE - 1);  // Page align address of this page
  Attributes  = 0;
  Length      = EFI_PAGE_SIZE;

  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_LOG_INFO ("Attributes: 0x%llx\n", Attributes);
  // This page is the current executing code page. Should be RO and not XP
  UT_ASSERT_FALSE ((Attributes & EFI_MEMORY_XP) == EFI_MEMORY_XP); // should be allowed to execute
  UT_ASSERT_TRUE ((Attributes & EFI_MEMORY_RO) == EFI_MEMORY_RO);  // Should be read only

  return UNIT_TEST_PASSED;
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      TestSuite;

  Framework = NULL;
  TestSuite = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Test Suite
  //
  Status = CreateUnitTestSuite (&TestSuite, Framework, "Basic", "Dxe.MemoryManagement.MemoryAttributesProtocol", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (TestSuite, "Memory Attributes Protocol Exists", "PrototocolExists", ProtocolExistsTestCase, NULL, NULL, NULL);
  AddTestCase (TestSuite, "Get Clear Set Attributes work as expected", "BasicGetClearSet", GetClearSetAttributesRunningCodeTestCase, ProtocolExistsTestCase, NULL, NULL);
  AddTestCase (TestSuite, "New EfiLoaderCode buffer attributes expected", "NewEfiLoaderCodeAttributes", GetAttributesNewBufferEfiLoaderCodeTestCase, ProtocolExistsTestCase, NULL, NULL);
  AddTestCase (TestSuite, "Loaded Code Attributes Allow Execution", "RunningCodeAttributes", GetAttributesRunningCodeTestCase, ProtocolExistsTestCase, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}
