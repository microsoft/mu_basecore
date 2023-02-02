/** @file
TCBZ3519
UEFI Shell based application for unit testing the Memory Attribute Protocol.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
***/

#define UNIT_TEST_APP_NAME     "Memory Attribute Protocol Unit Test Application"
#define UNIT_TEST_APP_VERSION  "0.2"

#include "MemoryAttributeProtocolFuncTestApp.h"

/**
  Ensure this image is protected.

**/
EFI_STATUS
STATIC
IsThisImageProtected (
  VOID
  )
{
  EFI_STATUS                     Status;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;
  EFI_PHYSICAL_ADDRESS           BaseAddress;
  UINT64                         Length;
  UINT64                         Attributes;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttribute);

  if (EFI_ERROR (Status)) {
    return RETURN_UNSUPPORTED;
  }

  BaseAddress = (UINTN)IsThisImageProtected & ~(EFI_PAGE_SIZE - 1);
  Attributes  = 0;
  Length      = EFI_PAGE_SIZE;

  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);

  if (EFI_ERROR (Status) || (Attributes == 0)) {
    return RETURN_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Allocate a page, set the page to read-only, no-execute, and read-protected, then free the page to
  test if the free process clears those attributes before attempting to free the memory.

  @param[in] Context      Unit test context

**/
UNIT_TEST_STATUS
EFIAPI
FreePagesWithProtectionAttributesTestCase (
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

  // Allocate any pages
  Status = gBS->AllocatePages (AllocateAnyPages, EfiLoaderCode, Length, &BaseAddress);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL ((VOID *)((UINTN)BaseAddress));

  Status = MemoryAttribute->SetMemoryAttributes (MemoryAttribute, BaseAddress, Length, EFI_MEMORY_RP | EFI_MEMORY_XP | EFI_MEMORY_RO);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Get the attributes of allocated pages
  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "%a - Attributes for memory at base: 0x%llx 0x%llx\n", __FUNCTION__, BaseAddress, Attributes));

  // Free the pages
  gBS->FreePages (BaseAddress, Length);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Allocate a pool, set the page table entry managing that pool to read-only, no-execute,
  and read-protected, then free the pool to test if the free process clears those
  attributes before attempting to free the pool.

  @param[in] Context      Unit test context

**/
UNIT_TEST_STATUS
EFIAPI
FreePoolWithProtectionAttributesTestCase (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                     Status;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;
  EFI_PHYSICAL_ADDRESS           BaseAddress;
  UINTN                          Length;
  UINT64                         Attributes;
  VOID                           *Buffer;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttribute);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (MemoryAttribute);

  Attributes  = 0;
  BaseAddress = 0;
  Length      = EFI_PAGE_SIZE;

  // Allocate a pool
  Status = gBS->AllocatePool (EfiLoaderCode, Length, &Buffer);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (Buffer);

  BaseAddress = ((EFI_PHYSICAL_ADDRESS)((UINTN)ALIGN_POINTER (Buffer, EFI_PAGE_SIZE))) - EFI_PAGE_SIZE;

  // Get the attributes of the pages representing the allocated pool
  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = MemoryAttribute->SetMemoryAttributes (MemoryAttribute, BaseAddress, Length, EFI_MEMORY_RP | EFI_MEMORY_XP | EFI_MEMORY_RO);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Free the pool
  gBS->FreePool (Buffer);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Allocate a page, free that page, then reallocate a page specifiying the previously
  allocated address to ensure that the attributes are as expected

  @param[in] Context      Unit test context

**/
UNIT_TEST_STATUS
EFIAPI
AllocateFreeAllocateAtAddressTestCase (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                     Status;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;
  EFI_PHYSICAL_ADDRESS           BaseAddress;
  UINTN                          Length;
  UINT64                         Attributes;
  UINT64                         CachedAttributes;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttribute);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (MemoryAttribute);

  Attributes  = 0;
  BaseAddress = 0;
  Length      = EFI_PAGE_SIZE;

  // Allocate any pages
  Status = gBS->AllocatePages (AllocateAnyPages, EfiLoaderCode, Length, &BaseAddress);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL ((VOID *)((UINTN)BaseAddress));

  // Get the attributes of allocated pages
  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_LOG_INFO ("%a - Attributes for memory at base: 0x%llx 0x%llx\n", __FUNCTION__, BaseAddress, Attributes);

  CachedAttributes = Attributes;

  // Flip all the attributes of the page range
  if ((CachedAttributes & EFI_MEMORY_XP) != 0) {
    Status = MemoryAttribute->ClearMemoryAttributes (MemoryAttribute, BaseAddress, Length, EFI_MEMORY_XP);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  } else {
    Status = MemoryAttribute->SetMemoryAttributes (MemoryAttribute, BaseAddress, Length, EFI_MEMORY_XP);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  if ((CachedAttributes & EFI_MEMORY_RO) != 0) {
    Status = MemoryAttribute->ClearMemoryAttributes (MemoryAttribute, BaseAddress, Length, EFI_MEMORY_RO);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  } else {
    Status = MemoryAttribute->SetMemoryAttributes (MemoryAttribute, BaseAddress, Length, EFI_MEMORY_RO);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  if ((CachedAttributes & EFI_MEMORY_RP) != 0) {
    Status = MemoryAttribute->ClearMemoryAttributes (MemoryAttribute, BaseAddress, Length, EFI_MEMORY_RP);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  } else {
    Status = MemoryAttribute->SetMemoryAttributes (MemoryAttribute, BaseAddress, Length, EFI_MEMORY_RP);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  // Free the pages
  gBS->FreePages (BaseAddress, Length);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Allocate pages at the previously allocated address
  Status = gBS->AllocatePages (AllocateAddress, EfiLoaderCode, Length, &BaseAddress);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL ((VOID *)((UINTN)BaseAddress));

  // Get the attributes of allocated pages and check them against the cached attributes
  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, Length, &Attributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (Attributes, CachedAttributes);

  UT_LOG_INFO ("%a - Attributes for memory after reallocation at base: 0x%llx 0x%llx\n", __FUNCTION__, BaseAddress, Attributes);

  // Free the pages
  gBS->FreePages (BaseAddress, Length);

  return UNIT_TEST_PASSED;
}

/**
  Allocate an unsplit page range, clear the no-execute attribute from a subsection of that region
  to force a split, then ensure the previously unsplit region has also had the no-execute attribute
  cleared.

  @param[in] Context      Unit test context

**/
UNIT_TEST_STATUS
EFIAPI
UpdateAttributesRequiresPageSplitTestCase (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                     Status;
  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttribute;
  EFI_PHYSICAL_ADDRESS           BaseAddress;
  UINT64                         Attributes;

  Status = GetUnsplitPageTableEntry (&BaseAddress);
  UT_ASSERT_NOT_EQUAL (BaseAddress, 0);

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttribute);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (MemoryAttribute);

  // Get the attributes of allocated 2MB page
  Status = MemoryAttribute->GetMemoryAttributes (MemoryAttribute, BaseAddress, PTE2MB, &Attributes);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_LOG_INFO ("Attributes for 2MB page at address: 0x%llx 0x%llx\n", BaseAddress, Attributes);

  if ((Attributes & EFI_MEMORY_XP) == 0) {
    // Set the 2MB region to NX
    Status = MemoryAttribute->SetMemoryAttributes (MemoryAttribute, BaseAddress, PTE2MB, EFI_MEMORY_XP);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  // Set subsection of 2MB region to executable
  Status = MemoryAttribute->ClearMemoryAttributes (MemoryAttribute, BaseAddress + EFI_PAGE_SIZE, EFI_PAGE_SIZE, EFI_MEMORY_XP);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Check that the 2MB page is no longer NX
  UT_ASSERT_EQUAL (GetSpitPageTableEntryNoExecute (BaseAddress), 0);

  // Free the pages
  gBS->FreePages (BaseAddress, EFI_SIZE_TO_PAGES (PTE2MB));

  return UNIT_TEST_PASSED;
}

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

/**
  Fetch the attributes, clear them, then set them back to their orignal value

  @param[in] Context      Unit test context

**/
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

/**
  Check that EfiLoaderCode is allocated as no-execute and read/write

  @param[in] Context      Unit test context

**/
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

/**
  Check that code region of image is set as read-only and executable

  @param[in] Context      Unit test context

**/
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

  Status = IsThisImageProtected ();

  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

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
  AddTestCase (TestSuite, "Allocate, free, then reallocate at the previous address", "AllocateFreeAllocateAtAddress", AllocateFreeAllocateAtAddressTestCase, ProtocolExistsTestCase, NULL, NULL);
  AddTestCase (TestSuite, "Ensure higher level pages have loose page attributes after split", "UpdateAttributesRequiresPageSplit", UpdateAttributesRequiresPageSplitTestCase, ProtocolExistsTestCase, NULL, NULL);
  AddTestCase (TestSuite, "Pages with protection attributes set can still be freed", "FreePagesWithProtectionAttributes", FreePagesWithProtectionAttributesTestCase, ProtocolExistsTestCase, NULL, NULL);

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
