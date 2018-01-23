/**
@file
UEFI Shell based application for unit testing the IntSafeLib.


Copyright (c) 2017, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include "IntSafeLibUnitTests.h"

#define UNIT_TEST_APP_NAME        L"Int Safe Lib Unit Test Application"
#define UNIT_TEST_APP_VERSION     L"0.1"

//
// Conversion function tests:
//
UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT8 Operand = 0x5b;
  UINT8 Result = 0;

  //
  // Positive UINT8 should result in just a cast
  //
  Status = SafeInt8ToUInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT8 Operand = 0x5b;
  UINT16 Result = 0;

  //
  // Positive UINT8 should result in just a cast
  //
  Status = SafeInt8ToUInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT8 Operand = 0x5b;
  UINT32 Result = 0;

  //
  // Positive UINT8 should result in just a cast
  //
  Status = SafeInt8ToUInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT8 Operand = 0x5b;
  UINTN Result = 0;

  //
  // Positive UINT8 should result in just a cast
  //
  Status = SafeInt8ToUIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUInt64(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT8 Operand = 0x5b;
  UINT64 Result = 0;

  //
  // Positive UINT8 should result in just a cast
  //
  Status = SafeInt8ToUInt64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUInt64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt8ToInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8 Operand = 0x5b;
  INT8 Result = 0;

  //
  // Operand <= 0x7F (MAX_INT8) should result in a cast
  //
  Status = SafeUInt8ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Operand larger than 0x7f should result in an error status
  //
  Operand = 0xaf;
  Status = SafeUInt8ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt8ToChar8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8 Operand = 0x5b;
  CHAR8 Result = 0;

  //
  // CHAR8 is typedefed as char, which by default is signed, thus
  // CHAR8 is same as INT8, so same tests as above:
  //

  //
  // Operand <= 0x7F (MAX_INT8) should result in a cast
  //
  Status = SafeUInt8ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Operand larger than 0x7f should result in an error status
  //
  Operand = 0xaf;
  Status = SafeUInt8ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Operand = 0x5b;
  INT8 Result = 0;

  //
  // If Operand is between MIN_INT8 and MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeInt16ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  Operand = (-35);
  Status = SafeInt16ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-35), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = 0x1234;
  Status = SafeInt16ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-17835);
  Status = SafeInt16ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToChar8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Operand = 0x5b;
  CHAR8 Result = 0;

  //
  // CHAR8 is typedefed as char, which by default is signed, thus
  // CHAR8 is same as INT8, so same tests as above:
  //

  //
  // If Operand is between MIN_INT8 and MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeInt16ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  Operand = (-35);
  Status = SafeInt16ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-35), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = 0x1234;
  Status = SafeInt16ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-17835);
  Status = SafeInt16ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToUInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Operand = 0x5b;
  UINT8 Result = 0;

  //
  // If Operand is between 0 and MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeInt16ToUInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = 0x1234;
  Status = SafeInt16ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-17835);
  Status = SafeInt16ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToUInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Operand = 0x5b5b;
  UINT16 Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeInt16ToUInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17835);
  Status = SafeInt16ToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToUInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Operand = 0x5b5b;
  UINT32 Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeInt16ToUInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17835);
  Status = SafeInt16ToUInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToUIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Operand = 0x5b5b;
  UINTN Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeInt16ToUIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17835);
  Status = SafeInt16ToUIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToUInt64(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Operand = 0x5b5b;
  UINT64 Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeInt16ToUInt64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17835);
  Status = SafeInt16ToUInt64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt16ToInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT16 Operand = 0x5b;
  INT8 Result = 0;

  //
  // If Operand is <= MAX_INT8, it's a cast
  //
  Status = SafeUInt16ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5b5b);
  Status = SafeUInt16ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt16ToChar8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT16 Operand = 0x5b;
  CHAR8 Result = 0;

  // CHAR8 is typedefed as char, which by default is signed, thus
  // CHAR8 is same as INT8, so same tests as above:

  //
  // If Operand is <= MAX_INT8, it's a cast
  //
  Status = SafeUInt16ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5b5b);
  Status = SafeUInt16ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt16ToUInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT16 Operand = 0xab;
  UINT8 Result = 0;

  //
  // If Operand is <= MAX_UINT8 (0xff), it's a cast
  //
  Status = SafeUInt16ToUInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5b5b);
  Status = SafeUInt16ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt16ToInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT16 Operand = 0x5b5b;
  INT16 Result = 0;

  //
  // If Operand is <= MAX_INT16 (0x7fff), it's a cast
  //
  Status = SafeUInt16ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUInt16ToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Operand = 0x5b;
  INT8 Result = 0;

  //
  // If Operand is between MIN_INT8 and MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeInt32ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  Operand = (-57);
  Status = SafeInt32ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-57), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeInt32ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeInt32ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToChar8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Operand = 0x5b;
  CHAR8 Result = 0;

  // CHAR8 is typedefed as char, which by default is signed, thus
	// CHAR8 is same as INT8, so same tests as above:

  //
  // If Operand is between MIN_INT8 and MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeInt32ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  Operand = (-57);
  Status = SafeInt32ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-57), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeInt32ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeInt32ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToUInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Operand = 0x5b;
  UINT8 Result = 0;

  //
  // If Operand is between 0 and MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeInt32ToUInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-57);
  Status = SafeInt32ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (0x5bababab);
  Status = SafeInt32ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeInt32ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Operand = 0x5b5b;
  INT16 Result = 0;

  //
  // If Operand is between MIN_INT16 and MAX_INT16 inclusive, then it's a cast
  //
  Status = SafeInt32ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  Operand = (-17857);
  Status = SafeInt32ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-17857), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeInt32ToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeInt32ToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToUInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Operand = 0xabab;
  UINT16 Result = 0;

  //
  // If Operand is between 0 and MAX_UINT16 inclusive, then it's a cast
  //
  Status = SafeInt32ToUInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17857);
  Status = SafeInt32ToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (0x5bababab);
  Status = SafeInt32ToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeInt32ToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToUInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Operand = 0x5bababab;
  UINT32 Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeInt32ToUInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeInt32ToUInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToUInt64(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Operand = 0x5bababab;
  UINT64 Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeInt32ToUInt64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeInt32ToUInt64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32ToInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Operand = 0x5b;
  INT8 Result = 0;

  //
  // If Operand is <= MAX_INT8, then it's a cast
  //
  Status = SafeUInt32ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeUInt32ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32ToChar8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Operand = 0x5b;
  CHAR8 Result = 0;

  // CHAR8 is typedefed as char, which by default is signed, thus
	// CHAR8 is same as INT8, so same tests as above:

  //
  // If Operand is <= MAX_INT8, then it's a cast
  //
  Status = SafeUInt32ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeUInt32ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32ToUInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Operand = 0xab;
  UINT8 Result = 0;

  //
  // If Operand is <= MAX_UINT8, then it's a cast
  //
  Status = SafeUInt32ToUInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUInt32ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32ToInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Operand = 0x5bab;
  INT16 Result = 0;

  //
  // If Operand is <= MAX_INT16, then it's a cast
  //
  Status = SafeUInt32ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUInt32ToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32ToUInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Operand = 0xabab;
  UINT16 Result = 0;

  //
  // If Operand is <= MAX_UINT16, then it's a cast
  //
  Status = SafeUInt32ToUInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUInt32ToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32ToInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Operand = 0x5bababab;
  INT32 Result = 0;

  //
  // If Operand is <= MAX_INT32, then it's a cast
  //
  Status = SafeUInt32ToInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUInt32ToInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Operand = 0x5b;
  INT8 Result = 0;

  //
  // If Operand is between MIN_INT8 and MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeIntNToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  Operand = (-53);
  Status = SafeIntNToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-53), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeIntNToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeIntNToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToChar8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Operand = 0x5b;
  CHAR8 Result = 0;

  // CHAR8 is typedefed as char, which by default is signed, thus
	// CHAR8 is same as INT8, so same tests as above:

  //
  // If Operand is between MIN_INT8 and MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeIntNToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  Operand = (-53);
  Status = SafeIntNToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-53), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeIntNToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeIntNToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToUInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Operand = 0xab;
  UINT8 Result = 0;

  //
  // If Operand is between 0 and MAX_UINT8 inclusive, then it's a cast
  //
  Status = SafeIntNToUInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeIntNToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeIntNToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Operand = 0x5bab;
  INT16 Result = 0;

  //
  // If Operand is between MIN_INT16 and MAX_INT16 inclusive, then it's a cast
  //
  Status = SafeIntNToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bab, Result);

  Operand = (-23467);
  Status = SafeIntNToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-23467), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeIntNToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeIntNToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToUInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Operand = 0xabab;
  UINT16 Result = 0;

  //
  // If Operand is between 0 and MAX_UINT16 inclusive, then it's a cast
  //
  Status = SafeIntNToUInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeIntNToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeIntNToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToUIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Operand = 0x5bababab;
  UINTN Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeIntNToUIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeIntNToUIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToUInt64(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Operand = 0x5bababab;
  UINT64 Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeIntNToUInt64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeIntNToUInt64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Operand = 0x5b;
  INT8 Result = 0;

  //
  // If Operand is <= MAX_INT8, then it's a cast
  //
  Status = SafeUIntNToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUIntNToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToChar8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Operand = 0x5b;
  CHAR8 Result = 0;

  // CHAR8 is typedefed as char, which by default is signed, thus
	// CHAR8 is same as INT8, so same tests as above:

  //
  // If Operand is <= MAX_INT8, then it's a cast
  //
  Status = SafeUIntNToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUIntNToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToUInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Operand = 0xab;
  UINT8 Result = 0;

  //
  // If Operand is <= MAX_UINT8, then it's a cast
  //
  Status = SafeUIntNToUInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUIntNToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Operand = 0x5bab;
  INT16 Result = 0;

  //
  // If Operand is <= MAX_INT16, then it's a cast
  //
  Status = SafeUIntNToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUIntNToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToUInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Operand = 0xabab;
  UINT16 Result = 0;

  //
  // If Operand is <= MAX_UINT16, then it's a cast
  //
  Status = SafeUIntNToUInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUIntNToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Operand = 0x5bababab;
  INT32 Result = 0;

  //
  // If Operand is <= MAX_INT32, then it's a cast
  //
  Status = SafeUIntNToInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUIntNToInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0x5b;
  INT8 Result = 0;

  //
  // If Operand is between MIN_INT8 and  MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeInt64ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  Operand = (-37);
  Status = SafeInt64ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-37), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToChar8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0x5b;
  CHAR8 Result = 0;

  // CHAR8 is typedefed as char, which by default is signed, thus
  // CHAR8 is same as INT8, so same tests as above:

  //
  // If Operand is between MIN_INT8 and  MAX_INT8 inclusive, then it's a cast
  //
  Status = SafeInt64ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  Operand = (-37);
  Status = SafeInt64ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-37), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToUInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0xab;
  UINT8 Result = 0;

  //
  // If Operand is between 0 and  MAX_UINT8 inclusive, then it's a cast
  //
  Status = SafeInt64ToUInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0x5bab;
  INT16 Result = 0;

  //
  // If Operand is between MIN_INT16 and  MAX_INT16 inclusive, then it's a cast
  //
  Status = SafeInt64ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bab, Result);

  Operand = (-23467);
  Status = SafeInt64ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-23467), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToUInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0xabab;
  UINT16 Result = 0;

  //
  // If Operand is between 0 and  MAX_UINT16 inclusive, then it's a cast
  //
  Status = SafeInt64ToUInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0x5bababab;
  INT32 Result = 0;

  //
  // If Operand is between MIN_INT32 and  MAX_INT32 inclusive, then it's a cast
  //
  Status = SafeInt64ToInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  Operand = (-1537977259);
  Status = SafeInt64ToInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-1537977259), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToUInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0xabababab;
  UINT32 Result = 0;

  //
  // If Operand is between 0 and  MAX_UINT32 inclusive, then it's a cast
  //
  Status = SafeInt64ToUInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToUInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToUInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToUInt64(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0x5babababefefefef;
  UINT64 Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeInt64ToUInt64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5babababefefefef, Result);

  //
  // Otherwise should result in an error status
  //
  Operand =  (-6605562033422200815);
  Status = SafeInt64ToUInt64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0x5b;
  INT8 Result = 0;

  //
  // If Operand is <= MAX_INT8, then it's a cast
  //
  Status = SafeUInt64ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToChar8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0x5b;
  CHAR8 Result = 0;

  // CHAR8 is typedefed as char, which by default is signed, thus
  // CHAR8 is same as INT8, so same tests as above:

  //
  // If Operand is <= MAX_INT8, then it's a cast
  //
  Status = SafeUInt64ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToUInt8(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0xab;
  UINT8 Result = 0;

  //
  // If Operand is <= MAX_UINT8, then it's a cast
  //
  Status = SafeUInt64ToUInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToUInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0x5bab;
  INT16 Result = 0;

  //
  // If Operand is <= MAX_INT16, then it's a cast
  //
  Status = SafeUInt64ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToUInt16(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0xabab;
  UINT16 Result = 0;

  //
  // If Operand is <= MAX_UINT16, then it's a cast
  //
  Status = SafeUInt64ToUInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToUInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0x5bababab;
  INT32 Result = 0;

  //
  // If Operand is <= MAX_INT32, then it's a cast
  //
  Status = SafeUInt64ToInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToUInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0xabababab;
  UINT32 Result = 0;

  //
  // If Operand is <= MAX_UINT32, then it's a cast
  //
  Status = SafeUInt64ToUInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToUInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToInt64(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0x5babababefefefef;
  INT64 Result = 0;

  //
  // If Operand is <= MAX_INT64, then it's a cast
  //
  Status = SafeUInt64ToInt64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5babababefefefef, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToInt64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

//
// Addition function tests:
//
UNIT_TEST_STATUS
EFIAPI
TestSafeUInt8Add(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8 Augend = 0x3a;
  UINT8 Addend = 0x3a;
  UINT8 Result = 0;

  //
  // If the result of addition doesn't overflow MAX_UINT8, then it's addition
  //
  Status = SafeUInt8Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x74, Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0xab;
  Addend = 0xbc;
  Status = SafeUInt8Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt16Add(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT16 Augend = 0x3a3a;
  UINT16 Addend = 0x3a3a;
  UINT16 Result = 0;

  //
  // If the result of addition doesn't overflow MAX_UINT16, then it's addition
  //
  Status = SafeUInt16Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x7474, Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0xabab;
  Addend = 0xbcbc;
  Status = SafeUInt16Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32Add(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Augend = 0x3a3a3a3a;
  UINT32 Addend = 0x3a3a3a3a;
  UINT32 Result = 0;

  //
  // If the result of addition doesn't overflow MAX_UINT32, then it's addition
  //
  Status = SafeUInt32Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x74747474, Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0xabababab;
  Addend = 0xbcbcbcbc;
  Status = SafeUInt32Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64Add(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Augend = 0x3a3a3a3a12121212;
  UINT64 Addend = 0x3a3a3a3a12121212;
  UINT64 Result = 0;

  //
  // If the result of addition doesn't overflow MAX_UINT64, then it's addition
  //
  Status = SafeUInt64Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x7474747424242424, Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0xababababefefefef;
  Addend = 0xbcbcbcbcdededede;
  Status = SafeUInt64Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8Add(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT8 Augend = 0x3a;
  INT8 Addend = 0x3a;
  INT8 Result = 0;

  //
  // If the result of addition doesn't overflow MAX_INT8
  // and doesn't underflow MIN_INT8, then it's addition
  //
  Status = SafeInt8Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x74, Result);

  Augend = (-58);
  Addend = (-58);
  Status = SafeInt8Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-116), Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0x5a;
  Addend = 0x5a;
  Status = SafeInt8Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Augend = (-90);
  Addend = (-90);
  Status = SafeInt8Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;

}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16Add(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Augend = 0x3a3a;
  INT16 Addend = 0x3a3a;
  INT16 Result = 0;

  //
  // If the result of addition doesn't overflow MAX_INT16
  // and doesn't underflow MIN_INT16, then it's addition
  //
  Status = SafeInt16Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x7474, Result);

  Augend = (-14906);
  Addend = (-14906);
  Status = SafeInt16Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-29812), Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0x5a5a;
  Addend = 0x5a5a;
  Status = SafeInt16Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Augend = (-23130);
  Addend = (-23130);
  Status = SafeInt16Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32Add(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Augend = 0x3a3a3a3a;
  INT32 Addend = 0x3a3a3a3a;
  INT32 Result = 0;

  //
  // If the result of addition doesn't overflow MAX_INT32
  // and doesn't underflow MIN_INT32, then it's addition
  //
  Status = SafeInt32Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x74747474, Result);

  Augend = (-976894522);
  Addend = (-976894522);
  Status = SafeInt32Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-1953789044), Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0x5a5a5a5a;
  Addend = 0x5a5a5a5a;
  Status = SafeInt32Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Augend = (-1515870810);
  Addend = (-1515870810);
  Status = SafeInt32Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64Add(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Augend = 0x3a3a3a3a3a3a3a3a;
  INT64 Addend = 0x3a3a3a3a3a3a3a3a;
  INT64 Result = 0;

  //
  // If the result of addition doesn't overflow MAX_INT64
  // and doesn't underflow MIN_INT64, then it's addition
  //
  Status = SafeInt64Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x7474747474747474, Result);

  Augend = (-4195730024608447034);
  Addend = (-4195730024608447034);
  Status = SafeInt64Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-8391460049216894068), Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0x5a5a5a5a5a5a5a5a;
  Addend = 0x5a5a5a5a5a5a5a5a;
  Status = SafeInt64Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Augend = (-6510615555426900570);
  Addend = (-6510615555426900570);
  Status = SafeInt64Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

//
// Subtraction function tests:
//
UNIT_TEST_STATUS
EFIAPI
TestSafeUInt8Sub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8 Minuend = 0x5a;
  UINT8 Subtrahend = 0x3b;
  UINT8 Result = 0;

  //
  // If Minuend >= Subtrahend, then it's subtraction
  //
  Status = SafeUInt8Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x1f, Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = 0x5a;
  Subtrahend = 0x6d;
  Status = SafeUInt8Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt16Sub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT16 Minuend = 0x5a5a;
  UINT16 Subtrahend = 0x3b3b;
  UINT16 Result = 0;

  //
  // If Minuend >= Subtrahend, then it's subtraction
  //
  Status = SafeUInt16Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x1f1f, Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = 0x5a5a;
  Subtrahend = 0x6d6d;
  Status = SafeUInt16Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32Sub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Minuend = 0x5a5a5a5a;
  UINT32 Subtrahend = 0x3b3b3b3b;
  UINT32 Result = 0;

  //
  // If Minuend >= Subtrahend, then it's subtraction
  //
  Status = SafeUInt32Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x1f1f1f1f, Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = 0x5a5a5a5a;
  Subtrahend = 0x6d6d6d6d;
  Status = SafeUInt32Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64Sub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Minuend = 0x5a5a5a5a5a5a5a5a;
  UINT64 Subtrahend = 0x3b3b3b3b3b3b3b3b;
  UINT64 Result = 0;

  //
  // If Minuend >= Subtrahend, then it's subtraction
  //
  Status = SafeUInt64Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x1f1f1f1f1f1f1f1f, Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = 0x5a5a5a5a5a5a5a5a;
  Subtrahend = 0x6d6d6d6d6d6d6d6d;
  Status = SafeUInt64Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8Sub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT8 Minuend = 0x5a;
  INT8 Subtrahend = 0x3a;
  INT8 Result = 0;

  //
  // If the result of subtractions doesn't overflow MAX_INT8 or
  // underflow MIN_INT8, then it's subtraction
  //
  Status = SafeInt8Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x20, Result);

  Minuend = 58;
  Subtrahend = 78;
  Status = SafeInt8Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-20), Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = (-80);
  Subtrahend = 80;
  Status = SafeInt8Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Minuend = (80);
  Subtrahend = (-80);
  Status = SafeInt8Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16Sub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Minuend = 0x5a5a;
  INT16 Subtrahend = 0x3a3a;
  INT16 Result = 0;

  //
  // If the result of subtractions doesn't overflow MAX_INT16 or
  // underflow MIN_INT16, then it's subtraction
  //
  Status = SafeInt16Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x2020, Result);

  Minuend = 0x3a3a;
  Subtrahend = 0x5a5a;
  Status = SafeInt16Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-8224), Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = (-31354);
  Subtrahend = 31354;
  Status = SafeInt16Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Minuend = (31354);
  Subtrahend = (-31354);
  Status = SafeInt16Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32Sub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Minuend = 0x5a5a5a5a;
  INT32 Subtrahend = 0x3a3a3a3a;
  INT32 Result = 0;

  //
  // If the result of subtractions doesn't overflow MAX_INT32 or
  // underflow MIN_INT32, then it's subtraction
  //
  Status = SafeInt32Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x20202020, Result);

  Minuend = 0x3a3a3a3a;
  Subtrahend = 0x5a5a5a5a;
  Status = SafeInt32Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-538976288), Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = (-2054847098);
  Subtrahend = 2054847098;
  Status = SafeInt32Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Minuend = (2054847098);
  Subtrahend = (-2054847098);
  Status = SafeInt32Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64Sub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Minuend = 0x5a5a5a5a5a5a5a5a;
  INT64 Subtrahend = 0x3a3a3a3a3a3a3a3a;
  INT64 Result = 0;

  //
  // If the result of subtractions doesn't overflow MAX_INT64 or
  // underflow MIN_INT64, then it's subtraction
  //
  Status = SafeInt64Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x2020202020202020, Result);

  Minuend = 0x3a3a3a3a3a3a3a3a;
  Subtrahend = 0x5a5a5a5a5a5a5a5a;
  Status = SafeInt64Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-2314885530818453536), Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = (-8825501086245354106);
  Subtrahend = 8825501086245354106;
  Status = SafeInt64Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Minuend = (8825501086245354106);
  Subtrahend = (-8825501086245354106);
  Status = SafeInt64Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

//
// Multiplication function tests:
//
UNIT_TEST_STATUS
EFIAPI
TestSafeUInt8Mult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT8 Multiplicand = 0x12;
  UINT8 Multiplier = 0xa;
  UINT8 Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_UINT8, it will succeed
  //
  Status = SafeUInt8Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xb4, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x12;
  Multiplier = 0x23;
  Status = SafeUInt8Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt16Mult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT16 Multiplicand = 0x212;
  UINT16 Multiplier = 0x7a;
  UINT16 Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_UINT16, it will succeed
  //
  Status = SafeUInt16Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xfc94, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x1234;
  Multiplier = 0x213;
  Status = SafeUInt16Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32Mult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Multiplicand = 0xa122a;
  UINT32 Multiplier = 0xd23;
  UINT32 Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_UINT32, it will succeed
  //
  Status = SafeUInt32Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x844c9dbe, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0xa122a;
  Multiplier = 0xed23;
  Status = SafeUInt32Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64Mult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Multiplicand = 0x123456789a;
  UINT64 Multiplier = 0x1234567;
  UINT64 Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_UINT64, it will succeed
  //
  Status = SafeUInt64Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x14b66db9745a07f6, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x123456789a;
  Multiplier = 0x12345678;
  Status = SafeUInt64Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8Mult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT8 Multiplicand = 0x12;
  INT8 Multiplier = 0x7;
  INT8 Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_INT8 and doesn't
  // underflow MIN_UINT8, it will succeed
  //
  Status = SafeInt8Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x7e, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x12;
  Multiplier = 0xa;
  Status = SafeInt8Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16Mult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT16 Multiplicand = 0x123;
  INT16 Multiplier = 0x67;
  INT16 Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_INT16 and doesn't
  // underflow MIN_UINT16, it will succeed
  //
  Status = SafeInt16Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x7515, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x123;
  Multiplier = 0xab;
  Status = SafeInt16Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32Mult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Multiplicand = 0x123456;
  INT32 Multiplier = 0x678;
  INT32 Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_INT32 and doesn't
  // underflow MIN_UINT32, it will succeed
  //
  Status = SafeInt32Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x75c28c50, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x123456;
  Multiplier = 0xabc;
  Status = SafeInt32Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64Mult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Multiplicand = 0x123456789;
  INT64 Multiplier = 0x6789abcd;
  INT64 Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_INT64 and doesn't
  // underflow MIN_UINT64, it will succeed
  //
  Status = SafeInt64Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x75cd9045220d6bb5, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x123456789;
  Multiplier = 0xa789abcd;
  Status = SafeInt64Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

/**

  Main fuction sets up the unit test environment

**/
EFI_STATUS
EFIAPI
UefiMain(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable)
{
  EFI_STATUS                Status;
  UNIT_TEST_FRAMEWORK       *Fw = NULL;
  UNIT_TEST_SUITE           *ConversionTestSuite;
  UNIT_TEST_SUITE           *AdditionSubtractionTestSuite;
  UNIT_TEST_SUITE           *MultiplicationTestSuite;
  CHAR16  ShortName[100];
  ShortName[0] = L'\0';

  UnicodeSPrint(&ShortName[0], sizeof(ShortName), L"%a", gEfiCallerBaseName);
  DEBUG((DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework(&Fw, UNIT_TEST_APP_NAME, ShortName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  ///
  // Test the conversion functions
  //
  Status = CreateUnitTestSuite(&ConversionTestSuite, Fw, L"Int Safe Conversions Test Suite", L"Common.IntSafe.Convert", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Conversions Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUInt8",    L"Common.IntSafe.Convert.TestSafeInt8ToUInt8",    TestSafeInt8ToUInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUInt16",   L"Common.IntSafe.Convert.TestSafeInt8ToUInt16",   TestSafeInt8ToUInt16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUInt32",   L"Common.IntSafe.Convert.TestSafeInt8ToUInt32",   TestSafeInt8ToUInt32,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUIntN",    L"Common.IntSafe.Convert.TestSafeInt8ToUIntN",    TestSafeInt8ToUIntN,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUInt64",   L"Common.IntSafe.Convert.TestSafeInt8ToUInt64",   TestSafeInt8ToUInt64,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt8ToInt8",    L"Common.IntSafe.Convert.TestSafeUInt8ToInt8",    TestSafeUInt8ToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt8ToChar8",   L"Common.IntSafe.Convert.TestSafeUInt8ToChar8",   TestSafeUInt8ToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToInt8",    L"Common.IntSafe.Convert.TestSafeInt16ToInt8",    TestSafeInt16ToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToChar8",   L"Common.IntSafe.Convert.TestSafeInt16ToChar8",   TestSafeInt16ToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUInt8",   L"Common.IntSafe.Convert.TestSafeInt16ToUInt8",   TestSafeInt16ToUInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUInt16",  L"Common.IntSafe.Convert.TestSafeInt16ToUInt16",  TestSafeInt16ToUInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUInt32",  L"Common.IntSafe.Convert.TestSafeInt16ToUInt32",  TestSafeInt16ToUInt32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUIntN",   L"Common.IntSafe.Convert.TestSafeInt16ToUIntN",   TestSafeInt16ToUIntN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUInt64",  L"Common.IntSafe.Convert.TestSafeInt16ToUInt64",  TestSafeInt16ToUInt64,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt16ToInt8",   L"Common.IntSafe.Convert.TestSafeUInt16ToInt8",   TestSafeUInt16ToInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt16ToChar8",  L"Common.IntSafe.Convert.TestSafeUInt16ToChar8",  TestSafeUInt16ToChar8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt16ToUInt8",  L"Common.IntSafe.Convert.TestSafeUInt16ToUInt8",  TestSafeUInt16ToUInt8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt16ToInt16",  L"Common.IntSafe.Convert.TestSafeUInt16ToInt16",  TestSafeUInt16ToInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToInt8",    L"Common.IntSafe.Convert.TestSafeInt32ToInt8",    TestSafeInt32ToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToChar8",   L"Common.IntSafe.Convert.TestSafeInt32ToChar8",   TestSafeInt32ToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUInt8",   L"Common.IntSafe.Convert.TestSafeInt32ToUInt8",   TestSafeInt32ToUInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToInt16",   L"Common.IntSafe.Convert.TestSafeInt32ToInt16",   TestSafeInt32ToInt16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUInt16",  L"Common.IntSafe.Convert.TestSafeInt32ToUInt16",  TestSafeInt32ToUInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUInt32",  L"Common.IntSafe.Convert.TestSafeInt32ToUInt32",  TestSafeInt32ToUInt32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUIntN",   L"Common.IntSafe.Convert.TestSafeInt32ToUIntN",   TestSafeInt32ToUIntN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUInt64",  L"Common.IntSafe.Convert.TestSafeInt32ToUInt64",  TestSafeInt32ToUInt64,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt32ToInt8",   L"Common.IntSafe.Convert.TestSafeUInt32ToInt8",   TestSafeUInt32ToInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt32ToChar8",  L"Common.IntSafe.Convert.TestSafeUInt32ToChar8",  TestSafeUInt32ToChar8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt32ToUInt8",  L"Common.IntSafe.Convert.TestSafeUInt32ToUInt8",  TestSafeUInt32ToUInt8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt32ToInt16",  L"Common.IntSafe.Convert.TestSafeUInt32ToInt16",  TestSafeUInt32ToInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt32ToUInt16", L"Common.IntSafe.Convert.TestSafeUInt32ToUInt16", TestSafeUInt32ToUInt16, NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt32ToInt32",  L"Common.IntSafe.Convert.TestSafeUInt32ToInt32",  TestSafeUInt32ToInt32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt32ToIntN",   L"Common.IntSafe.Convert.TestSafeUInt32ToIntN",   TestSafeUInt32ToIntN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToInt8",     L"Common.IntSafe.Convert.TestSafeIntNToInt8",     TestSafeIntNToInt8,     NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToChar8",    L"Common.IntSafe.Convert.TestSafeIntNToChar8",    TestSafeIntNToChar8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUInt8",    L"Common.IntSafe.Convert.TestSafeIntNToUInt8",    TestSafeIntNToUInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToInt16",    L"Common.IntSafe.Convert.TestSafeIntNToInt16",    TestSafeIntNToInt16,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUInt16",   L"Common.IntSafe.Convert.TestSafeIntNToUInt16",   TestSafeIntNToUInt16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToInt32",    L"Common.IntSafe.Convert.TestSafeIntNToInt32",    TestSafeIntNToInt32,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUInt32",   L"Common.IntSafe.Convert.TestSafeIntNToUInt32",   TestSafeIntNToUInt32,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUIntN",    L"Common.IntSafe.Convert.TestSafeIntNToUIntN",    TestSafeIntNToUIntN,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUInt64",   L"Common.IntSafe.Convert.TestSafeIntNToUInt64",   TestSafeIntNToUInt64,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUIntNToInt8",    L"Common.IntSafe.Convert.TestSafeUIntNToInt8",    TestSafeUIntNToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUIntNToChar8",   L"Common.IntSafe.Convert.TestSafeUIntNToChar8",   TestSafeUIntNToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUIntNToUInt8",   L"Common.IntSafe.Convert.TestSafeUIntNToUInt8",   TestSafeUIntNToUInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUIntNToInt16",   L"Common.IntSafe.Convert.TestSafeUIntNToInt16",   TestSafeUIntNToInt16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUIntNToUInt16",  L"Common.IntSafe.Convert.TestSafeUIntNToUInt16",  TestSafeUIntNToUInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUIntNToInt32",   L"Common.IntSafe.Convert.TestSafeUIntNToInt32",   TestSafeUIntNToInt32,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUIntNToUInt32",  L"Common.IntSafe.Convert.TestSafeUIntNToUInt32",  TestSafeUIntNToUInt32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUIntNToIntN",    L"Common.IntSafe.Convert.TestSafeUIntNToIntN",    TestSafeUIntNToIntN,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUIntNToInt64",   L"Common.IntSafe.Convert.TestSafeUIntNToInt64",   TestSafeUIntNToInt64,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToInt8",    L"Common.IntSafe.Convert.TestSafeInt64ToInt8",    TestSafeInt64ToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToChar8",   L"Common.IntSafe.Convert.TestSafeInt64ToChar8",   TestSafeInt64ToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUInt8",   L"Common.IntSafe.Convert.TestSafeInt64ToUInt8",   TestSafeInt64ToUInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToInt16",   L"Common.IntSafe.Convert.TestSafeInt64ToInt16",   TestSafeInt64ToInt16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUInt16",  L"Common.IntSafe.Convert.TestSafeInt64ToUInt16",  TestSafeInt64ToUInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToInt32",   L"Common.IntSafe.Convert.TestSafeInt64ToInt32",   TestSafeInt64ToInt32,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUInt32",  L"Common.IntSafe.Convert.TestSafeInt64ToUInt32",  TestSafeInt64ToUInt32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToIntN",    L"Common.IntSafe.Convert.TestSafeInt64ToIntN",    TestSafeInt64ToIntN,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUIntN",   L"Common.IntSafe.Convert.TestSafeInt64ToUIntN",   TestSafeInt64ToUIntN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUInt64",  L"Common.IntSafe.Convert.TestSafeInt64ToUInt64",  TestSafeInt64ToUInt64,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToInt8",   L"Common.IntSafe.Convert.TestSafeUInt64ToInt8",   TestSafeUInt64ToInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToChar8",  L"Common.IntSafe.Convert.TestSafeUInt64ToChar8",  TestSafeUInt64ToChar8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToUInt8",  L"Common.IntSafe.Convert.TestSafeUInt64ToUInt8",  TestSafeUInt64ToUInt8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToInt16",  L"Common.IntSafe.Convert.TestSafeUInt64ToInt16",  TestSafeUInt64ToInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToUInt16", L"Common.IntSafe.Convert.TestSafeUInt64ToUInt16", TestSafeUInt64ToUInt16, NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToInt32",  L"Common.IntSafe.Convert.TestSafeUInt64ToInt32",  TestSafeUInt64ToInt32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToUInt32", L"Common.IntSafe.Convert.TestSafeUInt64ToUInt32", TestSafeUInt64ToUInt32, NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToIntN",   L"Common.IntSafe.Convert.TestSafeUInt64ToIntN",   TestSafeUInt64ToIntN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToUIntN",  L"Common.IntSafe.Convert.TestSafeUInt64ToUIntN",  TestSafeUInt64ToUIntN,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUInt64ToInt64",  L"Common.IntSafe.Convert.TestSafeUInt64ToInt64",  TestSafeUInt64ToInt64,  NULL, NULL, NULL);

  //
  // Test the addition and subtraction functions
  //
  Status = CreateUnitTestSuite(&AdditionSubtractionTestSuite, Fw, L"Int Safe Add/Subtract Test Suite", L"Common.IntSafe.AddSubtract", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Int Safe Add/Subtract Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUInt8Add",  L"Common.IntSafe.AddSubtract.TestSafeUInt8Add",  TestSafeUInt8Add,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUInt16Add", L"Common.IntSafe.AddSubtract.TestSafeUInt16Add", TestSafeUInt16Add, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUInt32Add", L"Common.IntSafe.AddSubtract.TestSafeUInt32Add", TestSafeUInt32Add, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUIntNAdd",  L"Common.IntSafe.AddSubtract.TestSafeUIntNAdd",  TestSafeUIntNAdd,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUInt64Add", L"Common.IntSafe.AddSubtract.TestSafeUInt64Add", TestSafeUInt64Add, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt8Add",   L"Common.IntSafe.AddSubtract.TestSafeInt8Add",   TestSafeInt8Add,   NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt16Add",  L"Common.IntSafe.AddSubtract.TestSafeInt16Add",  TestSafeInt16Add,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt32Add",  L"Common.IntSafe.AddSubtract.TestSafeInt32Add",  TestSafeInt32Add,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeIntNAdd",   L"Common.IntSafe.AddSubtract.TestSafeIntNAdd",   TestSafeIntNAdd,   NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt64Add",  L"Common.IntSafe.AddSubtract.TestSafeInt64Add",  TestSafeInt64Add,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUInt8Sub",  L"Common.IntSafe.AddSubtract.TestSafeUInt8Sub",  TestSafeUInt8Sub,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUInt16Sub", L"Common.IntSafe.AddSubtract.TestSafeUInt16Sub", TestSafeUInt16Sub, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUInt32Sub", L"Common.IntSafe.AddSubtract.TestSafeUInt32Sub", TestSafeUInt32Sub, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUIntNSub",  L"Common.IntSafe.AddSubtract.TestSafeUIntNSub",  TestSafeUIntNSub,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUInt64Sub", L"Common.IntSafe.AddSubtract.TestSafeUInt64Sub", TestSafeUInt64Sub, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt8Sub",   L"Common.IntSafe.AddSubtract.TestSafeInt8Sub",   TestSafeInt8Sub,   NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt16Sub",  L"Common.IntSafe.AddSubtract.TestSafeInt16Sub",  TestSafeInt16Sub,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt32Sub",  L"Common.IntSafe.AddSubtract.TestSafeInt32Sub",  TestSafeInt32Sub,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeIntNSub",   L"Common.IntSafe.AddSubtract.TestSafeIntNSub",   TestSafeIntNSub,   NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt64Sub",  L"Common.IntSafe.AddSubtract.TestSafeInt64Sub",  TestSafeInt64Sub,  NULL, NULL, NULL);

  //
  // Test the multiplication functions
  //
  Status = CreateUnitTestSuite(&MultiplicationTestSuite, Fw, L"Int Safe Multiply Test Suite", L"Common.IntSafe.Multiply", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Int Safe Multiply Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase(MultiplicationTestSuite, L"Test SafeUInt8Mult",  L"Common.IntSafe.Multiply.TestSafeUInt8Mult",  TestSafeUInt8Mult,  NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeUInt16Mult", L"Common.IntSafe.Multiply.TestSafeUInt16Mult", TestSafeUInt16Mult, NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeUInt32Mult", L"Common.IntSafe.Multiply.TestSafeUInt32Mult", TestSafeUInt32Mult, NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeUIntNMult",  L"Common.IntSafe.Multiply.TestSafeUIntNMult",  TestSafeUIntNMult,  NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeUInt64Mult", L"Common.IntSafe.Multiply.TestSafeUInt64Mult", TestSafeUInt64Mult, NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeInt8Mult",   L"Common.IntSafe.Multiply.TestSafeInt8Mult",   TestSafeInt8Mult,   NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeInt16Mult",  L"Common.IntSafe.Multiply.TestSafeInt16Mult",  TestSafeInt16Mult,  NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeInt32Mult",  L"Common.IntSafe.Multiply.TestSafeInt32Mult",  TestSafeInt32Mult,  NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeIntNMult",   L"Common.IntSafe.Multiply.TestSafeIntNMult",   TestSafeIntNMult,   NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeInt64Mult",  L"Common.IntSafe.Multiply.TestSafeInt64Mult",  TestSafeInt64Mult,  NULL, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites(Fw);

EXIT:
  if (Fw)
  {
    FreeUnitTestFramework(Fw);
  }

  return Status;
}