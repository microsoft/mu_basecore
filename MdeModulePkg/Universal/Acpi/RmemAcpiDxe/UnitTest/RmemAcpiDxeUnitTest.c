/** @file
  Host-based unit tests for the RMEM ACPI publisher.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>

#include "../RmemAcpiDxe.c"

#define UNIT_TEST_APP_NAME     "RMEM ACPI DXE Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

EFI_BOOT_SERVICES               MockBoot;
STATIC EFI_ACPI_TABLE_PROTOCOL  mMockAcpiTableProtocol;
STATIC EFI_STATUS               mLocateProtocolStatus;
STATIC EFI_STATUS               mInstallAcpiTableStatus;
STATIC EFI_STATUS               mInstallProtocolStatus;
STATIC EFI_STATUS               mCreateEventStatus;
STATIC UINTN                    mCloseEventCalls;
STATIC UINTN                    mUninstallProtocolCalls;
STATIC UINTN                    mInstalledTableSize;
STATIC UINT8                    mInstalledTable[sizeof (RMEM_TABLE_HEADER) + sizeof (RMEM_ENTRY)];

STATIC
EFI_STATUS
EFIAPI
MockInstallAcpiTable (
  IN  EFI_ACPI_TABLE_PROTOCOL  *This,
  IN  VOID                     *AcpiTableBuffer,
  IN  UINTN                    AcpiTableBufferSize,
  OUT UINTN                    *TableKey
  )
{
  if (EFI_ERROR (mInstallAcpiTableStatus)) {
    return mInstallAcpiTableStatus;
  }

  if (AcpiTableBufferSize > sizeof (mInstalledTable)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  CopyMem (mInstalledTable, AcpiTableBuffer, AcpiTableBufferSize);
  mInstalledTableSize = AcpiTableBufferSize;
  *TableKey           = 1;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MockLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  )
{
  if (!EFI_ERROR (mLocateProtocolStatus)) {
    *Interface = &mMockAcpiTableProtocol;
  }

  return mLocateProtocolStatus;
}

STATIC
EFI_STATUS
EFIAPI
MockInstallProtocolInterface (
  IN OUT EFI_HANDLE          *Handle,
  IN     EFI_GUID            *Protocol,
  IN     EFI_INTERFACE_TYPE  InterfaceType,
  IN     VOID                *Interface
  )
{
  if (!EFI_ERROR (mInstallProtocolStatus)) {
    *Handle = (EFI_HANDLE)(UINTN)1;
  }

  return mInstallProtocolStatus;
}

STATIC
EFI_STATUS
EFIAPI
MockUninstallProtocolInterface (
  IN EFI_HANDLE  Handle,
  IN EFI_GUID    *Protocol,
  IN VOID        *Interface
  )
{
  mUninstallProtocolCalls++;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MockCreateEventEx (
  IN       UINT32            Type,
  IN       EFI_TPL           NotifyTpl,
  IN       EFI_EVENT_NOTIFY  NotifyFunction OPTIONAL,
  IN CONST VOID              *NotifyContext OPTIONAL,
  IN CONST EFI_GUID          *EventGroup OPTIONAL,
  OUT      EFI_EVENT         *Event
  )
{
  if (!EFI_ERROR (mCreateEventStatus)) {
    *Event = (EFI_EVENT)(UINTN)2;
  }

  return mCreateEventStatus;
}

STATIC
EFI_STATUS
EFIAPI
MockCloseEvent (
  IN EFI_EVENT  Event
  )
{
  mCloseEventCalls++;
  return EFI_SUCCESS;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
ResetRmemState (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  ZeroMem (mEntries, sizeof (mEntries));
  mEntryCount         = 0;
  mFinalized          = FALSE;
  mRegistrationFailed = FALSE;
  mPublicationEvent   = NULL;

  ZeroMem (&MockBoot, sizeof (MockBoot));
  MockBoot.LocateProtocol             = MockLocateProtocol;
  MockBoot.InstallProtocolInterface   = MockInstallProtocolInterface;
  MockBoot.UninstallProtocolInterface = MockUninstallProtocolInterface;
  MockBoot.CreateEventEx              = MockCreateEventEx;
  MockBoot.CloseEvent                 = MockCloseEvent;

  mMockAcpiTableProtocol.InstallAcpiTable = MockInstallAcpiTable;
  mLocateProtocolStatus                  = EFI_SUCCESS;
  mInstallAcpiTableStatus                = EFI_SUCCESS;
  mInstallProtocolStatus                 = EFI_SUCCESS;
  mCreateEventStatus                     = EFI_SUCCESS;
  mCloseEventCalls                       = 0;
  mUninstallProtocolCalls                = 0;
  mInstalledTableSize                    = 0;
  ZeroMem (mInstalledTable, sizeof (mInstalledTable));
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
ValidRangesAreRegistered (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1000,
             0x1000,
             RmemCategorySecurity,
             "Secure"
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (mEntryCount, 1);
  UT_ASSERT_EQUAL (mEntries[0].Base, 0x1000);
  UT_ASSERT_EQUAL (mEntries[0].Size, 0x1000);
  UT_ASSERT_EQUAL (mEntries[0].Category, RmemCategorySecurity);
  UT_ASSERT_EQUAL (AsciiStrCmp (mEntries[0].Label, "Secure"), 0);

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x2000,
             0x1000,
             RmemCategoryFirmwareRuntime,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (mEntryCount, 2);
  UT_ASSERT_EQUAL (mEntries[1].Label[0], '\0');
  UT_ASSERT_FALSE (mRegistrationFailed);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
DuplicateRangesAreIgnored (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1000,
             0x1000,
             RmemCategorySecurity,
             "Secure"
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1000,
             0x1000,
             RmemCategorySecurity,
             "Secure"
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ALREADY_STARTED);
  UT_ASSERT_EQUAL (mEntryCount, 1);
  UT_ASSERT_FALSE (mRegistrationFailed);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
OverlappingRangesAreRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1000,
             0x1000,
             RmemCategorySecurity,
             "Secure"
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1800,
             0x1000,
             RmemCategoryOther,
             "Overlap"
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);
  UT_ASSERT_EQUAL (mEntryCount, 1);
  UT_ASSERT_TRUE (mRegistrationFailed);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
InvalidParametersAreRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EDKII_RMEM_REGISTRATION_PROTOCOL  InvalidProtocol;
  EFI_STATUS                        Status;

  Status = RmemAddReservedRange (
             NULL,
             0x1000,
             0x1000,
             RmemCategorySecurity,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_FALSE (mRegistrationFailed);

  InvalidProtocol = mRmemProtocol;
  InvalidProtocol.Revision++;
  Status = RmemAddReservedRange (
             &InvalidProtocol,
             0x1000,
             0x1000,
             RmemCategorySecurity,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_FALSE (mRegistrationFailed);

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1000,
             0,
             RmemCategorySecurity,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (mRegistrationFailed);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
InvalidRangesAndCategoriesAreRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             MAX_UINT64,
             2,
             RmemCategorySecurity,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  mRegistrationFailed = FALSE;
  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1000,
             0x1000,
             RmemCategoryUnknown,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  mRegistrationFailed = FALSE;
  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1000,
             0x1000,
             RmemCategoryMax,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (mRegistrationFailed);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
OversizedLabelsAreRejected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CHAR8       Label[RMEM_LABEL_MAX_LEN + 1];
  EFI_STATUS  Status;

  SetMem (Label, RMEM_LABEL_MAX_LEN, 'A');
  Label[RMEM_LABEL_MAX_LEN] = '\0';
  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1000,
             0x1000,
             RmemCategoryOther,
             Label
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_BAD_BUFFER_SIZE);
  UT_ASSERT_EQUAL (mEntryCount, 0);
  UT_ASSERT_TRUE (mRegistrationFailed);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
CapacityIsEnforced (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT32      Index;

  for (Index = 0; Index < RMEM_MAX_ENTRIES; Index++) {
    Status = RmemAddReservedRange (
               &mRmemProtocol,
               (UINT64)Index * 0x1000,
               0x1000,
               RmemCategoryOther,
               NULL
               );
    UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  }

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             (UINT64)RMEM_MAX_ENTRIES * 0x1000,
             0x1000,
             RmemCategoryOther,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_OUT_OF_RESOURCES);
  UT_ASSERT_EQUAL (mEntryCount, RMEM_MAX_ENTRIES);
  UT_ASSERT_TRUE (mRegistrationFailed);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
FinalizationPreventsRegistration (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  mFinalized = TRUE;
  Status     = RmemAddReservedRange (
                 &mRmemProtocol,
                 0x1000,
                 0x1000,
                 RmemCategoryOther,
                 NULL
                 );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);
  UT_ASSERT_EQUAL (mEntryCount, 0);
  UT_ASSERT_FALSE (mRegistrationFailed);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TableIsSerializedAndInstalled (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  RMEM_ENTRY         *Entry;
  RMEM_TABLE_HEADER  *Table;
  EFI_STATUS         Status;

  Status = RmemAddReservedRange (
             &mRmemProtocol,
             0x1000,
             0x2000,
             RmemCategorySecurity,
             "Secure"
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = RmemPublishTable ();
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (mInstalledTableSize, sizeof (mInstalledTable));

  Table = (RMEM_TABLE_HEADER *)mInstalledTable;
  Entry = (RMEM_ENTRY *)(mInstalledTable + sizeof (RMEM_TABLE_HEADER));
  UT_ASSERT_EQUAL (Table->Header.Signature, RMEM_TABLE_SIGNATURE);
  UT_ASSERT_EQUAL (Table->Header.Length, sizeof (mInstalledTable));
  UT_ASSERT_EQUAL (Table->Header.Revision, RMEM_TABLE_REVISION);
  UT_ASSERT_EQUAL (Table->EntryCount, 1);
  UT_ASSERT_EQUAL (CalculateSum8 (mInstalledTable, mInstalledTableSize), 0);
  UT_ASSERT_EQUAL (Entry->Base, 0x1000);
  UT_ASSERT_EQUAL (Entry->Size, 0x2000);
  UT_ASSERT_EQUAL (Entry->Category, RmemCategorySecurity);
  UT_ASSERT_EQUAL (AsciiStrCmp (Entry->Label, "Secure"), 0);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
PublicationFailuresAreReturned (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = RmemPublishTable ();
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  mRegistrationFailed = TRUE;
  Status              = RmemPublishTable ();
  UT_ASSERT_STATUS_EQUAL (Status, EFI_COMPROMISED_DATA);

  mRegistrationFailed = FALSE;
  Status              = RmemAddReservedRange (
                          &mRmemProtocol,
                          0x1000,
                          0x1000,
                          RmemCategoryOther,
                          NULL
                          );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  mLocateProtocolStatus = EFI_NOT_FOUND;
  Status                = RmemPublishTable ();
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  mLocateProtocolStatus   = EFI_SUCCESS;
  mInstallAcpiTableStatus = EFI_ACCESS_DENIED;
  Status                  = RmemPublishTable ();
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
PublicationEventFinalizesRegistration (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_EVENT  Event;

  Event = (EFI_EVENT)(UINTN)3;
  RmemOnPublicationEvent (Event, NULL);
  UT_ASSERT_TRUE (mFinalized);
  UT_ASSERT_EQUAL (mCloseEventCalls, 1);
  UT_ASSERT_EQUAL (mPublicationEvent, NULL);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
EntryPointInstallsProtocolAndEvent (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = RmemAcpiDxeEntryPoint (NULL, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (mPublicationEvent, (EFI_EVENT)(UINTN)2);
  UT_ASSERT_EQUAL (mUninstallProtocolCalls, 0);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
EntryPointCleansUpAfterEventFailure (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  mCreateEventStatus = EFI_OUT_OF_RESOURCES;
  Status             = RmemAcpiDxeEntryPoint (NULL, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_OUT_OF_RESOURCES);
  UT_ASSERT_EQUAL (mUninstallProtocolCalls, 1);

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
EntryPointReturnsProtocolInstallFailure (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  mInstallProtocolStatus = EFI_OUT_OF_RESOURCES;
  Status                 = RmemAcpiDxeEntryPoint (NULL, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_OUT_OF_RESOURCES);
  UT_ASSERT_EQUAL (mUninstallProtocolCalls, 0);

  return UNIT_TEST_PASSED;
}

STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      RegistrationTests;

  Framework = NULL;
  Status    = InitUnitTestFramework (
                &Framework,
                UNIT_TEST_APP_NAME,
                gEfiCallerBaseName,
                UNIT_TEST_APP_VERSION
                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CreateUnitTestSuite (
             &RegistrationTests,
             Framework,
             "RMEM registration tests",
             "RmemAcpiDxe.Registration",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return EFI_OUT_OF_RESOURCES;
  }

  AddTestCase (
    RegistrationTests,
    "Valid ranges are registered",
    "Valid",
    ValidRangesAreRegistered,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Duplicate ranges are ignored",
    "Duplicate",
    DuplicateRangesAreIgnored,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Overlapping ranges are rejected",
    "Overlap",
    OverlappingRangesAreRejected,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Invalid parameters are rejected",
    "Parameters",
    InvalidParametersAreRejected,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Invalid ranges and categories are rejected",
    "Validation",
    InvalidRangesAndCategoriesAreRejected,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Oversized labels are rejected",
    "Label",
    OversizedLabelsAreRejected,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Registration capacity is enforced",
    "Capacity",
    CapacityIsEnforced,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Finalization prevents registration",
    "Finalized",
    FinalizationPreventsRegistration,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Table is serialized and installed",
    "Publish",
    TableIsSerializedAndInstalled,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Publication failures are returned",
    "PublishFailure",
    PublicationFailuresAreReturned,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Publication event finalizes registration",
    "Finalize",
    PublicationEventFinalizesRegistration,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Entry point installs protocol and event",
    "EntryPoint",
    EntryPointInstallsProtocolAndEvent,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Entry point cleans up after event failure",
    "EntryPointCleanup",
    EntryPointCleansUpAfterEventFailure,
    ResetRmemState,
    NULL,
    NULL
    );
  AddTestCase (
    RegistrationTests,
    "Entry point returns protocol install failure",
    "EntryPointFailure",
    EntryPointReturnsProtocolInstallFailure,
    ResetRmemState,
    NULL,
    NULL
    );

  Status = RunAllTestSuites (Framework);
  FreeUnitTestFramework (Framework);
  return Status;
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  return UnitTestingEntry ();
}