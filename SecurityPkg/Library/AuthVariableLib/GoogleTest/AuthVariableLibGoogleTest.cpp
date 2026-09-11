/** @file
  Unit tests for the AuthVariableLib SignerInfo cardinality policy.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockBaseCryptLib.h>

#include <vector>

extern "C" {
  #include <Uefi.h>
  #include <Guid/ImageAuthentication.h>
  #include <Guid/WinCertificate.h>
  #include <Library/BaseMemoryLib.h>
  #include "../AuthServiceInternal.h"

  /**
    Verify a time-based authenticated variable update.

    @param[in]  VariableName    Variable to update.
    @param[in]  VendorGuid      Variable namespace.
    @param[in]  Data            Authentication descriptor and payload.
    @param[in]  DataSize        Size of Data in bytes.
    @param[in]  Attributes      Variable attributes.
    @param[in]  AuthVarType     Authorization method.
    @param[in]  OrgTimeStamp    Previous timestamp, if any.
    @param[out] VarPayloadPtr   Receives the payload pointer on success.
    @param[out] VarPayloadSize  Receives the payload length on success.
    @return Status of payload verification.
  **/
  EFI_STATUS
  VerifyTimeBasedPayload (
    IN     CHAR16        *VariableName,
    IN     EFI_GUID      *VendorGuid,
    IN     VOID          *Data,
    IN     UINTN         DataSize,
    IN     UINT32        Attributes,
    IN     AUTHVAR_TYPE  AuthVarType,
    IN     EFI_TIME      *OrgTimeStamp,
    OUT    UINT8         **VarPayloadPtr,
    OUT    UINTN         *VarPayloadSize
    );
}

using ::testing::_;
using ::testing::Return;

STATIC CONST UINT8  mSha256Oid[] = {
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01
};

STATIC CHAR16  mVariableName[] = { 'T', 'e', 's', 't', 'V', 'a', 'r', 'i', 'a', 'b', 'l', 'e', 0 };

/**
  Build an authenticated update with a mock signature and one payload byte.

  @return Buffer containing the authentication descriptor and payload.
**/
STATIC std::vector<UINT8>
BuildAuthenticatedVariableData (
  VOID
  )
{
  CONST UINTN                    SigDataSize = 22;
  std::vector<UINT8>             Data (OFFSET_OF_AUTHINFO2_CERT_DATA + SigDataSize + 1, 0);
  EFI_VARIABLE_AUTHENTICATION_2  *Authentication;

  Authentication                                = reinterpret_cast<EFI_VARIABLE_AUTHENTICATION_2 *>(Data.data ());
  Authentication->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData) + SigDataSize;
  Authentication->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  Authentication->AuthInfo.CertType             = gEfiCertPkcs7Guid;
  Authentication->AuthInfo.CertData[1]          = TWO_BYTE_ENCODE;
  CopyMem (Authentication->AuthInfo.CertData + 13, mSha256Oid, sizeof (mSha256Oid));

  return Data;
}

/**
  Fail scratch allocation so the test can distinguish an accepted signer count.

  @param[in,out] ScratchBufferSize  Requested size; unchanged.
  @param[out]    ScratchBuffer      Unused on failure.
  @retval EFI_OUT_OF_RESOURCES      Always returned by this stub.
**/
STATIC
EFI_STATUS
EFIAPI
GetScratchBufferOutOfResources (
  IN OUT UINTN  *ScratchBufferSize,
  OUT    VOID   **ScratchBuffer
  )
{
  (VOID)ScratchBufferSize;
  (VOID)ScratchBuffer;
  return EFI_OUT_OF_RESOURCES;
}

class VerifyTimeBasedPayloadSignerInfoTest : public ::testing::Test {
protected:
  MockBaseCryptLib BaseCryptLibMock;
  AUTH_VAR_LIB_CONTEXT_IN AuthVarContext;
  EFI_GUID VendorGuid = { 0 };
  UINT8 *Payload;
  UINTN PayloadSize;

  /** Install the mock authentication context before each signer-count test. **/
  VOID
  SetUp (
    ) override
  {
    ZeroMem (&AuthVarContext, sizeof (AuthVarContext));
    AuthVarContext.GetScratchBuffer = GetScratchBufferOutOfResources;
    mAuthVarLibContextIn            = &AuthVarContext;
    Payload                         = NULL;
    PayloadSize                     = 0;
  }

  /** Release the fixture's reference from the library context. **/
  VOID
  TearDown (
    ) override
  {
    mAuthVarLibContextIn = NULL;
  }

  /**
    Verify an update using the specified mock CMS signer count.

    @param[in] SignerInfoCount  Number reported by the mock CMS parser.
    @return Status returned by the payload verifier.
  **/
  EFI_STATUS
  VerifyWithSignerInfoCount (
    IN UINTN  SignerInfoCount
    )
  {
    std::vector<UINT8>  Data = BuildAuthenticatedVariableData ();

    EXPECT_CALL (BaseCryptLibMock, CmsGetSignerInfoNum (_, _))
      .WillOnce (Return (SignerInfoCount));

    return VerifyTimeBasedPayload (
             mVariableName,
             &VendorGuid,
             Data.data (),
             Data.size (),
             EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
             AuthVarTypePayload,
             NULL,
             &Payload,
             &PayloadSize
             );
  }
};

/** An unsigned update fails the signer-count check. **/
TEST_F (VerifyTimeBasedPayloadSignerInfoTest, RejectsZeroSignerInfos) {
  EXPECT_EQ (VerifyWithSignerInfoCount (0), EFI_SECURITY_VIOLATION);
}

/** An update with more than one signer fails the signer-count check. **/
TEST_F (VerifyTimeBasedPayloadSignerInfoTest, RejectsMultipleSignerInfos) {
  EXPECT_EQ (VerifyWithSignerInfoCount (2), EFI_SECURITY_VIOLATION);
}

/** A single signer passes the count check and reaches scratch allocation. **/
TEST_F (VerifyTimeBasedPayloadSignerInfoTest, AcceptsOneSignerInfo) {
  EXPECT_EQ (VerifyWithSignerInfoCount (1), EFI_OUT_OF_RESOURCES);
}

/**
  Run the AuthVariableLib host tests.

  @param[in] argc  Number of command-line arguments.
  @param[in] argv  Command-line arguments for GoogleTest.
  @return GoogleTest process exit status.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
