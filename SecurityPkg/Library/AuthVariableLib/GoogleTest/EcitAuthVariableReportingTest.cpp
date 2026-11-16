/** @file
  Regression tests for authenticated-variable ECIT reporting.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockBaseCryptLib.h>
#include <string>

extern "C" {
  #include "../AuthServiceInternal.h"
  #include <Library/VariablePolicyLib.h>
  #include <Guid/CryptoIndicatorTable.h>
  #include <Guid/CryptoOpId.h>
}

STATIC EFI_STATUS   mQueryStatus[2];
STATIC UINTN        mQueries;
STATIC UINTN        mRegistrations;
STATIC BOOLEAN      mOnlyUnsupportedDigests;
STATIC std::string  mReportedOids;

STATIC
EFI_STATUS
EFIAPI
GetSyntheticCapabilities (
  IN CONST EFI_GUID             *OperationId,
  OUT BASE_CRYPT_OP_CAPABILITY  **Capabilities,
  OUT UINTN                     *CapabilityCount
  )
{
  STATIC CONST CHAR8     *SignatureOids[]         = { "1.2.840.113549.1.1.11" };
  STATIC CONST CHAR8     *UnsupportedDigestOids[] = { "2.16.840.1.101.3.4.2.4" };
  STATIC CONST CHAR8     *DigestOids[]            = {
    "1.3.14.3.2.26",
    "2.16.840.1.101.3.4.2.4",
    "2.16.840.1.101.3.4.2.1",
    "2.16.840.1.101.3.4.2.2",
    "2.16.840.1.101.3.4.2.3"
  };
  STATIC CONST EFI_GUID  *Operations[] = { &gCryptoOpCmsVerifyGuid, &gCryptoOpCmsContentDigestGuid };
  UINTN                  Query;
  CONST CHAR8            **Oids;
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
  if (EFI_ERROR (mQueryStatus[Query]) || (mOnlyUnsupportedDigests && (Query == 0))) {
    return mQueryStatus[Query];
  }

  Oids  = (Query == 0) ? SignatureOids : DigestOids;
  Count = (Query == 0) ? ARRAY_SIZE (SignatureOids) : ARRAY_SIZE (DigestOids);
  if (mOnlyUnsupportedDigests) {
    Oids  = UnsupportedDigestOids;
    Count = ARRAY_SIZE (UnsupportedDigestOids);
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
  EXPECT_TRUE (CompareGuid (FeatureIdentifier, &gEfiEcitFeatureSbDatabaseUpdateVerificationGuid));
  EXPECT_EQ (EntryDataSize % 8, 0U);
  EXPECT_NE (ScanMem8 (EntryData, EntryDataSize, '\0'), nullptr);
  mRegistrations++;
  mReportedOids = (CONST CHAR8 *)EntryData;
  return EFI_SUCCESS;
}

extern "C"
BOOLEAN
EFIAPI
IsVariablePolicyEnabled (
  VOID
  )
{
  ADD_FAILURE () << "ECIT reporting must not evaluate variable policy";
  return FALSE;
}

class EcitAuthVariableReportingTest : public ::testing::Test {
protected:
  ::testing::StrictMock<MockBaseCryptLib> Crypt;

  void
  SetUp (
    ) override
  {
    mQueryStatus[0]         = EFI_SUCCESS;
    mQueryStatus[1]         = EFI_SUCCESS;
    mQueries                = 0;
    mRegistrations          = 0;
    mOnlyUnsupportedDigests = FALSE;
    mReportedOids.clear ();
    EXPECT_CALL (Crypt, GetCryptoOpCapability (::testing::_, ::testing::_, ::testing::_))
      .WillRepeatedly (::testing::Invoke (GetSyntheticCapabilities));
  }
};

TEST_F (EcitAuthVariableReportingTest, ReportsOnlyImplementedDigests) {
  ReportDatabaseUpdateVerificationCapability ();
  ASSERT_EQ (mQueries, 2U);
  ASSERT_EQ (mRegistrations, 1U);
  EXPECT_EQ (
    mReportedOids,
    "1.2.840.113549.1.1.11,"
    "2.16.840.1.101.3.4.2.1,2.16.840.1.101.3.4.2.2,2.16.840.1.101.3.4.2.3"
    );
}

TEST_F (EcitAuthVariableReportingTest, QueryFailuresDoNotPublishPartialOrEmptySets) {
  for (UINTN FailedQueries = 1; FailedQueries < 4; FailedQueries++) {
    mQueries        = 0;
    mRegistrations  = 0;
    mQueryStatus[0] = (FailedQueries & 1) ? EFI_UNSUPPORTED : EFI_SUCCESS;
    mQueryStatus[1] = (FailedQueries & 2) ? EFI_UNSUPPORTED : EFI_SUCCESS;
    ReportDatabaseUpdateVerificationCapability ();
    EXPECT_EQ (mRegistrations, 0U);
    EXPECT_EQ (mQueries, (FailedQueries & 1) ? 1U : 2U);
  }
}

TEST_F (EcitAuthVariableReportingTest, NonemptyDigestSetCanFilterToEmpty) {
  mOnlyUnsupportedDigests = TRUE;
  ReportDatabaseUpdateVerificationCapability ();
  EXPECT_EQ (mQueries, 2U);
  EXPECT_EQ (mRegistrations, 1U);
  EXPECT_TRUE (mReportedOids.empty ());
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
