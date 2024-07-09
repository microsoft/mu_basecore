/** @file
  Host based unit tests for the PEI policy service module.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiMultiPhase.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <PolicyInterface.h>
#include <PolicyPei.h>

#define UNIT_TEST_NAME     "PEI Policy Service Unit Test"
#define UNIT_TEST_VERSION  "1.0"

//
// Internal tracking.
//

typedef struct _NOTIFICATION_TRACKER {
  EFI_GUID    Guid;
  UINT32      Events;
  UINT32      Priority;
} NOTIFICATION_TRACKER;

NOTIFICATION_TRACKER  Notifications[150]      = { 0 };
UINT32                NotificationsCount      =  0;
UINT32                NotifyCountUpdateRemove =  0;

extern UINT32                 HobByteIndex;
extern PEI_POLICY_NOTIFY_HOB  *CurrentPolicyList;

/**
  Cleans up the policy and test state.

  @param[in]  Context                     Unused.
**/
VOID
EFIAPI
PolicyServiceCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  ZeroMem (&Notifications[0], sizeof (Notifications));
  NotificationsCount      = 0;
  NotifyCountUpdateRemove = 0;
  HobByteIndex            = 0;
}

/**
  Callback for a policy notification event. This logs information about the
  callback event for testing purposes.

  @param[in]  PolicyGuid        The GUID of the policy being notified.
  @param[in]  EventTypes        The events that occurred for the notification.
  @param[in]  CallbackHandle    The handle for the callback being invoked.
**/
VOID
EFIAPI
GenericNotify (
  IN CONST EFI_GUID  *PolicyGuid,
  IN UINT32          EventTypes,
  IN VOID            *CallbackHandle
  )
{
  VOID        *Policy;
  EFI_STATUS  Status;

  ASSERT (CurrentPolicyList->Index == NOTIFY_HANDLE_HOB_INDEX (CallbackHandle));

  Notifications[NotificationsCount].Guid     = *PolicyGuid;
  Notifications[NotificationsCount].Events   = EventTypes;
  Notifications[NotificationsCount].Priority = CurrentPolicyList->Entries[NOTIFY_HANDLE_ENTRY_INDEX (CallbackHandle)].Priority;
  NotificationsCount++;

  //
  // For tests where we want to update the policy during a notification. First,
  // remove this entry in the notification list to avoid infinite loops.
  //

  if (NotifyCountUpdateRemove == NotificationsCount) {
    Status = PeiUnregisterNotify (CallbackHandle);
    ASSERT (!EFI_ERROR (Status));
    Policy = AllocatePool (10);
    ASSERT (Policy != NULL);
    Status = PeiSetPolicy (PolicyGuid, POLICY_ATTRIBUTE_FINALIZED, Policy, 10);
    ASSERT (!EFI_ERROR (Status));
  }
}

/**
  Tests the basics of the notify callbacks.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
SimpleNotifyTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  VOID        *Handle;
  VOID        *Policy;
  EFI_GUID    TestGuid0 = {
    0x93c7693d, 0xc072, 0x4b98, { 0xb1, 0x83, 0x18, 0x3d, 0xfe, 0xd5, 0x90, 0x5a }
  };
  EFI_GUID    TestGuid1 = {
    0xba63f0a3, 0x5058, 0x4daf, { 0x9e, 0x12, 0x54, 0x03, 0x81, 0xa1, 0x59, 0x6c }
  };
  EFI_GUID    TestGuid2 = {
    0xd122d807, 0xdacd, 0x4586, { 0x93, 0xa8, 0xa7, 0xc8, 0x99, 0x13, 0xe4, 0x82 }
  };

  //
  // Setup a basic notify.
  //

  Status = PeiRegisterNotify (
             &TestGuid0,
             POLICY_NOTIFY_ALL,
             POLICY_NOTIFY_DEFAULT_PRIORITY,
             GenericNotify,
             &Handle
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = PeiRegisterNotify (
             &TestGuid1,
             POLICY_NOTIFY_ALL,
             POLICY_NOTIFY_DEFAULT_PRIORITY,
             GenericNotify,
             &Handle
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = PeiRegisterNotify (
             &TestGuid2,
             POLICY_NOTIFY_ALL,
             POLICY_NOTIFY_DEFAULT_PRIORITY,
             GenericNotify,
             &Handle
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_EQUAL (NotificationsCount, 0);

  Policy = AllocatePool (10);
  UT_ASSERT_NOT_NULL (Policy);
  Status = PeiSetPolicy (&TestGuid0, 0, Policy, 10);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_EQUAL (NotificationsCount, 1);
  UT_ASSERT_MEM_EQUAL (&Notifications[0].Guid, &TestGuid0, sizeof (EFI_GUID));
  UT_ASSERT_EQUAL (Notifications[0].Events, POLICY_NOTIFY_SET);

  Policy = AllocatePool (10);
  UT_ASSERT_NOT_NULL (Policy);
  Status = PeiSetPolicy (&TestGuid1, POLICY_ATTRIBUTE_FINALIZED, Policy, 10);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_EQUAL (NotificationsCount, 2);
  UT_ASSERT_MEM_EQUAL (&Notifications[1].Guid, &TestGuid1, sizeof (EFI_GUID));
  UT_ASSERT_EQUAL (Notifications[1].Events, POLICY_NOTIFY_SET | POLICY_NOTIFY_FINALIZED);

  Policy = AllocatePool (10);
  UT_ASSERT_NOT_NULL (Policy);
  Status = PeiSetPolicy (&TestGuid2, 0, Policy, 10);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_EQUAL (NotificationsCount, 3);
  UT_ASSERT_MEM_EQUAL (&Notifications[2].Guid, &TestGuid2, sizeof (EFI_GUID));
  UT_ASSERT_EQUAL (Notifications[2].Events, POLICY_NOTIFY_SET);

  //
  // Remove a policy
  //

  Status = PeiRemovePolicy (&TestGuid2);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_EQUAL (NotificationsCount, 4);
  UT_ASSERT_MEM_EQUAL (&Notifications[3].Guid, &TestGuid2, sizeof (EFI_GUID));
  UT_ASSERT_EQUAL (Notifications[3].Events, POLICY_NOTIFY_REMOVED);

  return UNIT_TEST_PASSED;
}

/**
  Tests the priority mechanism of the notify callbacks.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
NotifyPriorityTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  VOID        *Handle;
  VOID        *Policy;
  EFI_GUID    TestGuid = {
    0xf0192692, 0xd698, 0x4955, { 0xaf, 0xa4, 0xd5, 0xb0, 0xed, 0xa1, 0x38, 0x13 }
  };

  Status = PeiRegisterNotify (
             &TestGuid,
             POLICY_NOTIFY_ALL,
             POLICY_NOTIFY_DEFAULT_PRIORITY,
             GenericNotify,
             &Handle
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = PeiRegisterNotify (
             &TestGuid,
             POLICY_NOTIFY_ALL,
             POLICY_NOTIFY_DEFAULT_PRIORITY - 1,
             GenericNotify,
             &Handle
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = PeiRegisterNotify (
             &TestGuid,
             POLICY_NOTIFY_ALL,
             POLICY_NOTIFY_DEFAULT_PRIORITY + 1,
             GenericNotify,
             &Handle
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  Policy = AllocatePool (10);
  UT_ASSERT_NOT_NULL (Policy);
  Status = PeiSetPolicy (&TestGuid, 0, Policy, 10);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_EQUAL (NotificationsCount, 3);
  UT_ASSERT_MEM_EQUAL (&Notifications[0].Guid, &TestGuid, sizeof (EFI_GUID));
  UT_ASSERT_MEM_EQUAL (&Notifications[1].Guid, &TestGuid, sizeof (EFI_GUID));
  UT_ASSERT_MEM_EQUAL (&Notifications[2].Guid, &TestGuid, sizeof (EFI_GUID));
  UT_ASSERT_EQUAL (Notifications[0].Events, POLICY_NOTIFY_SET);
  UT_ASSERT_EQUAL (Notifications[1].Events, POLICY_NOTIFY_SET);
  UT_ASSERT_EQUAL (Notifications[2].Events, POLICY_NOTIFY_SET);
  UT_ASSERT_EQUAL (Notifications[0].Priority, POLICY_NOTIFY_DEFAULT_PRIORITY - 1);
  UT_ASSERT_EQUAL (Notifications[1].Priority, POLICY_NOTIFY_DEFAULT_PRIORITY);
  UT_ASSERT_EQUAL (Notifications[2].Priority, POLICY_NOTIFY_DEFAULT_PRIORITY + 1);

  return UNIT_TEST_PASSED;
}

/**
  Tests editing the policy in a callback.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
EditingNotifyTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  VOID        *Handle;
  VOID        *Policy;
  EFI_GUID    TestGuid = {
    0xd71e9bc1, 0x56bf, 0x4ed1, { 0xaf, 0x1c, 0x52, 0x67, 0xf1, 0xfe, 0xed, 0xce }
  };

  Status = PeiRegisterNotify (
             &TestGuid,
             POLICY_NOTIFY_ALL,
             POLICY_NOTIFY_DEFAULT_PRIORITY,
             GenericNotify,
             &Handle
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = PeiRegisterNotify (
             &TestGuid,
             POLICY_NOTIFY_ALL,
             POLICY_NOTIFY_DEFAULT_PRIORITY - 1,
             GenericNotify,
             &Handle
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = PeiRegisterNotify (
             &TestGuid,
             POLICY_NOTIFY_ALL,
             POLICY_NOTIFY_DEFAULT_PRIORITY + 1,
             GenericNotify,
             &Handle
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  NotifyCountUpdateRemove = 2;

  Policy = AllocatePool (10);
  UT_ASSERT_NOT_NULL (Policy);
  Status = PeiSetPolicy (&TestGuid, 0, Policy, 10);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_EQUAL (NotificationsCount, 4);
  UT_ASSERT_MEM_EQUAL (&Notifications[0].Guid, &TestGuid, sizeof (EFI_GUID));
  UT_ASSERT_MEM_EQUAL (&Notifications[1].Guid, &TestGuid, sizeof (EFI_GUID));
  UT_ASSERT_MEM_EQUAL (&Notifications[2].Guid, &TestGuid, sizeof (EFI_GUID));
  UT_ASSERT_MEM_EQUAL (&Notifications[3].Guid, &TestGuid, sizeof (EFI_GUID));

  UT_ASSERT_EQUAL (Notifications[0].Priority, POLICY_NOTIFY_DEFAULT_PRIORITY - 1);
  UT_ASSERT_EQUAL (Notifications[0].Events, POLICY_NOTIFY_SET);

  UT_ASSERT_EQUAL (Notifications[1].Priority, POLICY_NOTIFY_DEFAULT_PRIORITY);
  UT_ASSERT_EQUAL (Notifications[1].Events, POLICY_NOTIFY_SET);

  UT_ASSERT_EQUAL (Notifications[2].Events, POLICY_NOTIFY_SET | POLICY_NOTIFY_FINALIZED);
  UT_ASSERT_EQUAL (Notifications[2].Priority, POLICY_NOTIFY_DEFAULT_PRIORITY - 1);

  UT_ASSERT_EQUAL (Notifications[3].Events, POLICY_NOTIFY_SET | POLICY_NOTIFY_FINALIZED);
  UT_ASSERT_EQUAL (Notifications[3].Priority, POLICY_NOTIFY_DEFAULT_PRIORITY + 1);

  return UNIT_TEST_PASSED;
}

/**
  Tests a large number of callbacks to test the multi-hob scenarios.

  @param[in]  Context                     Unused.

  @retval   UNIT_TEST_PASSED              Test passed.
  @retval   UNIT_TEST_ERROR_TEST_FAILED   Test failed.
**/
UNIT_TEST_STATUS
EFIAPI
LargeNumberNotifyTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS    Status;
  VOID          *Handle;
  VOID          *Policy;
  UINT32        Index;
  CONST UINT32  Count    = 140;
  EFI_GUID      TestGuid = {
    0x455d8ad7, 0xe730, 0x409b, { 0xbd, 0xaa, 0xcb, 0x52, 0xaf, 0x0a, 0x8b, 0x9e }
  };

  for (Index = 0; Index < Count; Index++) {
    Status = PeiRegisterNotify (
               &TestGuid,
               POLICY_NOTIFY_ALL,
               Count - Index,
               GenericNotify,
               &Handle
               );

    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  Policy = AllocatePool (10);
  UT_ASSERT_NOT_NULL (Policy);
  Status = PeiSetPolicy (&TestGuid, 0, Policy, 10);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  UT_ASSERT_EQUAL (NotificationsCount, Count);
  for (Index = 0; Index < Count; Index++) {
    UT_ASSERT_MEM_EQUAL (&Notifications[Index].Guid, &TestGuid, sizeof (EFI_GUID));
    UT_ASSERT_EQUAL (Notifications[Index].Priority, Index + 1);
    UT_ASSERT_EQUAL (Notifications[Index].Events, POLICY_NOTIFY_SET);
  }

  return UNIT_TEST_PASSED;
}

/**
  Initialize the unit test framework, suite, and unit tests for the
  sample unit tests and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      PolicyNotifyTests;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the PolicyNotifyTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&PolicyNotifyTests, Framework, "Policy Notification Tests", "PeiPolicy.Notify", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for PolicyNotifyTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (PolicyNotifyTests, "Test of basic policy notifications", "SimpleNotifyTest", SimpleNotifyTest, NULL, PolicyServiceCleanup, NULL);
  AddTestCase (PolicyNotifyTests, "Test of policy priority", "NotifyPriorityTest", NotifyPriorityTest, NULL, PolicyServiceCleanup, NULL);
  AddTestCase (PolicyNotifyTests, "Tests more complex use cases of notifications", "EditingNotifyTest", EditingNotifyTest, NULL, PolicyServiceCleanup, NULL);
  AddTestCase (PolicyNotifyTests, "Tests a large number of notifications", "LargeNumberNotifyTest", LargeNumberNotifyTest, NULL, PolicyServiceCleanup, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UefiTestMain ();
}
