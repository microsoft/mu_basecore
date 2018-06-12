/** @file -- SampleUnitTestApp.c
This is a sample EFI Shell application to demostrate the usage of the Unit Test Library.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.


Copyright (C) 2016 Microsoft Corporation. All Rights Reserved.

**/
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <UnitTestTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestAssertLib.h>


#define UNIT_TEST_APP_NAME        L"Sample Unit Test Library Application"
#define UNIT_TEST_APP_SHORT_NAME  L"Sample_Unit_Test_Lib_App"
#define UNIT_TEST_APP_VERSION     L"0.1"


BOOLEAN       mSampleGlobalTestBoolean = FALSE;
VOID          *mSampleGlobalTestPointer = NULL;


///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================


//
// Anything you think might be helpful that isn't a test itself.
//

UNIT_TEST_STATUS
EFIAPI
MakeSureThatPointerIsNull (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UT_ASSERT_EQUAL(mSampleGlobalTestPointer,          NULL);
  return UNIT_TEST_PASSED;
} // ListsShouldHaveTheSameDescriptorSize()


VOID
EFIAPI
ClearThePointer (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  mSampleGlobalTestPointer = NULL;
  return;
} // ClearThePointer()


///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================


UNIT_TEST_STATUS
EFIAPI
OnePlusOneShouldEqualTwo (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UINTN     A, B, C;

  A = 1;
  B = 1;
  C = A + B;

  UT_ASSERT_EQUAL(C, 2);
  return UNIT_TEST_PASSED;
} // OnePlusOneShouldEqualTwo()


UNIT_TEST_STATUS
EFIAPI
GlobalBooleanShouldBeChangeable (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  mSampleGlobalTestBoolean = TRUE;
  UT_ASSERT_TRUE(mSampleGlobalTestBoolean);
  
  mSampleGlobalTestBoolean = FALSE;
  UT_ASSERT_FALSE(mSampleGlobalTestBoolean);

  return UNIT_TEST_PASSED;
} // GlobalBooleanShouldBeChangeable()


UNIT_TEST_STATUS
EFIAPI
GlobalPointerShouldBeChangeable (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  mSampleGlobalTestPointer = (VOID*)-1;
  UT_ASSERT_EQUAL(mSampleGlobalTestPointer, ((VOID*)-1));
  return UNIT_TEST_PASSED;
} // GlobalPointerShouldBeChangeable()


///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================


/** 
  SampleUnitTestApp
  
  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
SampleUnitTestApp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  UNIT_TEST_FRAMEWORK       *Fw = NULL;
  UNIT_TEST_SUITE           *SimpleMathTests, *GlobalVarTests;

  DEBUG(( DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, UNIT_TEST_APP_SHORT_NAME, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the SimpleMathTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &SimpleMathTests, Fw, L"Simple Math Tests", L"Sample.Math", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SimpleMathTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase( SimpleMathTests, L"Adding 1 to 1 should produce 2", L"Sample.Math.Addition", OnePlusOneShouldEqualTwo, NULL, NULL, NULL );

  //
  // Populate the GlobalVarTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &GlobalVarTests, Fw, L"Global Variable Tests", L"Sample.Globals", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for GlobalVarTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase( GlobalVarTests, L"You should be able to change a global BOOLEAN", L"Sample.Globals.Boolean", GlobalBooleanShouldBeChangeable, NULL, NULL, NULL );
  AddTestCase( GlobalVarTests, L"You should be able to change a global pointer", L"Sample.Globals.Pointer", GlobalPointerShouldBeChangeable, MakeSureThatPointerIsNull, ClearThePointer, NULL );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites( Fw );

EXIT:
  if (Fw)
  {
    FreeUnitTestFramework( Fw );
  }

  return Status;
}