/** @file
  Unit tests for the Secure Boot authority measurement helpers in
  DxeImageVerificationLib (Measurement.c): GetMeasuredAuthorities,
  AssignVariableName, AssignVendorGuid, IsSecureAuthorityVariable,
  IsDataMeasured, AddDataMeasured, MeasureVariable, and SecureBootHook.

  The de-duplication helpers are exercised against a caller-supplied
  MEASURED_AUTHORITIES instance so no module-global state is relied upon,
  and the PCR 7 extend path is exercised against a mocked
  TpmMeasureAndLogData.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockTpmMeasurementLib.h>

#include <vector>

extern "C" {
  #include <Uefi.h>
  #include <Guid/ImageAuthentication.h>
  #include <IndustryStandard/UefiTcgPlatform.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include "../DxeImageVerificationLib.h"

  CHAR16 *
  AssignVariableName (
    IN CHAR16  *VariableName
    );

  EFI_GUID *
  AssignVendorGuid (
    IN EFI_GUID  *VendorGuid
    );

  BOOLEAN
  IsSecureAuthorityVariable (
    IN CHAR16    *VariableName,
    IN EFI_GUID  *VendorGuid
    );

  BOOLEAN
  IsDataMeasured (
    IN CONST MEASURED_AUTHORITIES  *Measured,
    IN CHAR16                      *VariableName,
    IN EFI_GUID                    *VendorGuid,
    IN VOID                        *Data,
    IN UINTN                       Size
    );

  EFI_STATUS
  AddDataMeasured (
    IN OUT MEASURED_AUTHORITIES  *Measured,
    IN     CHAR16                *VariableName,
    IN     EFI_GUID              *VendorGuid,
    IN     VOID                  *Data,
    IN     UINTN                 Size
    );

  EFI_STATUS
  MeasureVariable (
    IN CHAR16    *VarName,
    IN EFI_GUID  *VendorGuid,
    IN VOID      *VarData,
    IN UINTN     VarSize
    );

  VOID
  SecureBootHook (
    IN OUT MEASURED_AUTHORITIES   *Measured,
    IN     CHAR16                 *VariableName,
    IN     EFI_GUID               *VendorGuid,
    IN     CONST IMAGE_AUTHORITY  *Authority
    );
}

using ::testing::_;
using ::testing::Return;

//
// The growth increment from Measurement.c. Kept in sync here so the list
// growth path can be exercised without exposing the module constant.
//
#define TEST_AUTHORITY_COUNT_INCREMENT  0x100

//
// A known Secure Boot authority variable name. The cast drops the char16_t
// literal's const qualifier so it matches the non-const parameter type of the
// helpers under test.
//
static CHAR16  *mDbName = (CHAR16 *)u"db";

//
// A vendor GUID that is not a Secure Boot authority vendor GUID.
//
static EFI_GUID  mNonAuthorityGuid = {
  0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 }
};

//
// A variable name that is not a Secure Boot authority variable name.
//
static CHAR16  *mNonAuthorityName = (CHAR16 *)u"NotADb";

//
// Free every Data copy and the backing list of a MEASURED_AUTHORITIES that was
// populated through AddDataMeasured, and reset it to the empty state.
//
static void
FreeMeasured (
  MEASURED_AUTHORITIES  &Measured
  )
{
  if (Measured.List != NULL) {
    for (UINTN Index = 0; Index < Measured.Count; Index++) {
      if (Measured.List[Index].Data != NULL) {
        FreePool (Measured.List[Index].Data);
      }
    }

    FreePool (Measured.List);
  }

  Measured.List  = NULL;
  Measured.Count = 0;
  Measured.Max   = 0;
}

// ---------------------------------------------------------------------------
// GetMeasuredAuthorities
// ---------------------------------------------------------------------------

class GetMeasuredAuthoritiesTest : public ::testing::Test {
protected:
  MockTpmMeasurementLib TpmMock;
};

TEST_F (GetMeasuredAuthoritiesTest, ReflectsSecureBootHookUpdates) {
  UINT8            Data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  IMAGE_AUTHORITY  Authority;

  Authority.Data = (EFI_SIGNATURE_DATA *)Data;
  Authority.Size = sizeof (Data);

  //
  // The module-global state starts empty: no list is allocated and nothing
  // has been measured yet.
  //
  MEASURED_AUTHORITIES  *Measured = GetMeasuredAuthorities ();

  ASSERT_NE (Measured, nullptr);
  EXPECT_EQ (Measured->List, nullptr);
  EXPECT_EQ (Measured->Count, (UINTN)0);

  EXPECT_CALL (
    TpmMock,
    TpmMeasureAndLogData ((UINT32)7, (UINT32)EV_EFI_VARIABLE_AUTHORITY, _, _, _, _)
    )
    .WillOnce (Return (EFI_SUCCESS));

  SecureBootHook (Measured, mDbName, &gEfiImageSecurityDatabaseGuid, &Authority);

  //
  // The same global instance is returned and now reflects the measured
  // authority recorded by SecureBootHook.
  //
  MEASURED_AUTHORITIES  *Updated = GetMeasuredAuthorities ();

  EXPECT_EQ (Updated, Measured);
  EXPECT_NE (Updated->List, nullptr);
  EXPECT_EQ (Updated->Count, (UINTN)1);
  EXPECT_TRUE (
    IsDataMeasured (Updated, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data))
    );

  FreeMeasured (*Updated);
}

// ---------------------------------------------------------------------------
// AssignVariableName
// ---------------------------------------------------------------------------

class AssignVariableNameTest : public ::testing::Test {
};

TEST_F (AssignVariableNameTest, NullName_ReturnsNull) {
  EXPECT_EQ (AssignVariableName (NULL), nullptr);
}

TEST_F (AssignVariableNameTest, UnknownName_ReturnsNull) {
  EXPECT_EQ (AssignVariableName (mNonAuthorityName), nullptr);
}

TEST_F (AssignVariableNameTest, KnownName_ReturnsCanonicalStorage) {
  CHAR16  Local[] = { u'd', u'b', u'\0' };

  CHAR16  *First  = AssignVariableName (Local);
  CHAR16  *Second = AssignVariableName (mDbName);

  //
  // A known name resolves to canonical storage owned by the module, so the
  // returned pointer must be independent of the caller's buffer and stable.
  //
  EXPECT_NE (First, nullptr);
  EXPECT_NE (First, Local);
  EXPECT_EQ (First, Second);
  EXPECT_EQ (StrCmp (First, mDbName), 0);
}

// ---------------------------------------------------------------------------
// AssignVendorGuid
// ---------------------------------------------------------------------------

class AssignVendorGuidTest : public ::testing::Test {
};

TEST_F (AssignVendorGuidTest, NullGuid_ReturnsNull) {
  EXPECT_EQ (AssignVendorGuid (NULL), nullptr);
}

TEST_F (AssignVendorGuidTest, UnknownGuid_ReturnsNull) {
  EXPECT_EQ (AssignVendorGuid (&mNonAuthorityGuid), nullptr);
}

TEST_F (AssignVendorGuidTest, KnownGuid_ReturnsCanonicalStorage) {
  EFI_GUID  Local;

  CopyGuid (&Local, &gEfiImageSecurityDatabaseGuid);

  EFI_GUID  *First  = AssignVendorGuid (&Local);
  EFI_GUID  *Second = AssignVendorGuid (&gEfiImageSecurityDatabaseGuid);

  EXPECT_NE (First, nullptr);
  EXPECT_NE (First, &Local);
  EXPECT_EQ (First, Second);
  EXPECT_TRUE (CompareGuid (First, &gEfiImageSecurityDatabaseGuid));
}

// ---------------------------------------------------------------------------
// IsSecureAuthorityVariable
// ---------------------------------------------------------------------------

class IsSecureAuthorityVariableTest : public ::testing::Test {
};

TEST_F (IsSecureAuthorityVariableTest, NullArgs_ReturnFalse) {
  EXPECT_FALSE (IsSecureAuthorityVariable (NULL, &gEfiImageSecurityDatabaseGuid));
  EXPECT_FALSE (IsSecureAuthorityVariable (mDbName, NULL));
}

TEST_F (IsSecureAuthorityVariableTest, KnownNameAndGuid_ReturnsTrue) {
  EXPECT_TRUE (IsSecureAuthorityVariable (mDbName, &gEfiImageSecurityDatabaseGuid));
}

TEST_F (IsSecureAuthorityVariableTest, KnownNameWrongGuid_ReturnsFalse) {
  EXPECT_FALSE (IsSecureAuthorityVariable (mDbName, &mNonAuthorityGuid));
}

TEST_F (IsSecureAuthorityVariableTest, WrongNameKnownGuid_ReturnsFalse) {
  EXPECT_FALSE (IsSecureAuthorityVariable (mNonAuthorityName, &gEfiImageSecurityDatabaseGuid));
}

// ---------------------------------------------------------------------------
// IsDataMeasured / AddDataMeasured
// ---------------------------------------------------------------------------

class MeasuredListTest : public ::testing::Test {
protected:
  MEASURED_AUTHORITIES Measured = { NULL, 0, 0 };

  void
  TearDown (
    ) override
  {
    FreeMeasured (Measured);
  }
};

TEST_F (MeasuredListTest, IsDataMeasured_NullArgs_ReturnFalse) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  EXPECT_FALSE (IsDataMeasured (NULL, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)));
  EXPECT_FALSE (IsDataMeasured (&Measured, NULL, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)));
  EXPECT_FALSE (IsDataMeasured (&Measured, mDbName, NULL, Data, sizeof (Data)));
  EXPECT_FALSE (IsDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, NULL, sizeof (Data)));
}

TEST_F (MeasuredListTest, IsDataMeasured_EmptyList_ReturnsFalse) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  EXPECT_FALSE (
    IsDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data))
    );
}

TEST_F (MeasuredListTest, AddDataMeasured_NullOrZeroArgs_ReturnInvalidParameter) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  EXPECT_EQ (
    AddDataMeasured (NULL, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    AddDataMeasured (&Measured, NULL, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    AddDataMeasured (&Measured, mDbName, NULL, Data, sizeof (Data)),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    AddDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, NULL, sizeof (Data)),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    AddDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, 0),
    EFI_INVALID_PARAMETER
    );

  EXPECT_EQ (Measured.Count, (UINTN)0);
}

TEST_F (MeasuredListTest, AddDataMeasured_RecordsEntry) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  EXPECT_EQ (
    AddDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_SUCCESS
    );

  ASSERT_EQ (Measured.Count, (UINTN)1);
  EXPECT_EQ (Measured.List[0].Size, (UINTN)sizeof (Data));

  //
  // A private copy of the data is stored, and the name / GUID are canonicalized
  // to module-owned storage.
  //
  EXPECT_NE (Measured.List[0].Data, (VOID *)Data);
  EXPECT_EQ (CompareMem (Measured.List[0].Data, Data, sizeof (Data)), 0);
  EXPECT_EQ (Measured.List[0].VariableName, AssignVariableName (mDbName));
  EXPECT_EQ (Measured.List[0].VendorGuid, AssignVendorGuid (&gEfiImageSecurityDatabaseGuid));
}

TEST_F (MeasuredListTest, IsDataMeasured_MatchingEntry_ReturnsTrue) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  ASSERT_EQ (
    AddDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_SUCCESS
    );

  EXPECT_TRUE (
    IsDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data))
    );
}

TEST_F (MeasuredListTest, IsDataMeasured_DifferentSize_ReturnsFalse) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  ASSERT_EQ (
    AddDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_SUCCESS
    );

  EXPECT_FALSE (
    IsDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data) - 1)
    );
}

TEST_F (MeasuredListTest, IsDataMeasured_DifferentData_ReturnsFalse) {
  UINT8  Data[4]  = { 1, 2, 3, 4 };
  UINT8  Other[4] = { 1, 2, 3, 5 };

  ASSERT_EQ (
    AddDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_SUCCESS
    );

  EXPECT_FALSE (
    IsDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Other, sizeof (Other))
    );
}

TEST_F (MeasuredListTest, IsDataMeasured_DifferentGuid_ReturnsFalse) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  ASSERT_EQ (
    AddDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_SUCCESS
    );

  EXPECT_FALSE (
    IsDataMeasured (&Measured, mDbName, &mNonAuthorityGuid, Data, sizeof (Data))
    );
}

TEST_F (MeasuredListTest, AddDataMeasured_GrowsListAcrossIncrement) {
  //
  // Add one more than the growth increment so the backing list must be
  // reallocated at least once, and confirm every entry remains tracked.
  //
  const UINTN  Total = TEST_AUTHORITY_COUNT_INCREMENT + 1;

  for (UINTN Index = 0; Index < Total; Index++) {
    UINT32  Data = (UINT32)Index;

    ASSERT_EQ (
      AddDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, &Data, sizeof (Data)),
      EFI_SUCCESS
      );
  }

  EXPECT_EQ (Measured.Count, Total);
  EXPECT_GE (Measured.Max, Total);

  for (UINTN Index = 0; Index < Total; Index++) {
    UINT32  Data = (UINT32)Index;

    EXPECT_TRUE (
      IsDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, &Data, sizeof (Data))
      );
  }
}

// ---------------------------------------------------------------------------
// MeasureVariable
// ---------------------------------------------------------------------------

class MeasureVariableTest : public ::testing::Test {
protected:
  MockTpmMeasurementLib TpmMock;
};

TEST_F (MeasureVariableTest, NullArgs_ReturnInvalidParameter) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  EXPECT_EQ (
    MeasureVariable (NULL, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    MeasureVariable (mDbName, NULL, Data, sizeof (Data)),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    MeasureVariable (mDbName, &gEfiImageSecurityDatabaseGuid, NULL, sizeof (Data)),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (MeasureVariableTest, Success_ExtendsPcr7AsVariableAuthority) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  EXPECT_CALL (
    TpmMock,
    TpmMeasureAndLogData (
      (UINT32)7,
      (UINT32)EV_EFI_VARIABLE_AUTHORITY,
      _,
      _,
      _,
      _
      )
    )
    .WillOnce (Return (EFI_SUCCESS));

  EXPECT_EQ (
    MeasureVariable (mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_SUCCESS
    );
}

TEST_F (MeasureVariableTest, MeasureFails_PropagatesError) {
  UINT8  Data[4] = { 1, 2, 3, 4 };

  EXPECT_CALL (TpmMock, TpmMeasureAndLogData (_, _, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  EXPECT_EQ (
    MeasureVariable (mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data)),
    EFI_DEVICE_ERROR
    );
}

// ---------------------------------------------------------------------------
// SecureBootHook
// ---------------------------------------------------------------------------

class SecureBootHookTest : public ::testing::Test {
protected:
  MockTpmMeasurementLib TpmMock;
  MEASURED_AUTHORITIES Measured = { NULL, 0, 0 };

  void
  TearDown (
    ) override
  {
    FreeMeasured (Measured);
  }

  static IMAGE_AUTHORITY
  MakeAuthority (
    const EFI_SIGNATURE_DATA  *Data,
    UINTN                     Size
    )
  {
    IMAGE_AUTHORITY  Authority;

    Authority.Data = Data;
    Authority.Size = Size;
    return Authority;
  }
};

TEST_F (SecureBootHookTest, NullMeasured_NoOp) {
  UINT8            Data[8]   = { 0 };
  IMAGE_AUTHORITY  Authority = MakeAuthority ((EFI_SIGNATURE_DATA *)Data, sizeof (Data));

  EXPECT_CALL (TpmMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  SecureBootHook (NULL, mDbName, &gEfiImageSecurityDatabaseGuid, &Authority);
}

TEST_F (SecureBootHookTest, NullAuthority_NoOp) {
  EXPECT_CALL (TpmMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  SecureBootHook (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, NULL);
  EXPECT_EQ (Measured.Count, (UINTN)0);
}

TEST_F (SecureBootHookTest, AuthorityWithNullDataOrZeroSize_NoOp) {
  UINT8            Data[8]  = { 0 };
  IMAGE_AUTHORITY  NullData = MakeAuthority (NULL, sizeof (Data));
  IMAGE_AUTHORITY  ZeroSize = MakeAuthority ((EFI_SIGNATURE_DATA *)Data, 0);

  EXPECT_CALL (TpmMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  SecureBootHook (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, &NullData);
  SecureBootHook (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, &ZeroSize);
  EXPECT_EQ (Measured.Count, (UINTN)0);
}

TEST_F (SecureBootHookTest, NonAuthorityVariable_NoOp) {
  UINT8            Data[8]   = { 0 };
  IMAGE_AUTHORITY  Authority = MakeAuthority ((EFI_SIGNATURE_DATA *)Data, sizeof (Data));

  EXPECT_CALL (TpmMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  SecureBootHook (&Measured, mNonAuthorityName, &gEfiImageSecurityDatabaseGuid, &Authority);
  EXPECT_EQ (Measured.Count, (UINTN)0);
}

TEST_F (SecureBootHookTest, FirstMeasure_RecordsAndExtends) {
  UINT8            Data[8]   = { 1, 2, 3, 4, 5, 6, 7, 8 };
  IMAGE_AUTHORITY  Authority = MakeAuthority ((EFI_SIGNATURE_DATA *)Data, sizeof (Data));

  EXPECT_CALL (
    TpmMock,
    TpmMeasureAndLogData ((UINT32)7, (UINT32)EV_EFI_VARIABLE_AUTHORITY, _, _, _, _)
    )
    .WillOnce (Return (EFI_SUCCESS));

  SecureBootHook (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, &Authority);

  EXPECT_EQ (Measured.Count, (UINTN)1);
  EXPECT_TRUE (
    IsDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data))
    );
}

TEST_F (SecureBootHookTest, DuplicateMeasure_ExtendsOnce) {
  UINT8            Data[8]   = { 1, 2, 3, 4, 5, 6, 7, 8 };
  IMAGE_AUTHORITY  Authority = MakeAuthority ((EFI_SIGNATURE_DATA *)Data, sizeof (Data));

  //
  // An already-measured authority must not be extended a second time.
  //
  EXPECT_CALL (
    TpmMock,
    TpmMeasureAndLogData ((UINT32)7, (UINT32)EV_EFI_VARIABLE_AUTHORITY, _, _, _, _)
    )
    .Times (1)
    .WillOnce (Return (EFI_SUCCESS));

  SecureBootHook (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, &Authority);
  SecureBootHook (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, &Authority);

  EXPECT_EQ (Measured.Count, (UINTN)1);
}

TEST_F (SecureBootHookTest, MeasureFails_NotRecorded) {
  UINT8            Data[8]   = { 1, 2, 3, 4, 5, 6, 7, 8 };
  IMAGE_AUTHORITY  Authority = MakeAuthority ((EFI_SIGNATURE_DATA *)Data, sizeof (Data));

  EXPECT_CALL (TpmMock, TpmMeasureAndLogData (_, _, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  SecureBootHook (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, &Authority);

  EXPECT_EQ (Measured.Count, (UINTN)0);
  EXPECT_FALSE (
    IsDataMeasured (&Measured, mDbName, &gEfiImageSecurityDatabaseGuid, Data, sizeof (Data))
    );
}
