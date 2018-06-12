/**

Implement UnitTestLogLib - Unit test debugging log

Copyright (c) Microsoft
**/

#include <Uefi.h>
#include <UnitTestTypes.h>
#include <Library/UnitTestLogLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>

#define UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH    (512)
#define UNIT_TEST_MAX_LOG_BUFFER (16 * 1024)



struct _UNIT_TEST_LOG_PREFIX_STRING
{
  UNIT_TEST_STATUS    LogLevel;
  CHAR8               *String;
};

struct _UNIT_TEST_LOG_PREFIX_STRING   mLogPrefixStrings[] =
{
  { DEBUG_ERROR,    "[ERROR]       " },
  { DEBUG_WARN,     "[WARNING]     " },
  { DEBUG_INFO,     "[INFO]        " },
  { DEBUG_VERBOSE,  "[VERBOSE]     " }
};
UINTN mLogPrefixStringsCount = sizeof( mLogPrefixStrings ) / sizeof( mLogPrefixStrings[0] );




//=============================================================================
//
// ----------------  TEST HELPER FUNCTIONS ------------------------------------
//
//=============================================================================

STATIC
CONST CHAR8*
GetStringForStatusLogPrefix (
  IN UINTN      LogLevel
  )
{
  UINTN   Index;
  CHAR8   *Result = NULL;

  for (Index = 0; Index < mLogPrefixStringsCount; Index++)
  {
    if (mLogPrefixStrings[Index].LogLevel == LogLevel)
    {
      Result = mLogPrefixStrings[Index].String;
      break;
    }
  }

  return Result;
}



STATIC
EFI_STATUS
AddStringToUnitTestLog (
  IN OUT UNIT_TEST    *UnitTest,
  IN CONST CHAR16     *String
  )
{
  EFI_STATUS  Status;

  //
  // Make sure that you're cooking with gas.
  //
  if (UnitTest == NULL || String == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  // If this is the first log for the test allocate log space
  if (UnitTest->Log == NULL) 
  {
    UnitTestLogInit(UnitTest, NULL, 0);
  }

  if (UnitTest->Log == NULL) 
  {
    DEBUG((DEBUG_ERROR, "Failed to allocate space for unit test log\n"));
    ASSERT(UnitTest->Log != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = StrnCatS(UnitTest->Log, UNIT_TEST_MAX_LOG_BUFFER/ sizeof(CHAR16), String, UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH);
  if(EFI_ERROR(Status)) 
  {
    DEBUG((DEBUG_ERROR, "Failed to add unit test log string.  Status = %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
AddUnitTestFailure(
  IN OUT UNIT_TEST    *UnitTest,
  IN CONST CHAR16 *FailureMessage,
  FAILURE_TYPE FailureType
)
{
  //
  // Make sure that you're cooking with gas.
  //
  if (UnitTest == NULL || FailureMessage == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  UnitTest->FailureType = FailureType;
  StrCpyS(&UnitTest->FailureMessage[0], UNIT_TEST_TESTFAILUREMSG_LENGTH, FailureMessage);

  return EFI_SUCCESS;
}


//=============================================================================
//
// ----------------  PUBLIC FUNCTIONS ------------------------------------
//
//=============================================================================
VOID
EFIAPI
UnitTestLogInit (
IN OUT UNIT_TEST  *Test,
IN UINT8      *Buffer,
IN UINTN      BufferSize
)
{
  //
  // Make sure that you're cooking with gas.
  //
  if (Test == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " called with invalid Test parameter\n"));
    return;
  }

  // If this is the first log for the test allocate log space
  if (Test->Log == NULL) 
  {
    Test->Log = AllocateZeroPool(UNIT_TEST_MAX_LOG_BUFFER);
  }

  //check again to make sure allocate worked
  if(Test->Log == NULL)
  {
    DEBUG((DEBUG_ERROR, "Failed to allocate memory for the log\n"));
    return;
  }
  
  if((Buffer != NULL) && (BufferSize > 0) && ((BufferSize <= UNIT_TEST_MAX_LOG_BUFFER)))
  {
    CopyMem(Test->Log, Buffer, BufferSize);
  }
}

VOID
EFIAPI
UnitTestLog (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN  UINTN                       ErrorLevel,
  IN  CONST CHAR8                 *Format,
  ...
  )
{
  CHAR8         NewFormatString[UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH];
  CHAR16        LogString[UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH];
  CONST CHAR8   *LogTypePrefix = NULL;
  VA_LIST       Marker;
  UINTN LogLevel = (UINTN) PcdGet32(UnitTestLogLevel);
  //
  // Make sure that this debug mode is enabled.
  //
  if ((ErrorLevel & LogLevel) == 0) {
      return;
  }

  //
  // If we need to define a new format string...
  // well... get to it.
  //
  LogTypePrefix = GetStringForStatusLogPrefix( ErrorLevel );
  if (LogTypePrefix != NULL)
  {
    AsciiSPrint( NewFormatString, sizeof( NewFormatString ), "%a%a", LogTypePrefix, Format );
  }
  else
  {
    AsciiStrCpyS( NewFormatString, sizeof( NewFormatString ), Format );
  }

  //
  // Convert the message to an ASCII String
  //
  VA_START (Marker, Format);
  UnicodeVSPrintAsciiFormat( LogString, sizeof( LogString ), NewFormatString, Marker );
  VA_END (Marker);

  //
  // Finally, add the string to the log.
  //
  AddStringToUnitTestLog( ((UNIT_TEST_FRAMEWORK*)Framework)->CurrentTest, LogString );

  return;
}

VOID
EFIAPI
UnitTestLogFailure(
  IN  UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  FAILURE_TYPE FailureType,
  IN  CONST CHAR8                 *Format,
  ...
)
{
  CHAR16        LogString[UNIT_TEST_TESTFAILUREMSG_LENGTH];
  VA_LIST       Marker;


  //
  // Convert the message to an ASCII String
  //
  VA_START(Marker, Format);
  UnicodeVSPrintAsciiFormat(LogString, sizeof(LogString), Format, Marker);
  VA_END(Marker);

  //
  // Finally, add the string to the log.
  //
  AddUnitTestFailure(((UNIT_TEST_FRAMEWORK*)Framework)->CurrentTest, LogString, FailureType);

  return;
}

