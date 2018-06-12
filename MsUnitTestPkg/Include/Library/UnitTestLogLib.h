/** @file
Provides a unit test framework logging.  This allows tests to focus on testing logic
and the library to handle unit test specific logging. 


Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>

**/

#ifndef __UNIT_TEST_LOG_LIB_H__
#define __UNIT_TEST_LOG_LIB_H__



///================================================================================================
///================================================================================================
///
/// UNIT TEST LOGGING DEFINITIONS AND FUNCTIONS
///
///================================================================================================
///================================================================================================


// IMPORTANT NOTE: These macros should ONLY be used in a Unit Test.
//                 They will consume the Framework Handle and update the Framework->CurrentTest.

#define UT_LOG_ERROR(Format, ...)              \
  UnitTestLog( Framework, DEBUG_ERROR, Format, __VA_ARGS__ );
#define UT_LOG_WARNING(Format, ...)            \
  UnitTestLog( Framework, DEBUG_WARN, Format, __VA_ARGS__ );
#define UT_LOG_INFO(Format, ...)               \
  UnitTestLog( Framework, DEBUG_INFO, Format, __VA_ARGS__ );
#define UT_LOG_VERBOSE(Format, ...)            \
  UnitTestLog( Framework, DEBUG_VERBOSE, Format, __VA_ARGS__ );

VOID
EFIAPI
UnitTestLog (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN  UINTN                       ErrorLevel,
  IN  CONST CHAR8                 *Format,
  ...
  );

VOID
EFIAPI
UnitTestLogInit (
IN OUT UNIT_TEST  *Test,
IN UINT8      *Buffer OPTIONAL,
IN UINTN      BufferSize
);

VOID
EFIAPI
UnitTestLogFailure(
  IN  UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  FAILURE_TYPE FailureType,
  IN  CONST CHAR8                 *Format,
  ...
);

#endif