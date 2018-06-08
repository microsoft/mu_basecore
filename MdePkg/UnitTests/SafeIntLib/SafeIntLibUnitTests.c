/**
@file
UEFI Shell based application for unit testing the SafeIntLib.


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

#include "SafeIntLibUnitTests.h"

#define UNIT_TEST_APP_NAME        L"Int Safe Lib Unit Test Application"
#define UNIT_TEST_APP_VERSION     L"0.1"

//
// Conversion function tests:
//
UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUint8(
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
  Status = SafeInt8ToUint8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUint16(
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
  Status = SafeInt8ToUint16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUint32(
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
  Status = SafeInt8ToUint32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUint32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUintn(
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
  Status = SafeInt8ToUintn(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUintn(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt8ToUint64(
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
  Status = SafeInt8ToUint64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Negative number should result in an error status
  //
  Operand = (-56);
  Status = SafeInt8ToUint64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint8ToInt8(
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
  Status = SafeUint8ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Operand larger than 0x7f should result in an error status
  //
  Operand = 0xaf;
  Status = SafeUint8ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint8ToChar8(
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
  Status = SafeUint8ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Operand larger than 0x7f should result in an error status
  //
  Operand = 0xaf;
  Status = SafeUint8ToChar8(Operand, &Result);
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
TestSafeInt16ToUint8(
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
  Status = SafeInt16ToUint8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = 0x1234;
  Status = SafeInt16ToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-17835);
  Status = SafeInt16ToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToUint16(
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
  Status = SafeInt16ToUint16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17835);
  Status = SafeInt16ToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToUint32(
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
  Status = SafeInt16ToUint32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17835);
  Status = SafeInt16ToUint32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToUintN(
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
  Status = SafeInt16ToUintN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17835);
  Status = SafeInt16ToUintN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt16ToUint64(
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
  Status = SafeInt16ToUint64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17835);
  Status = SafeInt16ToUint64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint16ToInt8(
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
  Status = SafeUint16ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5b5b);
  Status = SafeUint16ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint16ToChar8(
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
  Status = SafeUint16ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5b5b);
  Status = SafeUint16ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint16ToUint8(
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
  Status = SafeUint16ToUint8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5b5b);
  Status = SafeUint16ToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint16ToInt16(
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
  Status = SafeUint16ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUint16ToInt16(Operand, &Result);
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
TestSafeInt32ToUint8(
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
  Status = SafeInt32ToUint8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-57);
  Status = SafeInt32ToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (0x5bababab);
  Status = SafeInt32ToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeInt32ToUint8(Operand, &Result);
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
TestSafeInt32ToUint16(
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
  Status = SafeInt32ToUint16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-17857);
  Status = SafeInt32ToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (0x5bababab);
  Status = SafeInt32ToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeInt32ToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToUint32(
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
  Status = SafeInt32ToUint32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeInt32ToUint32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToUint64(
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
  Status = SafeInt32ToUint64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeInt32ToUint64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint32ToInt8(
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
  Status = SafeUint32ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeUint32ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint32ToChar8(
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
  Status = SafeUint32ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeUint32ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint32ToUint8(
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
  Status = SafeUint32ToUint8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUint32ToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint32ToInt16(
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
  Status = SafeUint32ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUint32ToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint32ToUint16(
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
  Status = SafeUint32ToUint16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUint32ToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint32ToInt32(
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
  Status = SafeUint32ToInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUint32ToInt32(Operand, &Result);
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
TestSafeIntNToUint8(
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
  Status = SafeIntNToUint8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeIntNToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeIntNToUint8(Operand, &Result);
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
TestSafeIntNToUint16(
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
  Status = SafeIntNToUint16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5bababab);
  Status = SafeIntNToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand = (-1537977259);
  Status = SafeIntNToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToUintN(
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
  Status = SafeIntNToUintN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeIntNToUintN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToUint64(
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
  Status = SafeIntNToUint64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeIntNToUint64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUintNToInt8(
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
  Status = SafeUintNToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUintNToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUintNToChar8(
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
  Status = SafeUintNToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUintNToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUintnToUint8(
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
  Status = SafeUintnToUint8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUintnToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUintNToInt16(
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
  Status = SafeUintNToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabab);
  Status = SafeUintNToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUintNToUint16(
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
  Status = SafeUintNToUint16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUintNToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUintNToInt32(
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
  Status = SafeUintNToInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUintNToInt32(Operand, &Result);
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
TestSafeInt64ToUint8(
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
  Status = SafeInt64ToUint8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToUint8(Operand, &Result);
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
TestSafeInt64ToUint16(
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
  Status = SafeInt64ToUint16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToUint16(Operand, &Result);
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
TestSafeInt64ToUint32(
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
  Status = SafeInt64ToUint32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToUint32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToUint32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToUint64(
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
  Status = SafeInt64ToUint64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5babababefefefef, Result);

  //
  // Otherwise should result in an error status
  //
  Operand =  (-6605562033422200815);
  Status = SafeInt64ToUint64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64ToInt8(
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
  Status = SafeUint64ToInt8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUint64ToInt8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64ToChar8(
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
  Status = SafeUint64ToChar8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5b, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUint64ToChar8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64ToUint8(
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
  Status = SafeUint64ToUint8(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUint64ToUint8(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64ToInt16(
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
  Status = SafeUint64ToInt16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUint64ToInt16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64ToUint16(
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
  Status = SafeUint64ToUint16(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUint64ToUint16(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64ToInt32(
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
  Status = SafeUint64ToInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUint64ToInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64ToUint32(
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
  Status = SafeUint64ToUint32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUint64ToUint32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64ToInt64(
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
  Status = SafeUint64ToInt64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5babababefefefef, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUint64ToInt64(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

//
// Addition function tests:
//
UNIT_TEST_STATUS
EFIAPI
TestSafeUint8Add(
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
  Status = SafeUint8Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x74, Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0xab;
  Addend = 0xbc;
  Status = SafeUint8Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint16Add(
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
  Status = SafeUint16Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x7474, Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0xabab;
  Addend = 0xbcbc;
  Status = SafeUint16Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint32Add(
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
  Status = SafeUint32Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x74747474, Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0xabababab;
  Addend = 0xbcbcbcbc;
  Status = SafeUint32Add(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64Add(
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
  Status = SafeUint64Add(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x7474747424242424, Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0xababababefefefef;
  Addend = 0xbcbcbcbcdededede;
  Status = SafeUint64Add(Augend, Addend, &Result);
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
TestSafeUint8Sub(
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
  Status = SafeUint8Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x1f, Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = 0x5a;
  Subtrahend = 0x6d;
  Status = SafeUint8Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint16Sub(
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
  Status = SafeUint16Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x1f1f, Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = 0x5a5a;
  Subtrahend = 0x6d6d;
  Status = SafeUint16Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint32Sub(
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
  Status = SafeUint32Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x1f1f1f1f, Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = 0x5a5a5a5a;
  Subtrahend = 0x6d6d6d6d;
  Status = SafeUint32Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64Sub(
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
  Status = SafeUint64Sub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x1f1f1f1f1f1f1f1f, Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = 0x5a5a5a5a5a5a5a5a;
  Subtrahend = 0x6d6d6d6d6d6d6d6d;
  Status = SafeUint64Sub(Minuend, Subtrahend, &Result);
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
TestSafeUint8Mult(
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
  Status = SafeUint8Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xb4, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x12;
  Multiplier = 0x23;
  Status = SafeUint8Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint16Mult(
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
  Status = SafeUint16Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xfc94, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x1234;
  Multiplier = 0x213;
  Status = SafeUint16Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint32Mult(
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
  Status = SafeUint32Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x844c9dbe, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0xa122a;
  Multiplier = 0xed23;
  Status = SafeUint32Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUint64Mult(
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
  Status = SafeUint64Mult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x14b66db9745a07f6, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x123456789a;
  Multiplier = 0x12345678;
  Status = SafeUint64Mult(Multiplicand, Multiplier, &Result);
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

  ConversionTestSuite = NULL;
  AdditionSubtractionTestSuite = NULL;
  MultiplicationTestSuite = NULL; 

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
  Status = CreateUnitTestSuite(&ConversionTestSuite, Fw, L"Int Safe Conversions Test Suite", L"Common.SafeInt.Convert", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Conversions Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUint8",    L"Common.SafeInt.Convert.TestSafeInt8ToUint8",    TestSafeInt8ToUint8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUint16",   L"Common.SafeInt.Convert.TestSafeInt8ToUint16",   TestSafeInt8ToUint16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUint32",   L"Common.SafeInt.Convert.TestSafeInt8ToUint32",   TestSafeInt8ToUint32,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUintn",    L"Common.SafeInt.Convert.TestSafeInt8ToUintn",    TestSafeInt8ToUintn,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt8ToUint64",   L"Common.SafeInt.Convert.TestSafeInt8ToUint64",   TestSafeInt8ToUint64,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint8ToInt8",    L"Common.SafeInt.Convert.TestSafeUint8ToInt8",    TestSafeUint8ToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint8ToChar8",   L"Common.SafeInt.Convert.TestSafeUint8ToChar8",   TestSafeUint8ToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToInt8",    L"Common.SafeInt.Convert.TestSafeInt16ToInt8",    TestSafeInt16ToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToChar8",   L"Common.SafeInt.Convert.TestSafeInt16ToChar8",   TestSafeInt16ToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUint8",   L"Common.SafeInt.Convert.TestSafeInt16ToUint8",   TestSafeInt16ToUint8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUint16",  L"Common.SafeInt.Convert.TestSafeInt16ToUint16",  TestSafeInt16ToUint16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUint32",  L"Common.SafeInt.Convert.TestSafeInt16ToUint32",  TestSafeInt16ToUint32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUintN",   L"Common.SafeInt.Convert.TestSafeInt16ToUintN",   TestSafeInt16ToUintN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt16ToUint64",  L"Common.SafeInt.Convert.TestSafeInt16ToUint64",  TestSafeInt16ToUint64,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint16ToInt8",   L"Common.SafeInt.Convert.TestSafeUint16ToInt8",   TestSafeUint16ToInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint16ToChar8",  L"Common.SafeInt.Convert.TestSafeUint16ToChar8",  TestSafeUint16ToChar8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint16ToUint8",  L"Common.SafeInt.Convert.TestSafeUint16ToUint8",  TestSafeUint16ToUint8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint16ToInt16",  L"Common.SafeInt.Convert.TestSafeUint16ToInt16",  TestSafeUint16ToInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToInt8",    L"Common.SafeInt.Convert.TestSafeInt32ToInt8",    TestSafeInt32ToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToChar8",   L"Common.SafeInt.Convert.TestSafeInt32ToChar8",   TestSafeInt32ToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUint8",   L"Common.SafeInt.Convert.TestSafeInt32ToUint8",   TestSafeInt32ToUint8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToInt16",   L"Common.SafeInt.Convert.TestSafeInt32ToInt16",   TestSafeInt32ToInt16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUint16",  L"Common.SafeInt.Convert.TestSafeInt32ToUint16",  TestSafeInt32ToUint16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUint32",  L"Common.SafeInt.Convert.TestSafeInt32ToUint32",  TestSafeInt32ToUint32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUintN",   L"Common.SafeInt.Convert.TestSafeInt32ToUintN",   TestSafeInt32ToUintN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt32ToUint64",  L"Common.SafeInt.Convert.TestSafeInt32ToUint64",  TestSafeInt32ToUint64,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint32ToInt8",   L"Common.SafeInt.Convert.TestSafeUint32ToInt8",   TestSafeUint32ToInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint32ToChar8",  L"Common.SafeInt.Convert.TestSafeUint32ToChar8",  TestSafeUint32ToChar8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint32ToUint8",  L"Common.SafeInt.Convert.TestSafeUint32ToUint8",  TestSafeUint32ToUint8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint32ToInt16",  L"Common.SafeInt.Convert.TestSafeUint32ToInt16",  TestSafeUint32ToInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint32ToUint16", L"Common.SafeInt.Convert.TestSafeUint32ToUint16", TestSafeUint32ToUint16, NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint32ToInt32",  L"Common.SafeInt.Convert.TestSafeUint32ToInt32",  TestSafeUint32ToInt32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint32ToIntN",   L"Common.SafeInt.Convert.TestSafeUint32ToIntN",   TestSafeUint32ToIntN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToInt8",     L"Common.SafeInt.Convert.TestSafeIntNToInt8",     TestSafeIntNToInt8,     NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToChar8",    L"Common.SafeInt.Convert.TestSafeIntNToChar8",    TestSafeIntNToChar8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUint8",    L"Common.SafeInt.Convert.TestSafeIntNToUint8",    TestSafeIntNToUint8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToInt16",    L"Common.SafeInt.Convert.TestSafeIntNToInt16",    TestSafeIntNToInt16,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUint16",   L"Common.SafeInt.Convert.TestSafeIntNToUint16",   TestSafeIntNToUint16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToInt32",    L"Common.SafeInt.Convert.TestSafeIntNToInt32",    TestSafeIntNToInt32,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUint32",   L"Common.SafeInt.Convert.TestSafeIntNToUint32",   TestSafeIntNToUint32,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUintN",    L"Common.SafeInt.Convert.TestSafeIntNToUintN",    TestSafeIntNToUintN,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeIntNToUint64",   L"Common.SafeInt.Convert.TestSafeIntNToUint64",   TestSafeIntNToUint64,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUintNToInt8",    L"Common.SafeInt.Convert.TestSafeUintNToInt8",    TestSafeUintNToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUintNToChar8",   L"Common.SafeInt.Convert.TestSafeUintNToChar8",   TestSafeUintNToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUintnToUint8",   L"Common.SafeInt.Convert.TestSafeUintnToUint8",   TestSafeUintnToUint8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUintNToInt16",   L"Common.SafeInt.Convert.TestSafeUintNToInt16",   TestSafeUintNToInt16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUintNToUint16",  L"Common.SafeInt.Convert.TestSafeUintNToUint16",  TestSafeUintNToUint16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUintNToInt32",   L"Common.SafeInt.Convert.TestSafeUintNToInt32",   TestSafeUintNToInt32,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUintNToUint32",  L"Common.SafeInt.Convert.TestSafeUintNToUint32",  TestSafeUintNToUint32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUintNToIntN",    L"Common.SafeInt.Convert.TestSafeUintNToIntN",    TestSafeUintNToIntN,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUintNToInt64",   L"Common.SafeInt.Convert.TestSafeUintNToInt64",   TestSafeUintNToInt64,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToInt8",    L"Common.SafeInt.Convert.TestSafeInt64ToInt8",    TestSafeInt64ToInt8,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToChar8",   L"Common.SafeInt.Convert.TestSafeInt64ToChar8",   TestSafeInt64ToChar8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUint8",   L"Common.SafeInt.Convert.TestSafeInt64ToUint8",   TestSafeInt64ToUint8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToInt16",   L"Common.SafeInt.Convert.TestSafeInt64ToInt16",   TestSafeInt64ToInt16,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUint16",  L"Common.SafeInt.Convert.TestSafeInt64ToUint16",  TestSafeInt64ToUint16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToInt32",   L"Common.SafeInt.Convert.TestSafeInt64ToInt32",   TestSafeInt64ToInt32,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUint32",  L"Common.SafeInt.Convert.TestSafeInt64ToUint32",  TestSafeInt64ToUint32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToIntN",    L"Common.SafeInt.Convert.TestSafeInt64ToIntN",    TestSafeInt64ToIntN,    NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUintN",   L"Common.SafeInt.Convert.TestSafeInt64ToUintN",   TestSafeInt64ToUintN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeInt64ToUint64",  L"Common.SafeInt.Convert.TestSafeInt64ToUint64",  TestSafeInt64ToUint64,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToInt8",   L"Common.SafeInt.Convert.TestSafeUint64ToInt8",   TestSafeUint64ToInt8,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToChar8",  L"Common.SafeInt.Convert.TestSafeUint64ToChar8",  TestSafeUint64ToChar8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToUint8",  L"Common.SafeInt.Convert.TestSafeUint64ToUint8",  TestSafeUint64ToUint8,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToInt16",  L"Common.SafeInt.Convert.TestSafeUint64ToInt16",  TestSafeUint64ToInt16,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToUint16", L"Common.SafeInt.Convert.TestSafeUint64ToUint16", TestSafeUint64ToUint16, NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToInt32",  L"Common.SafeInt.Convert.TestSafeUint64ToInt32",  TestSafeUint64ToInt32,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToUint32", L"Common.SafeInt.Convert.TestSafeUint64ToUint32", TestSafeUint64ToUint32, NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToIntN",   L"Common.SafeInt.Convert.TestSafeUint64ToIntN",   TestSafeUint64ToIntN,   NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToUintN",  L"Common.SafeInt.Convert.TestSafeUint64ToUintN",  TestSafeUint64ToUintN,  NULL, NULL, NULL);
  AddTestCase(ConversionTestSuite, L"Test SafeUint64ToInt64",  L"Common.SafeInt.Convert.TestSafeUint64ToInt64",  TestSafeUint64ToInt64,  NULL, NULL, NULL);

  //
  // Test the addition and subtraction functions
  //
  Status = CreateUnitTestSuite(&AdditionSubtractionTestSuite, Fw, L"Int Safe Add/Subtract Test Suite", L"Common.SafeInt.AddSubtract", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Int Safe Add/Subtract Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUint8Add",  L"Common.SafeInt.AddSubtract.TestSafeUint8Add",  TestSafeUint8Add,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUint16Add", L"Common.SafeInt.AddSubtract.TestSafeUint16Add", TestSafeUint16Add, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUint32Add", L"Common.SafeInt.AddSubtract.TestSafeUint32Add", TestSafeUint32Add, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUintNAdd",  L"Common.SafeInt.AddSubtract.TestSafeUintNAdd",  TestSafeUintNAdd,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUint64Add", L"Common.SafeInt.AddSubtract.TestSafeUint64Add", TestSafeUint64Add, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt8Add",   L"Common.SafeInt.AddSubtract.TestSafeInt8Add",   TestSafeInt8Add,   NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt16Add",  L"Common.SafeInt.AddSubtract.TestSafeInt16Add",  TestSafeInt16Add,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt32Add",  L"Common.SafeInt.AddSubtract.TestSafeInt32Add",  TestSafeInt32Add,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeIntNAdd",   L"Common.SafeInt.AddSubtract.TestSafeIntNAdd",   TestSafeIntNAdd,   NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt64Add",  L"Common.SafeInt.AddSubtract.TestSafeInt64Add",  TestSafeInt64Add,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUint8Sub",  L"Common.SafeInt.AddSubtract.TestSafeUint8Sub",  TestSafeUint8Sub,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUint16Sub", L"Common.SafeInt.AddSubtract.TestSafeUint16Sub", TestSafeUint16Sub, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUint32Sub", L"Common.SafeInt.AddSubtract.TestSafeUint32Sub", TestSafeUint32Sub, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUintNSub",  L"Common.SafeInt.AddSubtract.TestSafeUintNSub",  TestSafeUintNSub,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeUint64Sub", L"Common.SafeInt.AddSubtract.TestSafeUint64Sub", TestSafeUint64Sub, NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt8Sub",   L"Common.SafeInt.AddSubtract.TestSafeInt8Sub",   TestSafeInt8Sub,   NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt16Sub",  L"Common.SafeInt.AddSubtract.TestSafeInt16Sub",  TestSafeInt16Sub,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt32Sub",  L"Common.SafeInt.AddSubtract.TestSafeInt32Sub",  TestSafeInt32Sub,  NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeIntNSub",   L"Common.SafeInt.AddSubtract.TestSafeIntNSub",   TestSafeIntNSub,   NULL, NULL, NULL);
  AddTestCase(AdditionSubtractionTestSuite, L"Test SafeInt64Sub",  L"Common.SafeInt.AddSubtract.TestSafeInt64Sub",  TestSafeInt64Sub,  NULL, NULL, NULL);

  //
  // Test the multiplication functions
  //
  Status = CreateUnitTestSuite(&MultiplicationTestSuite, Fw, L"Int Safe Multiply Test Suite", L"Common.SafeInt.Multiply", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Int Safe Multiply Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase(MultiplicationTestSuite, L"Test SafeUint8Mult",  L"Common.SafeInt.Multiply.TestSafeUint8Mult",  TestSafeUint8Mult,  NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeUint16Mult", L"Common.SafeInt.Multiply.TestSafeUint16Mult", TestSafeUint16Mult, NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeUint32Mult", L"Common.SafeInt.Multiply.TestSafeUint32Mult", TestSafeUint32Mult, NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeUintNMult",  L"Common.SafeInt.Multiply.TestSafeUintNMult",  TestSafeUintNMult,  NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeUint64Mult", L"Common.SafeInt.Multiply.TestSafeUint64Mult", TestSafeUint64Mult, NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeInt8Mult",   L"Common.SafeInt.Multiply.TestSafeInt8Mult",   TestSafeInt8Mult,   NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeInt16Mult",  L"Common.SafeInt.Multiply.TestSafeInt16Mult",  TestSafeInt16Mult,  NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeInt32Mult",  L"Common.SafeInt.Multiply.TestSafeInt32Mult",  TestSafeInt32Mult,  NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeIntNMult",   L"Common.SafeInt.Multiply.TestSafeIntNMult",   TestSafeIntNMult,   NULL, NULL, NULL);
  AddTestCase(MultiplicationTestSuite, L"Test SafeInt64Mult",  L"Common.SafeInt.Multiply.TestSafeInt64Mult",  TestSafeInt64Mult,  NULL, NULL, NULL);

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