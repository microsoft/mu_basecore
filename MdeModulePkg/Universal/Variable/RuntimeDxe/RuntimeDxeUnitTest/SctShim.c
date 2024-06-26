/** @file
  Helper functions and shim defintions for missing
  SCT components.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UnitTestLib.h>

#include "VariableServicesBBTestMain.h"
#include "SctShim.h"

EFI_GUID  gTestVendor1Guid        = TEST_VENDOR1_GUID;
EFI_GUID  gTestVendor2Guid        = TEST_VENDOR2_GUID;
EFI_GUID  gTestGenericFailureGuid = TEST_GENERIC_FAILURE_GUID;

EFI_BOOT_SERVICES  *gtBS = NULL;

CHAR16 *
EFIAPI
UnsafeStrnCpy (
  OUT     CHAR16        *Destination,
  IN      CONST CHAR16  *Source,
  IN      UINTN         Length
  )
{
  // NOTE: This is *VERY* unsafe. But it's just a test... so...
  StrnCpyS (Destination, (UINTN)-1, Source, Length);
  return Destination;
}

CHAR16 *
EFIAPI
UnsafeStrCat (
  IN OUT  CHAR16        *Destination,
  IN      CONST CHAR16  *Source
  )
{
  // NOTE: This is *VERY* unsafe. But it's just a test... so...
  StrCatS (Destination, (UINTN)-1, Source);
  return Destination;
}

INTN
SctCompareGuid (
  IN EFI_GUID  *Guid1,
  IN EFI_GUID  *Guid2
  )
{
  INT32  *g1, *g2, r;

  //
  // Compare 32 bits at a time
  //

  g1 = (INT32 *)Guid1;
  g2 = (INT32 *)Guid2;

  r  = g1[0] - g2[0];
  r |= g1[1] - g2[1];
  r |= g1[2] - g2[2];
  r |= g1[3] - g2[3];

  return r;
}

UINTN
SctXtoi (
  CHAR16  *str
  )
{
  UINTN   u;
  CHAR16  c;

  // skip preceeding white space
  while (*str && *str == ' ') {
    str += 1;
  }

  // skip preceeding zeros
  while (*str && *str == '0') {
    str += 1;
  }

  // skip preceeding white space
  if (*str && ((*str == 'x') || (*str == 'X'))) {
    str += 1;
  }

  // convert hex digits
  u = 0;
  c = *(str++);
  while (c) {
    if ((c >= 'a') &&  (c <= 'f')) {
      c -= 'a' - 'A';
    }

    if (((c >= '0') &&  (c <= '9'))  ||  ((c >= 'A') &&  (c <= 'F'))) {
      u = (u << 4)  |  (c - (c >= 'A' ? 'A'-10 : '0'));
    } else {
      break;
    }

    c = *(str++);
  }

  return u;
}

CHAR16 *
SctPoolPrint (
  IN CONST CHAR16  *Fmt,
  ...
  )
{
  UINTN    Count;
  VA_LIST  Args;
  CHAR16   *Output;

  VA_START (Args, Fmt);
  Count = SPrintLength (Fmt, Args);
  VA_END (Args);

  Output = AllocatePool (Count);

  if (Output != NULL) {
    VA_START (Args, Fmt);
    UnicodeVSPrint (Output, Count, Fmt, Args);
    VA_END (Args);
  }

  return Output;
}

EFI_STATUS
Myitox (
  IN UINTN    Num,
  OUT CHAR16  *StringNum
  )
{
  UINTN        CurrentNum;
  UINTN        CurrentPos;
  const UINTN  Times = 4;

  for ( CurrentPos = 0; CurrentPos < Times; CurrentPos++ ) {
    CurrentNum = Num % 16;
    Num       /= 16;

    if ( CurrentNum < 10 ) {
      StringNum[Times-CurrentPos-1] = (CHAR16)(L'0' + CurrentNum);
    } else {
      StringNum[Times-CurrentPos-1] = (CHAR16)(L'A' + (CurrentNum - 10));
    }
  }

  StringNum[Times] = '\0';

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HostTestRecordAssertion (
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *This,
  IN EFI_TEST_ASSERTION                  Type,
  IN EFI_GUID                            EventId,
  IN CHAR16                              *Description,
  IN CHAR16                              *Detail,
  ...
  )

/*++

Routine Description:

  Records the test result.

Arguments:

  This          - Standard test library protocol instance.
  Type          - Test result.
  EventId       - GUID for the checkpoint.
  Description   - Simple description for the checkpoint.
  Detail        - Format string for the detail test information.

Returns:

  EFI_SUCCESS           - record the assertion successfully.
  EFI_BAD_BUFFER_SIZE   - the Description string is too long.
  EFI_INVALID_PARAMETER - invalid Type.

--*/
{
  VA_LIST                     Marker;
  CHAR8                       AsciiBuffer[EFI_MAX_PRINT_BUFFER];
  CHAR16                      AssertionDetail[EFI_MAX_PRINT_BUFFER];
  CHAR16                      AssertionType[10];
  UINTN                       LogLevel;
  SCT_HOST_TEST_PRIVATE_DATA  *Private;

  Private = SCT_HOST_TEST_PRIVATE_DATA_FROM_STSL (This);

  //
  // Check the parameter
  //
  if (StrnLenS (Description, EFI_MAX_PRINT_BUFFER) + 14 > EFI_MAX_PRINT_BUFFER) {
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Build assertion detail string
  //
  VA_START (Marker, Detail);
  UnicodeVSPrint (AssertionDetail, EFI_MAX_PRINT_BUFFER, Detail, Marker);
  VA_END (Marker);

  if ( StrnLenS (AssertionDetail, EFI_MAX_PRINT_BUFFER) + 5 < EFI_MAX_PRINT_BUFFER) {
    StrCatS (AssertionDetail, EFI_MAX_PRINT_BUFFER, L"\r\n");
  }

  //
  // Write log file detail data
  //
  switch (Type) {
    case EFI_TEST_ASSERTION_PASSED:
      StrCpyS (AssertionType, EFI_MAX_PRINT_BUFFER, L"PASS");
      LogLevel = UNIT_TEST_LOG_LEVEL_VERBOSE;
      break;
    case EFI_TEST_ASSERTION_WARNING:
      StrCpyS (AssertionType, EFI_MAX_PRINT_BUFFER, L"WARNING");
      LogLevel = UNIT_TEST_LOG_LEVEL_WARN;
      break;
    case EFI_TEST_ASSERTION_FAILED:
      StrCpyS (AssertionType, EFI_MAX_PRINT_BUFFER, L"FAILURE");
      *Private->ReturnStatus = UNIT_TEST_ERROR_TEST_FAILED;
      LogLevel               = UNIT_TEST_LOG_LEVEL_ERROR;
      break;
    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  AsciiSPrintUnicodeFormat (
    AsciiBuffer,
    EFI_MAX_PRINT_BUFFER,
    L"%s -- %s\n"
    L"%g\n"
    L"%s\n",
    Description,
    AssertionType,
    &EventId,
    AssertionDetail
    );
  UnitTestLog (LogLevel, AsciiBuffer);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HostTestRecordMessage (
  IN EFI_STANDARD_TEST_LIBRARY_PROTOCOL  *This,
  IN EFI_VERBOSE_LEVEL                   VerboseLevel,
  IN CHAR16                              *Message,
  ...
  )

/*++

Routine Description:

  Records the test message.

Arguments:

  This          - Standard test library protocol instance.
  VerboseLevel  - Minimal verbose level to record this message. For example,
                  EFI_VERBOSE_LEVEL_QUIET means this message should be recorded
                  even the test is run in QUIET mode. On the contrary,
                  EFI_VERBOSE_LEVEL_EXHAUSTIVE means this message will only be
                  recorded when the test is run in EXHAUSTIVE mode.
  Message       - Format string for the detail test information.

Returns:

  EFI_SUCCESS   - record the message successfully.

--*/
{
  EFI_STATUS  Status;
  VA_LIST     Marker;
  CHAR8       AsciiBuffer[EFI_MAX_PRINT_BUFFER];

  Status = EFI_SUCCESS;

  if (VerboseLevel <= EFI_TEST_LEVEL_MINIMAL) {
    VA_START (Marker, Message);
    AsciiVSPrintUnicodeFormat (AsciiBuffer, EFI_MAX_PRINT_BUFFER, Message, Marker);
    VA_END (Marker);

    if ( AsciiStrnLenS (AsciiBuffer, EFI_MAX_PRINT_BUFFER) + 3 < EFI_MAX_PRINT_BUFFER ) {
      AsciiStrCatS (AsciiBuffer, EFI_MAX_PRINT_BUFFER, "\r\n");
    }

    UnitTestLog (UNIT_TEST_LOG_LEVEL_VERBOSE, AsciiBuffer);
  }

  return Status;
}

EFI_STATUS
GetTestSupportLibrary (
  IN EFI_HANDLE                           SupportHandle,
  OUT EFI_STANDARD_TEST_LIBRARY_PROTOCOL  **StandardLib,
  OUT EFI_TEST_RECOVERY_LIBRARY_PROTOCOL  **RecoveryLib,
  OUT EFI_TEST_LOGGING_LIBRARY_PROTOCOL   **LoggingLib
  )
{
  SCT_HOST_TEST_PRIVATE_DATA  *PrivateData;

  PrivateData  = (SCT_HOST_TEST_PRIVATE_DATA *)SupportHandle;
  *StandardLib = &PrivateData->StandardTest;
  *RecoveryLib = NULL;
  *LoggingLib  = NULL;

  return EFI_SUCCESS;
}

VOID
InitSctShim (
  IN EFI_BOOT_SERVICES     *BS,
  IN EFI_RUNTIME_SERVICES  *RT
  )
{
  gtBS = BS;
}

EFI_STATUS
InitSctPrivateData (
  IN OUT UNIT_TEST_STATUS            *TestResult,
  OUT    SCT_HOST_TEST_PRIVATE_DATA  *PrivateData
  )
{
  ZeroMem (PrivateData, sizeof (*PrivateData));
  PrivateData->Signature                    = SCT_HOST_TEST_PRIVATE_DATA_SIGNATURE;
  PrivateData->ReturnStatus                 = TestResult;
  PrivateData->StandardTest.RecordAssertion = HostTestRecordAssertion;
  PrivateData->StandardTest.RecordMessage   = HostTestRecordMessage;
  return EFI_SUCCESS;
}
