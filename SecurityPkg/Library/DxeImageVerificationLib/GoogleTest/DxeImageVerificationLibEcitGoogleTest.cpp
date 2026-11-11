/** @file
  Regression tests for image-verification ECIT reporting.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockBaseCryptLib.h>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

extern "C" {
  #include <Uefi.h>
  #include <Guid/CryptoIndicatorTable.h>
  #include <Guid/CryptoOpId.h>
  #include <Guid/ImageAuthentication.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include "../Ecit.h"
}

struct CapturedReport {
  EFI_GUID              Feature;
  std::vector<UINT8>    Data;
};

static EFI_STATUS                   mQueryStatus[2];
static UINTN                        mQueries;
static BOOLEAN                      mUnsupportedDigestsOnly;
static BOOLEAN                      mInvalidCount;
static BOOLEAN                      mOverflowCount;
static BOOLEAN                      mInvalidOid;
static BOOLEAN                      mDuplicateOid;
static CONST EFI_GUID               *mFailedFeature;
static std::vector<EFI_GUID>        mAttempts;
static std::vector<CapturedReport>  mReports;

static EFI_STATUS
EFIAPI
GetSyntheticCapabilities (
  IN CONST EFI_GUID             *OperationId,
  OUT BASE_CRYPT_OP_CAPABILITY  **Capabilities,
  OUT UINTN                     *CapabilityCount
  )
{
  static CONST CHAR8     *SignatureOids[] = { "1.2.840.113549.1.1.11" };
  static CONST CHAR8     *DuplicateOids[] = { "1.2.840.113549.1.1.11", "2.16.840.1.101.3.4.2.1" };
  static CONST CHAR8     *InvalidOids[]   = { "not-an-oid" };
  static CONST CHAR8     *DigestOids[]    = {
    "1.3.14.3.2.26",
    "2.16.840.1.101.3.4.2.4",
    "2.16.840.1.101.3.4.2.1",
    "2.16.840.1.101.3.4.2.2",
    "2.16.840.1.101.3.4.2.3"
  };
  static CONST EFI_GUID  *Operations[] = { &gCryptoOpAuthenticodeVerifyGuid, &gCryptoOpAuthenticodeHashGuid };
  CONST CHAR8            **Oids;
  UINTN                  Query;
  UINTN                  Count;
  UINTN                  Size;
  CHAR8                  *Cursor;

  *Capabilities    = nullptr;
  *CapabilityCount = 0;
  Query            = mQueries++;
  EXPECT_LT (Query, ARRAY_SIZE (Operations));
  if (Query >= ARRAY_SIZE (Operations)) {
    return EFI_INVALID_PARAMETER;
  }

  EXPECT_TRUE (CompareGuid (OperationId, Operations[Query]));
  if (EFI_ERROR (mQueryStatus[Query])) {
    return mQueryStatus[Query];
  }

  if (mInvalidCount && (Query == 0)) {
    *CapabilityCount = 1;
    return EFI_SUCCESS;
  }

  if (mOverflowCount) {
    *Capabilities = (BASE_CRYPT_OP_CAPABILITY *)AllocateZeroPool (sizeof (**Capabilities));
    if (*Capabilities == nullptr) {
      return EFI_OUT_OF_RESOURCES;
    }

    *CapabilityCount = (Query == 0) ? MAX_UINTN : 1;
    return EFI_SUCCESS;
  }

  if (mUnsupportedDigestsOnly && (Query == 0)) {
    return EFI_SUCCESS;
  }

  Oids  = (Query == 0) ? SignatureOids : DigestOids;
  Count = (Query == 0) ? ARRAY_SIZE (SignatureOids) : ARRAY_SIZE (DigestOids);
  if (mUnsupportedDigestsOnly) {
    Count = 2;
  } else if (mInvalidOid && (Query == 0)) {
    Oids  = InvalidOids;
    Count = ARRAY_SIZE (InvalidOids);
  } else if (mDuplicateOid && (Query == 0)) {
    Oids  = DuplicateOids;
    Count = ARRAY_SIZE (DuplicateOids);
  }

  Size = Count * sizeof (**Capabilities);
  for (UINTN Index = 0; Index < Count; Index++) {
    Size += AsciiStrSize (Oids[Index]);
  }

  *Capabilities = (BASE_CRYPT_OP_CAPABILITY *)AllocateZeroPool (Size);
  if (*Capabilities == nullptr) {
    return EFI_OUT_OF_RESOURCES;
  }

  Cursor = (CHAR8 *)(*Capabilities + Count);
  for (UINTN Index = 0; Index < Count; Index++) {
    Size                                    = AsciiStrSize (Oids[Index]);
    (*Capabilities)[Index].AlgorithmOid     = Cursor;
    (*Capabilities)[Index].AlgorithmOidSize = Size;
    CopyMem (Cursor, Oids[Index], Size);
    Cursor += Size;
  }

  *CapabilityCount = Count;
  return EFI_SUCCESS;
}

extern "C"
EFI_STATUS
EFIAPI
EcitRegisterCryptoCapability (
  IN CONST EFI_GUID  *FeatureIdentifier,
  IN CONST VOID      *EntryData,
  IN UINTN           EntryDataSize
  )
{
  if ((FeatureIdentifier == nullptr) || ((EntryData == nullptr) && (EntryDataSize != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  mAttempts.push_back (*FeatureIdentifier);
  if ((mFailedFeature != nullptr) && CompareGuid (FeatureIdentifier, mFailedFeature)) {
    return EFI_OUT_OF_RESOURCES;
  }

  CapturedReport  Report;

  CopyGuid (&Report.Feature, FeatureIdentifier);
  Report.Data.resize (EntryDataSize);
  if (EntryDataSize != 0) {
    CopyMem (Report.Data.data (), EntryData, EntryDataSize);
  }

  mReports.push_back (std::move (Report));
  return EFI_SUCCESS;
}

static CONST CapturedReport *
FindReport (
  CONST EFI_GUID  &Feature
  )
{
  for (const auto &Report : mReports) {
    if (CompareGuid (&Report.Feature, &Feature)) {
      return &Report;
    }
  }

  return nullptr;
}

static void
ExpectDatabaseTypes (
  CONST EFI_GUID  &Feature
  )
{
  static CONST EFI_GUID  Expected[] = {
    EFI_CERT_X509_GUID,
    EFI_CERT_V2_X509_GUID,
    EFI_CERT_SHA256_GUID,
    EFI_CERT_V2_SHA256_GUID,
    EFI_CERT_SHA384_GUID,
    EFI_CERT_V2_SHA384_GUID,
    EFI_CERT_SHA512_GUID,
    EFI_CERT_V2_SHA512_GUID,
    EFI_CERT_X509_SHA256_GUID,
    EFI_CERT_V2_X509_SHA256_GUID,
    EFI_CERT_X509_SHA384_GUID,
    EFI_CERT_V2_X509_SHA384_GUID,
    EFI_CERT_X509_SHA512_GUID,
    EFI_CERT_V2_X509_SHA512_GUID
  };
  CONST CapturedReport   *Report = FindReport (Feature);

  ASSERT_NE (Report, nullptr);
  ASSERT_EQ (Report->Data.size (), sizeof (Expected));
  for (const auto &ExpectedGuid : Expected) {
    UINTN  Matches = 0;

    for (UINTN Index = 0; Index < ARRAY_SIZE (Expected); Index++) {
      EFI_GUID  Actual;

      CopyMem (&Actual, Report->Data.data () + Index * sizeof (Actual), sizeof (Actual));
      if (CompareGuid (&Actual, &ExpectedGuid)) {
        Matches++;
      }
    }

    EXPECT_EQ (Matches, (UINTN)1);
  }
}

static std::string
ReadOidPayload (
  CONST CapturedReport  &Report
  )
{
  EXPECT_EQ (Report.Data.size () % 8, 0U);
  const auto  End = std::find (Report.Data.begin (), Report.Data.end (), 0);

  EXPECT_NE (End, Report.Data.end ());
  if (End == Report.Data.end ()) {
    return { };
  }

  EXPECT_TRUE (
    std::all_of (
           End,
           Report.Data.end (),
           [] (UINT8 Value) {
    return Value == 0;
  }
           )
    );
  return std::string (Report.Data.begin (), End);
}

class DxeImageVerificationLibEcitGoogleTest : public ::testing::Test {
protected:
  ::testing::StrictMock<MockBaseCryptLib> Crypt;

  void
  SetUp (
    ) override
  {
    mQueryStatus[0]         = EFI_SUCCESS;
    mQueryStatus[1]         = EFI_SUCCESS;
    mQueries                = 0;
    mUnsupportedDigestsOnly = FALSE;
    mInvalidCount           = FALSE;
    mOverflowCount          = FALSE;
    mInvalidOid             = FALSE;
    mDuplicateOid           = FALSE;
    mFailedFeature          = nullptr;
    mAttempts.clear ();
    mReports.clear ();
    EXPECT_CALL (Crypt, GetCryptoOpCapability (::testing::_, ::testing::_, ::testing::_))
      .WillRepeatedly (::testing::Invoke (GetSyntheticCapabilities));
  }
};

TEST_F (DxeImageVerificationLibEcitGoogleTest, ReportsSupportedAlgorithmsAndExactDatabaseTypes) {
  ReportImageVerificationCapabilities ();

  ASSERT_EQ (mQueries, (UINTN)2);
  ASSERT_EQ (mReports.size (), 3U);
  auto  Report = FindReport (gEfiEcitFeatureSbImageVerificationGuid);

  ASSERT_NE (Report, nullptr);
  EXPECT_EQ (
    ReadOidPayload (*Report),
    "1.2.840.113549.1.1.11,2.16.840.1.101.3.4.2.1,"
    "2.16.840.1.101.3.4.2.2,2.16.840.1.101.3.4.2.3"
    );
  ExpectDatabaseTypes (gEfiEcitFeatureSbImageAuthorizationGuid);
  ExpectDatabaseTypes (gEfiEcitFeatureSbImageRevocationGuid);
}

TEST_F (DxeImageVerificationLibEcitGoogleTest, QueryFailuresDoNotPublishPartialAlgorithmSets) {
  for (UINTN FailedQueries = 1; FailedQueries < 4; FailedQueries++) {
    mQueries = 0;
    mAttempts.clear ();
    mReports.clear ();
    mQueryStatus[0] = (FailedQueries & 1) ? EFI_UNSUPPORTED : EFI_SUCCESS;
    mQueryStatus[1] = (FailedQueries & 2) ? EFI_UNSUPPORTED : EFI_SUCCESS;
    ReportImageVerificationCapabilities ();
    EXPECT_EQ (FindReport (gEfiEcitFeatureSbImageVerificationGuid), nullptr);
    EXPECT_EQ (mQueries, (FailedQueries & 1) ? (UINTN)1 : (UINTN)2);
    EXPECT_EQ (mReports.size (), 2U);
    ExpectDatabaseTypes (gEfiEcitFeatureSbImageAuthorizationGuid);
    ExpectDatabaseTypes (gEfiEcitFeatureSbImageRevocationGuid);
  }
}

TEST_F (DxeImageVerificationLibEcitGoogleTest, UnsupportedDigestsProduceAnEmptyAlgorithmSet) {
  mUnsupportedDigestsOnly = TRUE;
  ReportImageVerificationCapabilities ();
  auto  Report = FindReport (gEfiEcitFeatureSbImageVerificationGuid);

  ASSERT_NE (Report, nullptr);
  EXPECT_EQ (Report->Data.size (), 8U);
  EXPECT_TRUE (ReadOidPayload (*Report).empty ());
  EXPECT_EQ (mReports.size (), 3U);
}

TEST_F (DxeImageVerificationLibEcitGoogleTest, InvalidProviderCountsDoNotPublishAlgorithms) {
  mInvalidCount = TRUE;
  ReportImageVerificationCapabilities ();
  EXPECT_EQ (mQueries, (UINTN)1);
  EXPECT_EQ (FindReport (gEfiEcitFeatureSbImageVerificationGuid), nullptr);
  EXPECT_EQ (mReports.size (), 2U);

  mQueries = 0;
  mReports.clear ();
  mAttempts.clear ();
  mInvalidCount  = FALSE;
  mOverflowCount = TRUE;
  ReportImageVerificationCapabilities ();
  EXPECT_EQ (mQueries, (UINTN)2);
  EXPECT_EQ (FindReport (gEfiEcitFeatureSbImageVerificationGuid), nullptr);
  EXPECT_EQ (mReports.size (), 2U);
}

TEST_F (DxeImageVerificationLibEcitGoogleTest, InvalidSignatureOidDoesNotPublishAlgorithms) {
  mInvalidOid = TRUE;
  ReportImageVerificationCapabilities ();
  EXPECT_EQ (FindReport (gEfiEcitFeatureSbImageVerificationGuid), nullptr);
  EXPECT_EQ (mReports.size (), 2U);
}

TEST_F (DxeImageVerificationLibEcitGoogleTest, DuplicateOidsAreReportedOnce) {
  mDuplicateOid = TRUE;
  ReportImageVerificationCapabilities ();
  auto  Report = FindReport (gEfiEcitFeatureSbImageVerificationGuid);

  ASSERT_NE (Report, nullptr);
  EXPECT_EQ (
    ReadOidPayload (*Report),
    "1.2.840.113549.1.1.11,2.16.840.1.101.3.4.2.1,"
    "2.16.840.1.101.3.4.2.2,2.16.840.1.101.3.4.2.3"
    );
}

TEST_F (DxeImageVerificationLibEcitGoogleTest, RegistrationFailureDoesNotPreventOtherReports) {
  CONST EFI_GUID  *Features[] = {
    &gEfiEcitFeatureSbImageVerificationGuid,
    &gEfiEcitFeatureSbImageAuthorizationGuid,
    &gEfiEcitFeatureSbImageRevocationGuid
  };

  for (CONST EFI_GUID *Feature : Features) {
    mQueries       = 0;
    mFailedFeature = Feature;
    mAttempts.clear ();
    mReports.clear ();
    ReportImageVerificationCapabilities ();
    EXPECT_EQ (mAttempts.size (), 3U);
    EXPECT_EQ (mReports.size (), 2U);
    EXPECT_EQ (FindReport (*Feature), nullptr);
  }
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
