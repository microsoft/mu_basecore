/**
  Implemnet UnitTestLib log services

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <stdlib.h>
#include <PiDxe.h>
#include <UnitTestFrameworkTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>

#define UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH  (512)
#define UNIT_TEST_MAX_LOG_BUFFER                SIZE_16KB

struct _UNIT_TEST_LOG_PREFIX_STRING {
  UNIT_TEST_STATUS    LogLevel;
  CHAR8               *String;
};

struct _UNIT_TEST_LOG_PREFIX_STRING  mLogPrefixStrings[] = {
  { UNIT_TEST_LOG_LEVEL_ERROR,   "[ERROR]       " },
  { UNIT_TEST_LOG_LEVEL_WARN,    "[WARNING]     " },
  { UNIT_TEST_LOG_LEVEL_INFO,    "[INFO]        " },
  { UNIT_TEST_LOG_LEVEL_VERBOSE, "[VERBOSE]     " }
};

//
// Function to handle the printing to stderr channel for Google Test
//
void _assert_true(BOOLEAN expression, const char* assertion, const char* file, unsigned int line) {
    if (!expression) {
      fprintf(stderr, "Assertion failed: %s, file %s, line %u.\n", assertion, file, line);
      abort();
    }
}

//
// Unit-Test Log helper functions
//

STATIC
CONST CHAR8 *
GetStringForStatusLogPrefix (
  IN UINTN  LogLevel
  )
{
  UINTN  Index;
  CHAR8  *Result;

  Result = NULL;
  for (Index = 0; Index < ARRAY_SIZE (mLogPrefixStrings); Index++) {
    if (mLogPrefixStrings[Index].LogLevel == LogLevel) {
      Result = mLogPrefixStrings[Index].String;
      break;
    }
  }

  return Result;
}

/**
  Test logging function that records a messages in the test framework log.
  Record is associated with the currently executing test case.

  @param[in]  ErrorLevel  The error level of the unit test log message.
  @param[in]  Format      Formatting string following the format defined in the
                          MdePkg/Include/Library/PrintLib.h.
  @param[in]  ...         Print args.
**/
VOID
EFIAPI
UnitTestLog (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  )
{
  CHAR8                       NewFormatString[UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH];
  CHAR8                       LogString[UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH];
  CONST CHAR8                 *LogTypePrefix;
  VA_LIST                     Marker;

  LogTypePrefix = NULL;

  //
  // Make sure that this unit test log level is enabled.
  //
  if ((ErrorLevel & (UINTN)PcdGet32 (PcdUnitTestLogLevel)) == 0) {
    return;
  }

  //
  // If we need to define a new format string...
  // well... get to it.
  //
  LogTypePrefix = GetStringForStatusLogPrefix (ErrorLevel);
  if (LogTypePrefix != NULL) {
    AsciiSPrint (NewFormatString, sizeof (NewFormatString), "%a%a", LogTypePrefix, Format);
  } else {
    AsciiStrCpyS (NewFormatString, sizeof (NewFormatString), Format);
  }

  //
  // Convert the message to an ASCII String
  //
  VA_START (Marker, Format);
  AsciiVSPrint (LogString, sizeof (LogString), NewFormatString, Marker);
  VA_END (Marker);

  //
  // Finally, add the string to the log.
  //
  printf(LogString);
}
