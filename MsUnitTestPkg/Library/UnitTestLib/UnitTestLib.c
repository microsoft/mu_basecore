/**

Implement UnitTestLib 

Copyright (c) Microsoft
**/

#include <PiDxe.h>
#include <UnitTestTypes.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestBootUsbLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestPersistenceLib.h>
#include <Library/UnitTestResultReportLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "Md5.h"

MD5_CTX     mFingerprintCtx;


// Prototyped here so that it can be included near the functions that
// it logically goes with.
STATIC
VOID
UpdateTestFromSave (
  IN OUT UNIT_TEST              *Test,
  IN     UNIT_TEST_SAVE_HEADER  *SavedState
  );


//=============================================================================
//
// ----------------  TEST HELPER FUNCTIONS ------------------------------------
//
//=============================================================================


/**
  This function will determine whether the short name violates any rules that would
  prevent it from being used as a reporting name or as a serialization name.

  Example: If the name cannot be serialized to a filesystem file name.

  @param[in]  ShortTitleString  A pointer to the short title string to be evaluated.

  @retval     TRUE    The string is acceptable.
  @retval     FALSE   The string should not be used.

**/
STATIC
BOOLEAN
IsFrameworkShortNameValid (
  IN  CHAR16    *ShortTitleString
  )
{
  // TODO: Finish this function.
  return TRUE;
} // IsFrameworkShortNameValid()


STATIC
CHAR16*
AllocateAndCopyString (
  IN  CHAR16    *StringToCopy
  )
{
  CHAR16    *NewString = NULL;
  UINTN     NewStringLength;

  NewStringLength = StrnLenS( StringToCopy, UNIT_TEST_MAX_STRING_LENGTH ) + 1;
  NewString = AllocatePool( NewStringLength * sizeof( CHAR16 ) );
  if (NewString != NULL)
  {
    StrCpyS( NewString, NewStringLength, StringToCopy );
  }

  return NewString;
} // AllocateAndCopyString ()


STATIC
VOID
SetFrameworkFingerprint (
  OUT UINT8                 *Fingerprint,
  IN  UNIT_TEST_FRAMEWORK   *Framework
  )
{
  MD5Init( &mFingerprintCtx );

  // For this one we'll just use the title and version as the unique fingerprint.
  MD5Update( &mFingerprintCtx, Framework->Title, (StrLen( Framework->Title ) * sizeof( CHAR16 )) );
  MD5Update( &mFingerprintCtx, Framework->VersionString, (StrLen( Framework->VersionString ) * sizeof( CHAR16 )) );

  MD5Final( &mFingerprintCtx, &Framework->Fingerprint[0] );
  return;
} // SetFrameworkFingerprint()


STATIC
VOID
SetSuiteFingerprint (
  OUT UINT8                 *Fingerprint,
  IN  UNIT_TEST_FRAMEWORK   *Framework,
  IN  UNIT_TEST_SUITE       *Suite
  )
{
  MD5Init( &mFingerprintCtx );

  // For this one, we'll use the fingerprint from the framework, and the title of the suite.
  MD5Update( &mFingerprintCtx, &Framework->Fingerprint[0], UNIT_TEST_FINGERPRINT_SIZE );
  MD5Update( &mFingerprintCtx, Suite->Title, (StrLen( Suite->Title ) * sizeof( CHAR16 )) );
  MD5Update(&mFingerprintCtx, Suite->Package, (StrLen(Suite->Package) * sizeof(CHAR16)));

  MD5Final( &mFingerprintCtx, &Suite->Fingerprint[0] );
  return;
} // SetSuiteFingerprint()


STATIC
VOID
SetTestFingerprint (
  OUT UINT8                 *Fingerprint,
  IN  UNIT_TEST_SUITE       *Suite,
  IN  UNIT_TEST             *Test
  )
{
  MD5Init( &mFingerprintCtx );

  // For this one, we'll use the fingerprint from the suite, and the description and classname of the test.
  MD5Update( &mFingerprintCtx, &Suite->Fingerprint[0], UNIT_TEST_FINGERPRINT_SIZE );
  MD5Update( &mFingerprintCtx, Test->Description, (StrLen( Test->Description ) * sizeof( CHAR16 )) );
  MD5Update(&mFingerprintCtx, Test->ClassName, (StrLen(Test->ClassName) * sizeof(CHAR16)));

  MD5Final( &mFingerprintCtx, &Test->Fingerprint[0] );
  return;
} // SetTestFingerprint()


STATIC
BOOLEAN
CompareFingerprints (
  IN  UINT8       *FingerprintA,
  IN  UINT8       *FingerprintB
  )
{
  return (CompareMem( FingerprintA, FingerprintB, UNIT_TEST_FINGERPRINT_SIZE ) == 0);
} // SetTestFingerprint()


EFI_STATUS
EFIAPI
FreeUnitTestFramework (
  IN UNIT_TEST_FRAMEWORK  *Framework
  )
{
  // TODO: Finish this function.
  return EFI_SUCCESS;
} // FreeUnitTestFramework()


STATIC
EFI_STATUS
FreeUnitTestSuiteEntry (
  IN UNIT_TEST_SUITE_LIST_ENTRY *SuiteEntry
  )
{
  // TODO: Finish this function.
  return EFI_SUCCESS;
} // FreeUnitTestSuiteEntry()


STATIC
EFI_STATUS
FreeUnitTestTestEntry (
  IN UNIT_TEST_LIST_ENTRY *TestEntry
  )
{
  // TODO: Finish this function.
  return EFI_SUCCESS;
} // FreeUnitTestTestEntry()


//=============================================================================
//
// ----------------  TEST SETUP FUNCTIONS -------------------------------------
//
//=============================================================================


/*
Method to Initialize the Unit Test framework

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
  )
{
  EFI_STATUS                Status = EFI_SUCCESS;
  UNIT_TEST_FRAMEWORK       *NewFramework = NULL;

  //
  // First, check all pointers and make sure nothing's broked.
  if (Framework == NULL || Title == NULL ||
      ShortTitle == NULL || VersionString == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Next, determine whether all of the strings are good to use.
  if (!IsFrameworkShortNameValid( ShortTitle ))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Next, set aside some space to start messing with the framework.
  NewFramework = AllocateZeroPool( sizeof( UNIT_TEST_FRAMEWORK ) );
  if (NewFramework == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Next, set up all the test data.
  NewFramework->Title         = AllocateAndCopyString( Title );
  NewFramework->ShortTitle    = AllocateAndCopyString( ShortTitle );
  NewFramework->VersionString = AllocateAndCopyString( VersionString );
  NewFramework->Log           = NULL;
  NewFramework->CurrentTest   = NULL;
  NewFramework->SavedState    = NULL;
  if (NewFramework->Title == NULL || NewFramework->ShortTitle == NULL ||
      NewFramework->VersionString == NULL)
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  InitializeListHead( &(NewFramework->TestSuiteList) );

  //
  // Create the framework fingerprint.
  SetFrameworkFingerprint( &NewFramework->Fingerprint[0], NewFramework );

  //
  // If there is a persisted context, load it now.
  if (DoesCacheExist( NewFramework ))
  {
    Status = LoadUnitTestCache( NewFramework, &(UNIT_TEST_SAVE_HEADER*)(NewFramework->SavedState) );
    if (EFI_ERROR( Status ))
    {
      // Don't actually report it as an error, but emit a warning.
      DEBUG(( DEBUG_ERROR, __FUNCTION__" - Cache was detected, but failed to load.\n" ));
      Status = EFI_SUCCESS;
    }
  }

Exit:
  //
  // If we're good, then let's copy the framework.
  if (!EFI_ERROR( Status ))
  {
    *Framework = NewFramework;
  }
  // Otherwise, we need to undo this horrible thing that we've done.
  else
  {
    FreeUnitTestFramework( NewFramework );
  }

  return Status;
}


EFI_STATUS
EFIAPI
CreateUnitTestSuite (
  OUT UNIT_TEST_SUITE           **Suite,
  IN UNIT_TEST_FRAMEWORK        *Framework,
  IN CHAR16                     *Title,
  IN CHAR16                     *Package,
  IN UNIT_TEST_SUITE_SETUP      Sup    OPTIONAL,
  IN UNIT_TEST_SUITE_TEARDOWN   Tdn    OPTIONAL
  )
{
  EFI_STATUS                    Status = EFI_SUCCESS;
  UNIT_TEST_SUITE_LIST_ENTRY    *NewSuiteEntry;

  //
  // First, let's check to make sure that our parameters look good.
  if ((Framework == NULL) || (Title == NULL) || (Package == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Create the new entry.
  NewSuiteEntry = AllocateZeroPool( sizeof( UNIT_TEST_SUITE_LIST_ENTRY ) );
  if (NewSuiteEntry == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the fields we think we need.
  NewSuiteEntry->UTS.Title            = AllocateAndCopyString( Title );
  NewSuiteEntry->UTS.Package          = AllocateAndCopyString(Package);
  NewSuiteEntry->UTS.Setup            = Sup;
  NewSuiteEntry->UTS.Teardown         = Tdn;
  NewSuiteEntry->UTS.ParentFramework  = Framework;
  InitializeListHead( &(NewSuiteEntry->Entry) );             // List entry for sibling suites.
  InitializeListHead( &(NewSuiteEntry->UTS.TestCaseList) );  // List entry for child tests.
  if (NewSuiteEntry->UTS.Title == NULL)
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  if (NewSuiteEntry->UTS.Package == NULL)
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Create the suite fingerprint.
  SetSuiteFingerprint( &NewSuiteEntry->UTS.Fingerprint[0], Framework, &NewSuiteEntry->UTS );

Exit:
  //
  // If everything is going well, add the new suite to the tail list for the framework.
  if (!EFI_ERROR( Status ))
  {
    InsertTailList( &(Framework->TestSuiteList), (LIST_ENTRY*)NewSuiteEntry );
    *Suite = &NewSuiteEntry->UTS;
  }
  // Otherwise, make with the destruction.
  else
  {
    FreeUnitTestSuiteEntry( NewSuiteEntry );
  }

  return Status;
}


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
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  UNIT_TEST_LIST_ENTRY  *NewTestEntry;
  UNIT_TEST_FRAMEWORK   *ParentFramework = (UNIT_TEST_FRAMEWORK*)Suite->ParentFramework;

  //
  // First, let's check to make sure that our parameters look good.
  if ((Suite == NULL) || (Description == NULL) || (ClassName == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Create the new entry.
  NewTestEntry = AllocateZeroPool( sizeof( UNIT_TEST_LIST_ENTRY ) );
  if (NewTestEntry == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Copy the fields we think we need.
  NewTestEntry->UT.Description  = AllocateAndCopyString( Description );
  NewTestEntry->UT.ClassName    = AllocateAndCopyString(ClassName);
  NewTestEntry->UT.FailureType  = FAILURETYPE_NOFAILURE;
  NewTestEntry->UT.FailureMessage[0] = '\0';
  NewTestEntry->UT.Log          = NULL;
  NewTestEntry->UT.PreReq       = PreReq;
  NewTestEntry->UT.CleanUp      = CleanUp;
  NewTestEntry->UT.RunTest      = Func;
  NewTestEntry->UT.Context      = Context;
  NewTestEntry->UT.Result       = UNIT_TEST_PENDING;
  NewTestEntry->UT.ParentSuite  = Suite;
  InitializeListHead( &(NewTestEntry->Entry) );      // List entry for sibling tests.
  if (NewTestEntry->UT.Description == NULL)
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Create the test fingerprint.
  SetTestFingerprint( &NewTestEntry->UT.Fingerprint[0], Suite, &NewTestEntry->UT );

  // TODO: Make sure that duplicate fingerprints cannot be created.

  //
  // If there is saved test data, update this record.
  if (ParentFramework->SavedState != NULL)
  {
    UpdateTestFromSave( &NewTestEntry->UT, ParentFramework->SavedState );
  }

Exit:
  //
  // If everything is going well, add the new suite to the tail list for the framework.
  if (!EFI_ERROR( Status ))
  {
    InsertTailList( &(Suite->TestCaseList), (LIST_ENTRY*)NewTestEntry );
  }
  // Otherwise, make with the destruction.
  else
  {
    FreeUnitTestTestEntry( NewTestEntry );
  }

  return Status;
}


//=============================================================================
//
// ----------------  TEST EXECUTION FUNCTIONS ---------------------------------
//
//=============================================================================

STATIC
EFI_STATUS
RunTestSuite (
  IN UNIT_TEST_SUITE      *Suite
  )
{
  UNIT_TEST_LIST_ENTRY  *TestEntry = NULL;
  UNIT_TEST             *Test;
  UNIT_TEST_FRAMEWORK   *ParentFramework = (UNIT_TEST_FRAMEWORK*)Suite->ParentFramework;

  if (Suite == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG((DEBUG_VERBOSE, "---------------------------------------------------------\n"));
  DEBUG((DEBUG_VERBOSE, "RUNNING TEST SUITE: %s\n", Suite->Title));
  DEBUG((DEBUG_VERBOSE, "---------------------------------------------------------\n"));

  if (Suite->Setup != NULL)
  {
    Suite->Setup( Suite->ParentFramework );
  }

  //
  // Iterate all tests within the suite
  //
  for (TestEntry = (UNIT_TEST_LIST_ENTRY*)GetFirstNode( &(Suite->TestCaseList) );                         // Start at the beginning.
       (LIST_ENTRY*)TestEntry != &(Suite->TestCaseList);                                                  // Go until you loop back to the head.
       TestEntry = (UNIT_TEST_LIST_ENTRY*)GetNextNode( &(Suite->TestCaseList), (LIST_ENTRY*)TestEntry) )  // Always get the next test.
  {
    Test                          = &TestEntry->UT;
    ParentFramework->CurrentTest  = Test;

    DEBUG((DEBUG_VERBOSE, "*********************************************************\n"));
    DEBUG((DEBUG_VERBOSE, " RUNNING TEST: %s:\n", Test->Description));
    DEBUG((DEBUG_VERBOSE, "**********************************************************\n"));

    //
    // First, check to see whether the test has already been run.
    // NOTE: This would generally only be the case if a saved state was detected and loaded.
    if (Test->Result != UNIT_TEST_PENDING && Test->Result != UNIT_TEST_RUNNING)
    {
      DEBUG(( DEBUG_VERBOSE, "Test was run on a previous pass. Skipping.\n" ));
      ParentFramework->CurrentTest  = NULL;
      continue;
    }

    //
    // Next, if we're still running, make sure that our test prerequisites are in place.
    if (Test->Result == UNIT_TEST_PENDING && Test->PreReq != NULL)
    {
      DEBUG(( DEBUG_VERBOSE, "PREREQ\n" ));
      if (Test->PreReq( Suite->ParentFramework, Test->Context ) != UNIT_TEST_PASSED)
      {
        DEBUG(( DEBUG_ERROR, "PreReq Not Met\n" ));
        Test->Result = UNIT_TEST_ERROR_PREREQ_NOT_MET;
        ParentFramework->CurrentTest  = NULL;
        continue;
      }
    }

    //
    // Now we should be ready to call the actual test.
    // We set the status to UNIT_TEST_RUNNING in case the test needs to reboot
    // or quit. The UNIT_TEST_RUNNING state will allow the test to resume
    // but will prevent the PreReq from being dispatched a second time.
    Test->Result = UNIT_TEST_RUNNING;
    Test->Result = Test->RunTest( Suite->ParentFramework, Test->Context );

    //
    // Finally, clean everything up, if need be.
    if (Test->CleanUp != NULL)
    {
      DEBUG(( DEBUG_VERBOSE, "CLEANUP\n" ));
      Test->CleanUp( Suite->ParentFramework, Test->Context );
    }

    //
    // End the test.
    ParentFramework->CurrentTest  = NULL;
  } // End Test iteration


  if (Suite->Teardown != NULL)
  {
    Suite->Teardown( Suite->ParentFramework );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RunAllTestSuites(
  IN UNIT_TEST_FRAMEWORK  *Framework
  )
{
  UNIT_TEST_SUITE_LIST_ENTRY *Suite = NULL;
  EFI_STATUS Status; 

  if (Framework == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG((DEBUG_VERBOSE, "---------------------------------------------------------\n"));
  DEBUG((DEBUG_VERBOSE, "------------     RUNNING ALL TEST SUITES   --------------\n"));
  DEBUG((DEBUG_VERBOSE, "---------------------------------------------------------\n"));

  //
  // Iterate all suites
  //
  for (Suite = (UNIT_TEST_SUITE_LIST_ENTRY*)GetFirstNode(&Framework->TestSuiteList);
    (LIST_ENTRY*)Suite != &Framework->TestSuiteList;
    Suite = (UNIT_TEST_SUITE_LIST_ENTRY*)GetNextNode(&Framework->TestSuiteList, (LIST_ENTRY*)Suite))
  {
    Status = RunTestSuite(&(Suite->UTS));
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "Test Suite Failed with Error.  %r\n", Status));
    }
  } // End Suite iteration

  //Save current state so if test is started again it doesn't have to run.  It will just report
  SaveFrameworkState(Framework, NULL, 0);
  OutputUnitTestFrameworkReport(Framework);

  return EFI_SUCCESS;
}

//=============================================================================
//
// ----------------  TEST UTILITY FUNCTIONS -----------------------------------
//
//=============================================================================


STATIC
VOID
UpdateTestFromSave (
  IN OUT UNIT_TEST              *Test,
  IN     UNIT_TEST_SAVE_HEADER  *SavedState
  )
{
  UNIT_TEST_SAVE_TEST     *CurrentTest, *MatchingTest;
  UINT8                   *FloatingPointer;
  UNIT_TEST_SAVE_CONTEXT  *SavedContext;
  UINTN                   Index;

  //
  // First, evaluate the inputs.
  if (Test == NULL || SavedState == NULL)
  {
    return;
  }
  if (SavedState->TestCount == 0)
  {
    return;
  }

  //
  // Next, determine whether a matching test can be found.
  // Start at the beginning.
  MatchingTest    = NULL;
  FloatingPointer = (UINT8*)SavedState + sizeof( *SavedState );
  for (Index = 0; Index < SavedState->TestCount; Index++)
  {
    CurrentTest = (UNIT_TEST_SAVE_TEST*)FloatingPointer;
    if (CompareFingerprints( &Test->Fingerprint[0], &CurrentTest->Fingerprint[0] ))
    {
      MatchingTest = CurrentTest;
      // If there's a saved context, it's important that we iterate through the entire list.
      if (!SavedState->HasSavedContext)
      {
        break;
      }
    }

    // If we didn't find it, we have to increment to the next test.
    FloatingPointer = (UINT8*)CurrentTest + CurrentTest->Size;
  }

  //
  // If a matching test was found, copy the status.
  if (MatchingTest)
  {
    // Override the test status with the saved status.
    Test->Result = MatchingTest->Result;

    Test->FailureType = MatchingTest->FailureType;
    StrnCpy(&Test->FailureMessage[0], &MatchingTest->FailureMessage[0], UNIT_TEST_TESTFAILUREMSG_LENGTH);

    // If there is a log string associated, grab that.
    // We can tell that there's a log string because the "size" will be larger than
    // the structure size.
    // IMPORTANT NOTE: There are security implications here.
    //                 This data is user-supplied and we're about to play kinda
    //                 fast and loose with data buffers.
    if (MatchingTest->Size > sizeof( UNIT_TEST_SAVE_TEST ))
    {
      UnitTestLogInit(Test, ((UINT8*)MatchingTest + sizeof( UNIT_TEST_SAVE_TEST )), MatchingTest->Size - sizeof( UNIT_TEST_SAVE_TEST ) );
    }
  }

  //
  // If the saved context exists and matches this test, grab it, too.
  if (SavedState->HasSavedContext)
  {
    // TODO: Reconcile the difference between the way "size" works for Test Saves
    //        and the way it works for Context Saves. Too confusing to use it different ways.

    // If there was a saved context, the "matching test" loop will have placed the FloatingPointer
    // at the beginning of the context structure.
    SavedContext = (UNIT_TEST_SAVE_CONTEXT*)FloatingPointer;
    if (SavedContext->Size > 0 &&
        CompareFingerprints( &Test->Fingerprint[0], &SavedContext->Fingerprint[0] ))
    {
      // Override the test context with the saved context.
      Test->Context = (VOID*)((UINT8*)SavedContext + sizeof( *SavedContext ));
    }
  }

  return;
} // UpdateTestFromSave()


STATIC
UNIT_TEST_SAVE_HEADER*
SerializeState (
  IN UNIT_TEST_FRAMEWORK_HANDLE FrameworkHandle,
  IN UNIT_TEST_CONTEXT          ContextToSave     OPTIONAL,
  IN UINTN                      ContextToSaveSize
  )
{
  UNIT_TEST_FRAMEWORK         *Framework  = FrameworkHandle;
  UNIT_TEST_SAVE_HEADER       *Header = NULL;
  LIST_ENTRY                  *SuiteListHead, *Suite, *TestListHead, *Test;
  UINT32                      TestCount, TotalSize;
  UINTN                       LogSize;
  UNIT_TEST_SAVE_TEST         *TestSaveData;
  UNIT_TEST_SAVE_CONTEXT      *TestSaveContext;
  UNIT_TEST                   *UnitTest;
  UINT8                       *FloatingPointer;

  //
  // First, let's not make assumptions about the parameters.
  if (Framework == NULL || (ContextToSave != NULL && ContextToSaveSize == 0) ||
      ContextToSaveSize > MAX_UINT32)
  {
    return NULL;
  }

  //
  // Next, we've gotta figure out the resources that will be required to serialize the
  // the framework state so that we can persist it.
  // To start with, we're gonna need a header.
  TotalSize = sizeof( UNIT_TEST_SAVE_HEADER );
  // Now we need to figure out how many tests there are.
  TestCount = 0;
  // Iterate all suites.
  SuiteListHead = &Framework->TestSuiteList;
  for (Suite = GetFirstNode( SuiteListHead ); Suite != SuiteListHead; Suite = GetNextNode( SuiteListHead, Suite ))
  {
    // Iterate all tests within the suite.
    TestListHead = &((UNIT_TEST_SUITE_LIST_ENTRY*)Suite)->UTS.TestCaseList;
    for (Test = GetFirstNode( TestListHead ); Test != TestListHead; Test = GetNextNode( TestListHead, Test ))
    {
      UnitTest = &((UNIT_TEST_LIST_ENTRY*)Test)->UT;
      // Account for the size of a test structure.
      TotalSize += sizeof( UNIT_TEST_SAVE_TEST );
      // If there's a log, make sure to account for the log size.
      if (UnitTest->Log != NULL)
      {
        // The +1 is for the NULL character. Can't forget the NULL character.
        LogSize = (StrLen( UnitTest->Log ) + 1) * sizeof( CHAR16 );
        ASSERT( LogSize < MAX_UINT32 );
        TotalSize += (UINT32)LogSize;
      }
      // Increment the test count.
      TestCount++;
    }
  }
  // If there are no tests, we're done here.
  if (TestCount == 0)
  {
    return NULL;
  }
  // Add room for the context, if there is one.
  if (ContextToSave != NULL)
  {
    TotalSize += sizeof( UNIT_TEST_SAVE_CONTEXT ) + (UINT32)ContextToSaveSize;
  }

  //
  // Now that we know the size, we need to allocate space for the serialized output.
  Header = AllocateZeroPool( TotalSize );
  if (Header == NULL)
  {
    return NULL;
  }

  //
  // Alright, let's start setting up some data.
  Header->Version         = UNIT_TEST_PERSISTENCE_LIB_VERSION;
  Header->BlobSize        = TotalSize;
  CopyMem( &Header->Fingerprint[0], &Framework->Fingerprint[0], UNIT_TEST_FINGERPRINT_SIZE );
  CopyMem( &Header->StartTime, &Framework->StartTime, sizeof( EFI_TIME ) );
  Header->TestCount       = TestCount;
  Header->HasSavedContext = FALSE;

  //
  // Start adding all of the test cases.
  // Set the floating pointer to the start of the current test save buffer.
  FloatingPointer = (UINT8*)Header + sizeof( UNIT_TEST_SAVE_HEADER );
  // Iterate all suites.
  SuiteListHead = &Framework->TestSuiteList;
  for (Suite = GetFirstNode( SuiteListHead ); Suite != SuiteListHead; Suite = GetNextNode( SuiteListHead, Suite ))
  {
    // Iterate all tests within the suite.
    TestListHead = &((UNIT_TEST_SUITE_LIST_ENTRY*)Suite)->UTS.TestCaseList;
    for (Test = GetFirstNode( TestListHead ); Test != TestListHead; Test = GetNextNode( TestListHead, Test ))
    {
      TestSaveData  = (UNIT_TEST_SAVE_TEST*)FloatingPointer;
      UnitTest      = &((UNIT_TEST_LIST_ENTRY*)Test)->UT;
      
      // Save the fingerprint.
      CopyMem( &TestSaveData->Fingerprint[0], &UnitTest->Fingerprint[0], UNIT_TEST_FINGERPRINT_SIZE );
      
      // Save the result.
      TestSaveData->Result = UnitTest->Result;
      TestSaveData->FailureType = UnitTest->FailureType;
      StrnCpy(&TestSaveData->FailureMessage[0], &UnitTest->FailureMessage[0], UNIT_TEST_TESTFAILUREMSG_LENGTH);

      
      // If there is a log, save the log.
      FloatingPointer += sizeof( UNIT_TEST_SAVE_TEST );
      if (UnitTest->Log != NULL)
      {
        // The +1 is for the NULL character. Can't forget the NULL character.
        LogSize = (StrLen( UnitTest->Log ) + 1) * sizeof( CHAR16 );
        CopyMem( FloatingPointer, UnitTest->Log, LogSize );
        FloatingPointer += LogSize;
      }

      // Update the size once the structure is complete.
      // NOTE: Should thise be a straight cast without validation?
      //       Maybe.
      //       Am I tired of writing code?
      //       Yes.
      TestSaveData->Size = (UINT32)(FloatingPointer - (UINT8*)TestSaveData);
    }
  }

  //
  // If there is a context to save, let's do that now.
  if (ContextToSave != NULL && Framework->CurrentTest != NULL)
  {
    TestSaveContext         = (UNIT_TEST_SAVE_CONTEXT*)FloatingPointer;
    TestSaveContext->Size   = (UINT32)ContextToSaveSize;
    CopyMem( &TestSaveContext->Fingerprint[0], &Framework->CurrentTest->Fingerprint[0], UNIT_TEST_FINGERPRINT_SIZE );
    CopyMem( ((UINT8*)TestSaveContext + sizeof( UNIT_TEST_SAVE_CONTEXT )), ContextToSave, ContextToSaveSize );
    Header->HasSavedContext = TRUE;
  }

  return Header;
}


EFI_STATUS
EFIAPI
SaveFrameworkState (
  IN UNIT_TEST_FRAMEWORK_HANDLE FrameworkHandle,
  IN UNIT_TEST_CONTEXT          ContextToSave     OPTIONAL,
  IN UINTN                      ContextToSaveSize
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_SAVE_HEADER       *Header = NULL;

  //
  // First, let's not make assumptions about the parameters.
  if (FrameworkHandle == NULL || (ContextToSave != NULL && ContextToSaveSize == 0) ||
      ContextToSaveSize > MAX_UINT32)
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Now, let's package up all the data for saving.
  Header = SerializeState( FrameworkHandle, ContextToSave, ContextToSaveSize );
  if (Header == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // All that should be left to do is save it using the associated persistence lib.
  Status = SaveUnitTestCache( FrameworkHandle, Header );
  if (EFI_ERROR( Status ))
  {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Could not save state! %r\n", Status ));
    Status = EFI_DEVICE_ERROR;
  }

  //
  // Free data that was used.
  FreePool( Header );

  return Status;
} // SaveFrameworkState()


EFI_STATUS
EFIAPI
SaveFrameworkStateAndQuit (
  IN UNIT_TEST_FRAMEWORK_HANDLE FrameworkHandle,
  IN UNIT_TEST_CONTEXT          ContextToSave     OPTIONAL,
  IN UINTN                      ContextToSaveSize
  )
{
  EFI_STATUS                  Status;

  //
  // First, let's not make assumptions about the parameters.
  if (FrameworkHandle == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Now, save all the data associated with this framework.
  Status = SaveFrameworkState( FrameworkHandle, ContextToSave, ContextToSaveSize );

  //
  // If we're all good, let's book...
  if (!EFI_ERROR( Status ))
  {
    //
    // Free data that was used.
    FreeUnitTestFramework( (UNIT_TEST_FRAMEWORK*)FrameworkHandle );

    //
    // Quit
    gBS->Exit( gImageHandle, EFI_SUCCESS, 0, NULL );
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Unit test failed to quit! Framework can no longer be used!\n" ));

    //
    // We REALLY shouldn't be here.
    Status = EFI_ABORTED;
  }

  return Status;
} // SaveFrameworkStateAndQuit()


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
  )
{
  EFI_STATUS                  Status;

  //
  // First, let's not make assumptions about the parameters.
  if (FrameworkHandle == NULL ||
      (ResetType != EfiResetCold && ResetType != EfiResetWarm))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Now, save all the data associated with this framework.
  Status = SaveFrameworkState( FrameworkHandle, ContextToSave, ContextToSaveSize );

  //
  // If we're all good, let's book...
  if (!EFI_ERROR( Status ))
  {
    //
    // Next, we want to update the BootNext variable to USB
    // so that we have a fighting chance of coming back here.
    //
    SetUsbBootNext();

    //
    // Free data that was used.
    FreeUnitTestFramework( (UNIT_TEST_FRAMEWORK*)FrameworkHandle );

    //
    // Reset 
    gRT->ResetSystem( ResetType, EFI_SUCCESS, 0, NULL );
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Unit test failed to quit! Framework can no longer be used!\n" ));

    //
    // We REALLY shouldn't be here.
    Status = EFI_ABORTED;
  }

  return Status;
} // SaveFrameworkStateAndReboot()
