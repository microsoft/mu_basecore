/** @file
Provides the basic types and common elements of the unit test framework 


Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>

**/

#ifndef __UNIT_TEST_TYPES_H__
#define __UNIT_TEST_TYPES_H__

///================================================================================================
///================================================================================================
///
/// HANDY DEFINITIONS
///
///================================================================================================
///================================================================================================
#define UNIT_TEST_MAX_STRING_LENGTH     (120)

#define UNIT_TEST_FINGERPRINT_SIZE      (16)    // Hardcoded to MD5_HASHSIZE.
#define UNIT_TEST_TESTFAILUREMSG_LENGTH (120)

typedef UINT32 UNIT_TEST_STATUS;
#define UNIT_TEST_PASSED                      (0)
#define UNIT_TEST_ERROR_PREREQ_NOT_MET        (1)
#define UNIT_TEST_ERROR_TEST_FAILED           (2)
#define UNIT_TEST_SKIPPED                     (0xFFFFFFFD)
#define UNIT_TEST_RUNNING                     (0xFFFFFFFE)
#define UNIT_TEST_PENDING                     (0xFFFFFFFF)

typedef UINT32 FAILURE_TYPE;
#define FAILURETYPE_NOFAILURE     (0)
#define FAILURETYPE_OTHER         (1)
#define FAILURETYPE_ASSERTTRUE    (2)
#define FAILURETYPE_ASSERTFALSE   (3)
#define FAILURETYPE_ASSERTEQUAL   (4)
#define FAILURETYPE_ASSERTNOTEQUAL (5)
#define FAILURETYPE_ASSERTNOTEFIERROR (6)
#define FAILURETYPE_ASSERTSTATUSEQUAL (7)
#define FAILURETYPE_ASSERTNOTNULL (8)

typedef VOID*   UNIT_TEST_FRAMEWORK_HANDLE; // Same as a UNIT_TEST_FRAMEWORK*, but with fewer build errors.
typedef VOID*   UNIT_TEST_SUITE_HANDLE;     // Same as a UNIT_TEST_SUITE*, but with fewer build errors.
typedef VOID*   UNIT_TEST_CONTEXT;


///================================================================================================
///================================================================================================
///
/// UNIT TEST FUNCTION TYPE DEFINITIONS
///
///================================================================================================
///================================================================================================


//
// Unit-Test Function pointer type.
//
typedef
UNIT_TEST_STATUS
(EFIAPI *UNIT_TEST_FUNCTION) (
  UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  UNIT_TEST_CONTEXT           Context
  );

//
// Unit-Test Prerequisite Function pointer type.
// NOTE: Should be the same as UnitTest.
//
typedef
UNIT_TEST_STATUS
(EFIAPI *UNIT_TEST_PREREQ) (
  UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  UNIT_TEST_CONTEXT           Context
  );

//
// Unit-Test Test Cleanup (after) function pointer type.
//
typedef
VOID
(EFIAPI *UNIT_TEST_CLEANUP) (
  UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  UNIT_TEST_CONTEXT           Context
  );

//
// Unit-Test Test Suite Setup (before) function pointer type.
//
typedef
VOID
(EFIAPI *UNIT_TEST_SUITE_SETUP) (
  UNIT_TEST_FRAMEWORK_HANDLE  Framework
  );

//
// Unit-Test Test Suite Teardown (after) function pointer type.
//
typedef
VOID
(EFIAPI *UNIT_TEST_SUITE_TEARDOWN) (
  UNIT_TEST_FRAMEWORK_HANDLE  Framework
  );


///================================================================================================
///================================================================================================
///
/// UNIT TEST DATA STRUCTURE DEFINITIONS
///
///================================================================================================
///================================================================================================


typedef struct {
  CHAR16                    *Description;
  CHAR16                    *ClassName;  //can't have spaces and should be short
  CHAR16                    *Log;
  FAILURE_TYPE              FailureType;
  CHAR16                    FailureMessage[UNIT_TEST_TESTFAILUREMSG_LENGTH];
  UINT8                     Fingerprint[UNIT_TEST_FINGERPRINT_SIZE];
  UNIT_TEST_STATUS          Result;
  UNIT_TEST_FUNCTION        RunTest;
  UNIT_TEST_PREREQ          PreReq;
  UNIT_TEST_CLEANUP         CleanUp;
  UNIT_TEST_CONTEXT         Context;
  UNIT_TEST_SUITE_HANDLE    ParentSuite;
} UNIT_TEST;

typedef struct {
  LIST_ENTRY    Entry;
  UNIT_TEST     UT;
} UNIT_TEST_LIST_ENTRY;

typedef struct {
  CHAR16                      *Title;
  CHAR16                      *Package;
  UINT8                       Fingerprint[UNIT_TEST_FINGERPRINT_SIZE];
  UNIT_TEST_SUITE_SETUP       Setup;
  UNIT_TEST_SUITE_TEARDOWN    Teardown;
  LIST_ENTRY                  TestCaseList;     // UNIT_TEST_LIST_ENTRY
  UNIT_TEST_FRAMEWORK_HANDLE  ParentFramework;
} UNIT_TEST_SUITE;

typedef struct {
  LIST_ENTRY        Entry;
  UNIT_TEST_SUITE   UTS;
} UNIT_TEST_SUITE_LIST_ENTRY;

typedef struct {
  CHAR16                    *Title;
  CHAR16                    *ShortTitle;      // This title should contain NO spaces or non-filename charatecters. Is used in reporting and serialization.
  CHAR16                    *VersionString;
  CHAR16                    *Log;
  UINT8                     Fingerprint[UNIT_TEST_FINGERPRINT_SIZE];
  LIST_ENTRY                TestSuiteList;    // UNIT_TEST_SUITE_LIST_ENTRY
  EFI_TIME                  StartTime;
  EFI_TIME                  EndTime;
  UNIT_TEST                 *CurrentTest;
  VOID                      *SavedState;      // This is an instance of UNIT_TEST_SAVE_HEADER*, if present.
} UNIT_TEST_FRAMEWORK;


//
//Structures for the framework to serializing unit test status
//
#pragma pack (1)

typedef struct
{
  UINT32            Size;
  UINT8             Fingerprint[UNIT_TEST_FINGERPRINT_SIZE];      // Fingerprint of the test itself.
  CHAR16            FailureMessage[UNIT_TEST_TESTFAILUREMSG_LENGTH];
  FAILURE_TYPE      FailureType;
  UNIT_TEST_STATUS  Result;
  // CHAR16            Log[];
} UNIT_TEST_SAVE_TEST;

typedef struct
{
  UINT32            Size;
  UINT8             Fingerprint[UNIT_TEST_FINGERPRINT_SIZE];      // Fingerprint of the corresponding test.
  // UINT8          Data[];                                       // Actual data of the context.
} UNIT_TEST_SAVE_CONTEXT;

typedef struct
{
  UINT8             Version;
  UINT32            BlobSize;
  UINT8             Fingerprint[UNIT_TEST_FINGERPRINT_SIZE];      // Fingerprint of the framework that has been saved.
  EFI_TIME          StartTime;
  UINT32            TestCount;
  BOOLEAN           HasSavedContext;
  // UNIT_TEST_SAVE_TEST    Tests[];                              // Array of structures starts here.
  // UNIT_TEST_SAVE_CONTEXT SavedContext[];                       // Saved context for the currently running test.
  // CHAR16                 Log[];                                // NOTE: Not yet implemented!!
} UNIT_TEST_SAVE_HEADER;

#pragma pack ()

#endif