/** @file
Provides a unit test framework.  This allows tests to focus on testing logic
and the framework to focus on runnings, reporting, statistics, etc. 


Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>

**/

#ifndef __UNIT_TEST_LIB_H__
#define __UNIT_TEST_LIB_H__

/*
Method to Initialize the Unit Test framework

@param Framework - Unit test framework to be created.
@param Title - String name of the framework. String is copied.
@param ShortTitle - Short string name of the framework. String is copied.
@param VersionString - Version string for the framework. String is copied.

@retval Success - Unit Test init.
@retval EFI_ERROR - Unit Tests init failed.  
*/
EFI_STATUS
EFIAPI
InitUnitTestFramework (
  OUT UNIT_TEST_FRAMEWORK   **Framework,
  IN  CHAR16                *Title,
  IN  CHAR16                *ShortTitle,
  IN  CHAR16                *VersionString
  );

/*
Creates Unit Test Suite in the Unit Test Framework

@param  Suite - Suite to create
@param  Framework - Framework to add suite to
@param  Title - String name of the suite. String is copied.
@param  Package - String name of the package. String is copied.
@param  Sup - Setup function, runs before suite.
@param  Tdn - Teardown function, runs after suite.

@retval Success - Unit Test Suite was created.
@retval EFI_OUT_OF_RESOURCES - Unit Test Suite failed to be created.
*/
EFI_STATUS
EFIAPI
CreateUnitTestSuite (
  OUT UNIT_TEST_SUITE           **Suite,
  IN UNIT_TEST_FRAMEWORK        *Framework,
  IN CHAR16                     *Title,
  IN CHAR16                     *Package,
  IN UNIT_TEST_SUITE_SETUP      Sup    OPTIONAL,
  IN UNIT_TEST_SUITE_TEARDOWN   Tdn    OPTIONAL
  );

/*
Adds test case to Suite

@param  Suite - Suite to add test to.
@param  Description - String describing test. String is copied.
@param  ClassName - String name of the test. String is copied.
@param  Func - Test function.
@param  PreReq - Prep function, runs before test.
@param  CleanUp - Clean up function, runs after test.
@param  Context - Pointer to context.

@retval Success - Unit test was added.
@retval EFI_OUT_OF_RESOURCES - Unit test failed to be added.
*/
EFI_STATUS
EFIAPI
AddTestCase (
  IN UNIT_TEST_SUITE      *Suite,
  IN CHAR16               *Description,
  IN CHAR16               *ClassName,
  IN UNIT_TEST_FUNCTION   Func,
  IN UNIT_TEST_PREREQ     PreReq    OPTIONAL,
  IN UNIT_TEST_CLEANUP    CleanUp   OPTIONAL,
  IN UNIT_TEST_CONTEXT    Context   OPTIONAL
  );

EFI_STATUS
EFIAPI
RunAllTestSuites(
  IN UNIT_TEST_FRAMEWORK  *Framework
  );

EFI_STATUS
EFIAPI
FreeUnitTestFramework (
  IN UNIT_TEST_FRAMEWORK  *Framework
  );

EFI_STATUS
EFIAPI
SaveFrameworkState (
  IN UNIT_TEST_FRAMEWORK_HANDLE FrameworkHandle,
  IN UNIT_TEST_CONTEXT          ContextToSave     OPTIONAL,
  IN UINTN                      ContextToSaveSize
  );

EFI_STATUS
EFIAPI
SaveFrameworkStateAndQuit (
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN UNIT_TEST_CONTEXT          ContextToSave     OPTIONAL,
  IN UINTN                      ContextToSaveSize
  );

/**
  NOTE: Takes in a ResetType, but currently only supports EfiResetCold
        and EfiResetWarm. All other types will return EFI_INVALID_PARAMETER.
        If a more specific reset is required, use SaveFrameworkState() and
        call gRT->ResetSystem() directly.

**/
EFI_STATUS
EFIAPI
SaveFrameworkStateAndReboot (
  IN UNIT_TEST_FRAMEWORK_HANDLE FrameworkHandle,
  IN UNIT_TEST_CONTEXT          ContextToSave     OPTIONAL,
  IN UINTN                      ContextToSaveSize,
  IN EFI_RESET_TYPE             ResetType
  );

#endif