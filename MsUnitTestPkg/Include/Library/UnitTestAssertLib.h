/** @file
Provides a unit test assert helpers.  This allows tests to focus on testing logic
and the library to handle common assertions. 


Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>

**/

#ifndef __UNIT_TEST_ASSERT_LIB_H__
#define __UNIT_TEST_ASSERT_LIB_H__


#define UT_ASSERT_TRUE(Expression)                \
  if(!UnitTestAssertTrue( Framework, (Expression), __FUNCTION__, __LINE__, #Expression )) { return UNIT_TEST_ERROR_TEST_FAILED;}

#define UT_ASSERT_FALSE(Expression)               \
  if(!UnitTestAssertFalse( Framework, (Expression), __FUNCTION__, __LINE__, #Expression )) { return UNIT_TEST_ERROR_TEST_FAILED;}

#define UT_ASSERT_EQUAL(ValueA, ValueB)           \
  if(!UnitTestAssertEqual( Framework, (UINTN)ValueA, (UINTN)ValueB, __FUNCTION__, __LINE__, #ValueA, #ValueB )) { return UNIT_TEST_ERROR_TEST_FAILED;}

#define UT_ASSERT_MEM_EQUAL(ValueA, ValueB, Length) \
  if(!UnitTestAssertMemEqual( Framework, (UINTN)ValueA, (UINTN)ValueB, (UINTN)Length, __FUNCTION__, __LINE__, #ValueA, #ValueB )) { return UNIT_TEST_ERROR_TEST_FAILED;}

#define UT_ASSERT_NOT_EQUAL(ValueA, ValueB)       \
  if(!UnitTestAssertNotEqual( Framework, ValueA, ValueB, __FUNCTION__, __LINE__, #ValueA, #ValueB )) { return UNIT_TEST_ERROR_TEST_FAILED;}

#define UT_ASSERT_NOT_EFI_ERROR(Status)           \
  if(!UnitTestAssertNotEfiError( Framework, Status, __FUNCTION__, __LINE__, #Status )) { return UNIT_TEST_ERROR_TEST_FAILED;}

#define UT_ASSERT_STATUS_EQUAL(Status, Expected)  \
  if(! UnitTestAssertStatusEqual( Framework, Status, Expected, __FUNCTION__, __LINE__, #Status )) { return UNIT_TEST_ERROR_TEST_FAILED;}

#define UT_ASSERT_NOT_NULL(Pointer)     \
  if(!UnitTestAssertNotNull(Framework, Pointer, __FUNCTION__, __LINE__, #Pointer)) { return UNIT_TEST_ERROR_TEST_FAILED; }


BOOLEAN
EFIAPI
UnitTestAssertTrue (
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN BOOLEAN                    Expression,
  IN CONST CHAR8                *FunctionName,
  IN UINTN                      LineNumber,
  IN CONST CHAR8                *Description
  );

BOOLEAN
EFIAPI
UnitTestAssertFalse (
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN BOOLEAN                    Expression,
  IN CONST CHAR8                *FunctionName,
  IN UINTN                      LineNumber,
  IN CONST CHAR8                *Description
  );

BOOLEAN
EFIAPI
UnitTestAssertNotEfiError (
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN EFI_STATUS                 Status,
  IN CONST CHAR8                *FunctionName,
  IN UINTN                      LineNumber,
  IN CONST CHAR8                *Description
  );

BOOLEAN
EFIAPI
UnitTestAssertEqual (
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN UINTN                      ValueA,
  IN UINTN                      ValueB,
  IN CONST CHAR8                *FunctionName,
  IN UINTN                      LineNumber,
  IN CONST CHAR8                *DescriptionA,
  IN CONST CHAR8                *DescriptionB
  );

BOOLEAN
EFIAPI
UnitTestAssertMemEqual(
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN UINTN                      ValueA,
  IN UINTN                      ValueB,
  IN UINTN                      Length,
  IN CONST CHAR8                *FunctionName,
  IN UINTN                      LineNumber,
  IN CONST CHAR8                *DescriptionA,
  IN CONST CHAR8                *DescriptionB
);

BOOLEAN
EFIAPI
UnitTestAssertNotEqual (
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN UINTN                      ValueA,
  IN UINTN                      ValueB,
  IN CONST CHAR8                *FunctionName,
  IN UINTN                      LineNumber,
  IN CONST CHAR8                *DescriptionA,
  IN CONST CHAR8                *DescriptionB
  );

BOOLEAN
EFIAPI
UnitTestAssertStatusEqual (
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN EFI_STATUS                 Status,
  IN EFI_STATUS                 Expected,
  IN CONST CHAR8                *FunctionName,
  IN UINTN                      LineNumber,
  IN CONST CHAR8                *Description
  );

BOOLEAN
EFIAPI
UnitTestAssertNotNull(
  IN UNIT_TEST_FRAMEWORK_HANDLE Framework,
  IN VOID*                      Pointer,
  IN CONST CHAR8                *FunctionName,
  IN UINTN                      LineNumber,
  IN CONST CHAR8                *PointerName
);

#endif