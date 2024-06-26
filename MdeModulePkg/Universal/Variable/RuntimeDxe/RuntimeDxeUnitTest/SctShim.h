/** @file
Helper functions and shim defintions for missing
SCT components.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SCT_SHIM_H_
#define SCT_SHIM_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UnitTestLib.h>

#include <StandardTestLibrary.h>
#include <TestLoggingLibrary.h>

#define EFI_BB_TEST_PROTOCOL                VOID
#define EFI_TEST_RECOVERY_LIBRARY_PROTOCOL  VOID
#define SctStrCmp                           StrCmp
#define SctStrnCmp                          StrnCmp
#define SctStrLen                           StrLen
#define SctStrCat                           UnsafeStrCat
#define SctStrnCpy                          UnsafeStrnCpy
#define gHwErrRecGuid                       gEfiHardwareErrorVariableGuid
#define gGlobalVariableGuid                 gEfiGlobalVariableGuid

#define EFI_MAX_PRINT_BUFFER  1024

#define SCT_HOST_TEST_PRIVATE_DATA_SIGNATURE  SIGNATURE_32('H','T','P','D')
typedef struct _SCT_HOST_TEST_PRIVATE_DATA SCT_HOST_TEST_PRIVATE_DATA;

//
// Generic Failure GUID is designed for the test environment issues or
// the walking-on issues. It will be recorded in the log files, but will not be
// summarized in the test report by the EFI SCT Suite.
//
#define TEST_GENERIC_FAILURE_GUID         \
  { 0x6a8caa83, 0xb9da, 0x46c7, { 0x98, 0xf6, 0xd4, 0x96, 0x9d, 0xab, 0xda, 0xa0 }}

extern EFI_GUID  gTestGenericFailureGuid;

//
// Dummy GUID is designed as a temporary GUID that will be replaced with a
// formal GUID later. It will be easy to use this GUID in prototype phase,
// and then use a tool to do the replacement.
//
#define TEST_DUMMY_GUID                   \
  { 0xece4bdd5, 0x8177, 0x448b, { 0x82, 0x03, 0x2d, 0x11, 0x0c, 0x1c, 0x20, 0xb8 }}

extern EFI_GUID  gTestDummyGuid;

//
// Global variables for the services tables
//

extern EFI_SYSTEM_TABLE      *gtST;
extern EFI_BOOT_SERVICES     *gtBS;
extern EFI_RUNTIME_SERVICES  *gtRT;

/**
  [ATTENTION] This function is deprecated for security reason.

  Copies up to a specified length from one Null-terminated Unicode string to
  another Null-terminated Unicode string and returns the new Unicode string.

  This function copies the contents of the Unicode string Source to the Unicode
  string Destination, and returns Destination. At most, Length Unicode
  characters are copied from Source to Destination. If Length is 0, then
  Destination is returned unmodified. If Length is greater that the number of
  Unicode characters in Source, then Destination is padded with Null Unicode
  characters. If Source and Destination overlap, then the results are
  undefined.

  If Length > 0 and Destination is NULL, then ASSERT().
  If Length > 0 and Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Length > 0 and Source is NULL, then ASSERT().
  If Length > 0 and Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Length is greater than
  PcdMaximumUnicodeStringLength, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().

  @param  Destination The pointer to a Null-terminated Unicode string.
  @param  Source      The pointer to a Null-terminated Unicode string.
  @param  Length      The maximum number of Unicode characters to copy.

  @return Destination.

**/
CHAR16 *
EFIAPI
UnsafeStrnCpy (
  OUT     CHAR16        *Destination,
  IN      CONST CHAR16  *Source,
  IN      UINTN         Length
  );

/**
  [ATTENTION] This function is deprecated for security reason.

  Concatenates one Null-terminated Unicode string to another Null-terminated
  Unicode string, and returns the concatenated Unicode string.

  This function concatenates two Null-terminated Unicode strings. The contents
  of Null-terminated Unicode string Source are concatenated to the end of
  Null-terminated Unicode string Destination. The Null-terminated concatenated
  Unicode String is returned. If Source and Destination overlap, then the
  results are undefined.

  If Destination is NULL, then ASSERT().
  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Destination contains more
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  Destination The pointer to a Null-terminated Unicode string.
  @param  Source      The pointer to a Null-terminated Unicode string.

  @return Destination.

**/
CHAR16 *
EFIAPI
UnsafeStrCat (
  IN OUT  CHAR16        *Destination,
  IN      CONST CHAR16  *Source
  );

INTN
SctCompareGuid (
  IN EFI_GUID  *Guid1,
  IN EFI_GUID  *Guid2
  );

UINTN
SctXtoi (
  CHAR16  *str
  );

CHAR16 *
SctPoolPrint (
  IN CONST CHAR16  *Fmt,
  ...
  );

VOID
InitSctShim (
  IN EFI_BOOT_SERVICES     *BS,
  IN EFI_RUNTIME_SERVICES  *RT
  );

EFI_STATUS
InitSctPrivateData (
  IN OUT UNIT_TEST_STATUS            *TestResult,
  OUT    SCT_HOST_TEST_PRIVATE_DATA  *PrivateData
  );

struct _SCT_HOST_TEST_PRIVATE_DATA {
  UINT32                                Signature;
  EFI_STANDARD_TEST_LIBRARY_PROTOCOL    StandardTest;
  UNIT_TEST_STATUS                      *ReturnStatus;
};

#define SCT_HOST_TEST_PRIVATE_DATA_FROM_STSL(a) \
  CR(a, SCT_HOST_TEST_PRIVATE_DATA, StandardTest, SCT_HOST_TEST_PRIVATE_DATA_SIGNATURE)

#endif // SCT_SHIM_H_
