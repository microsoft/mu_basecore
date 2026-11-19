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
  #include <Guid/GlobalVariable.h>
  #include <Guid/ImageAuthentication.h>
  #include <Guid/WinCertificate.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/VariablePolicyLib.h>
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
  /**
    Process a variable update using the installed authentication context.

    @param[in] VariableName  Variable to update.
    @param[in] VendorGuid    Variable namespace.
    @param[in] Data          Update buffer.
    @param[in] DataSize      Buffer length in bytes.
    @param[in] Attributes    Requested variable attributes.
    @return Status of the authenticated update.
  **/
  EFI_STATUS
  EFIAPI
  AuthVariableLibProcessVariable (
    IN CHAR16    *VariableName,
    IN EFI_GUID  *VendorGuid,
    IN VOID      *Data,
    IN UINTN     DataSize,
    IN UINT32    Attributes
    );

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
  Build an authenticated update containing a mock X.509 signature list.

  @return Buffer containing the descriptor and signature list.
**/
STATIC std::vector<UINT8>
BuildAuthenticatedX509SignatureListData (
  VOID
  )
{
  CONST UINTN                    SigDataSize       = 22;
  CONST UINTN                    CertSize          = 1;
  CONST UINTN                    SignatureSize     = sizeof (EFI_GUID) + CertSize;
  CONST UINTN                    SignatureListSize = sizeof (EFI_SIGNATURE_LIST) + SignatureSize;
  std::vector<UINT8>             Data (OFFSET_OF_AUTHINFO2_CERT_DATA + SigDataSize + SignatureListSize, 0);
  EFI_VARIABLE_AUTHENTICATION_2  *Authentication;
  EFI_SIGNATURE_LIST             *SignatureList;
  EFI_SIGNATURE_DATA             *SignatureData;

  Authentication                                = reinterpret_cast<EFI_VARIABLE_AUTHENTICATION_2 *>(Data.data ());
  Authentication->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData) + SigDataSize;
  Authentication->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  Authentication->AuthInfo.CertType             = gEfiCertPkcs7Guid;
  Authentication->AuthInfo.CertData[1]          = TWO_BYTE_ENCODE;
  CopyMem (Authentication->AuthInfo.CertData + 13, mSha256Oid, sizeof (mSha256Oid));

  SignatureList                      = reinterpret_cast<EFI_SIGNATURE_LIST *>(Data.data () + OFFSET_OF_AUTHINFO2_CERT_DATA + SigDataSize);
  SignatureList->SignatureType       = gEfiCertX509Guid;
  SignatureList->SignatureListSize   = (UINT32)SignatureListSize;
  SignatureList->SignatureHeaderSize = 0;
  SignatureList->SignatureSize       = (UINT32)SignatureSize;
  SignatureData                      = reinterpret_cast<EFI_SIGNATURE_DATA *>((UINT8 *)SignatureList + sizeof (EFI_SIGNATURE_LIST));
  SignatureData->SignatureData[0]    = 0;

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

//
// KEK self-signed append (edk2#12159): when updating KEK with
// EFI_VARIABLE_APPEND_WRITE set, a signature that fails PK verification
// shall be retried against the existing KEK database before being rejected.
//
STATIC CHAR16  mKekVariableName[] = { 'K', 'E', 'K', 0 };

STATIC std::vector<UINT8>  mExistingKekSignatureList;

/**
  Look up the fixture's existing KEK database.

  @param[in]  VariableName      Name to look up.
  @param[in]  VendorGuid        Namespace to look up.
  @param[out] AuthVariableInfo  Receives the fixture data on success.
  @retval EFI_SUCCESS           The request identifies KEK.
  @retval EFI_NOT_FOUND         The requested variable is not present.
**/
STATIC
EFI_STATUS
EFIAPI
FindVariableKekOnly (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  if ((VendorGuid != NULL) && CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) &&
      (StrCmp (VariableName, mKekVariableName) == 0))
  {
    AuthVariableInfo->Data      = mExistingKekSignatureList.data ();
    AuthVariableInfo->DataSize  = mExistingKekSignatureList.size ();
    AuthVariableInfo->TimeStamp = NULL;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC UINT8  mScratchBuffer[512];
STATIC UINTN  mUpdateVariableCallCount;

/**
  Lend the fixture's scratch buffer to the verifier.

  @param[in,out] ScratchBufferSize  Requested size in bytes; unchanged.
  @param[out]    ScratchBuffer      Receives the fixture buffer on success.
  @retval EFI_SUCCESS               The request fits the buffer.
  @retval EFI_OUT_OF_RESOURCES      The request is too large.
**/
STATIC
EFI_STATUS
EFIAPI
GetScratchBufferSuccess (
  IN OUT UINTN  *ScratchBufferSize,
  OUT    VOID   **ScratchBuffer
  )
{
  if (*ScratchBufferSize > sizeof (mScratchBuffer)) {
    return EFI_OUT_OF_RESOURCES;
  }

  *ScratchBuffer = mScratchBuffer;
  return EFI_SUCCESS;
}

/**
  Count a variable write without modifying a variable store.

  @param[in,out] AuthVariableInfo  Unused update descriptor.
  @retval EFI_SUCCESS              Always returned by this stub.
**/
STATIC
EFI_STATUS
EFIAPI
UpdateVariableCountingSuccess (
  IN OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  (VOID)AuthVariableInfo;
  mUpdateVariableCallCount++;
  return EFI_SUCCESS;
}

/**
  Build a signature list containing one mock X.509 entry.

  @param[in] CertByte  Byte to use as the mock certificate.
  @return Serialized signature list.
**/
STATIC std::vector<UINT8>
BuildX509SignatureListEntry (
  IN UINT8  CertByte
  )
{
  CONST UINTN         CertSize          = 1;
  CONST UINTN         SignatureSize     = sizeof (EFI_GUID) + CertSize;
  CONST UINTN         SignatureListSize = sizeof (EFI_SIGNATURE_LIST) + SignatureSize;
  std::vector<UINT8>  Data (SignatureListSize, 0);
  EFI_SIGNATURE_LIST  *SignatureList;
  EFI_SIGNATURE_DATA  *SignatureData;

  SignatureList                      = reinterpret_cast<EFI_SIGNATURE_LIST *>(Data.data ());
  SignatureList->SignatureType       = gEfiCertX509Guid;
  SignatureList->SignatureListSize   = (UINT32)SignatureListSize;
  SignatureList->SignatureHeaderSize = 0;
  SignatureList->SignatureSize       = (UINT32)SignatureSize;
  SignatureData                      = reinterpret_cast<EFI_SIGNATURE_DATA *>(Data.data () + sizeof (EFI_SIGNATURE_LIST));
  SignatureData->SignatureData[0]    = CertByte;

  return Data;
}

/**
  Simulate an empty variable store for the variable policy library.

  @param[in]     VariableName  Unused variable name.
  @param[in]     VendorGuid    Unused namespace.
  @param[out]    Attributes    Unused optional attributes.
  @param[in,out] DataSize      Unused buffer size.
  @param[out]    Data          Unused optional data buffer.
  @retval EFI_NOT_FOUND        No variable is present.
**/
STATIC
EFI_STATUS
EFIAPI
PolicyGetVariableNotFound (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  OUT UINT32    *Attributes OPTIONAL,
  IN OUT UINTN  *DataSize,
  OUT VOID      *Data OPTIONAL
  )
{
  return EFI_NOT_FOUND;
}

class KekSelfSignedAppendTest : public ::testing::Test {
protected:
  MockBaseCryptLib BaseCryptLibMock;
  AUTH_VAR_LIB_CONTEXT_IN AuthVarContext;
  EFI_GUID VendorGuid = gEfiGlobalVariableGuid;
  UINT32 OriginalPlatformMode;

  /** Enable variable policy in user mode with an existing KEK. **/
  VOID
  SetUp (
    ) override
  {
    ZeroMem (&AuthVarContext, sizeof (AuthVarContext));
    AuthVarContext.FindVariable     = FindVariableKekOnly;
    AuthVarContext.GetScratchBuffer = GetScratchBufferSuccess;
    AuthVarContext.UpdateVariable   = UpdateVariableCountingSuccess;
    mAuthVarLibContextIn            = &AuthVarContext;
    OriginalPlatformMode            = mPlatformMode;
    mPlatformMode                   = USER_MODE;
    mUpdateVariableCallCount        = 0;
    mExistingKekSignatureList       = BuildX509SignatureListEntry (0xAA);
    ASSERT_EQ (InitVariablePolicyLib (PolicyGetVariableNotFound), EFI_SUCCESS);
    ASSERT_TRUE (IsVariablePolicyEnabled ());
  }

  /** Restore the platform mode and release variable policy state. **/
  VOID
  TearDown (
    ) override
  {
    mAuthVarLibContextIn = NULL;
    mPlatformMode        = OriginalPlatformMode;
    EXPECT_EQ (DeinitVariablePolicyLib (), EFI_SUCCESS);
  }
};

/** A KEK-authorized append may proceed when PK verification fails. **/
TEST_F (KekSelfSignedAppendTest, AcceptsSelfSignedAppendWhenPkVerificationFails) {
  std::vector<UINT8>  Data       = BuildAuthenticatedX509SignatureListData ();
  CONST UINT32        Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                                   EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS |
                                   EFI_VARIABLE_APPEND_WRITE;

  // PK verification is attempted first and fails to extract signers.
  EXPECT_CALL (BaseCryptLibMock, CmsGetSignerInfoNum (_, _))
    .WillRepeatedly (Return (1));
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (Return (FALSE));

  // Fallback KEK verification succeeds against the existing KEK database.
  EXPECT_CALL (BaseCryptLibMock, Pkcs7Verify (_, _, _, _, _, _))
    .WillOnce (Return (TRUE));
  EXPECT_CALL (BaseCryptLibMock, X509IsPublicKeySupported (_, _))
    .WillOnce (Return (TRUE));

  EXPECT_EQ (
    AuthVariableLibProcessVariable (
      mKekVariableName,
      &VendorGuid,
      Data.data (),
      Data.size (),
      Attributes
      ),
    EFI_SUCCESS
    );
  EXPECT_GT (mUpdateVariableCallCount, (UINTN)0);
}

/** Neither verification path authorizes a write if both signatures fail. **/
TEST_F (KekSelfSignedAppendTest, RejectsAppendWhenNeitherPkNorKekSignerIsValid) {
  std::vector<UINT8>  Data       = BuildAuthenticatedX509SignatureListData ();
  CONST UINT32        Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                                   EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS |
                                   EFI_VARIABLE_APPEND_WRITE;

  EXPECT_CALL (BaseCryptLibMock, CmsGetSignerInfoNum (_, _))
    .WillRepeatedly (Return (1));
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (Return (FALSE));
  EXPECT_CALL (BaseCryptLibMock, Pkcs7Verify (_, _, _, _, _, _))
    .WillOnce (Return (FALSE));

  EXPECT_EQ (
    AuthVariableLibProcessVariable (
      mKekVariableName,
      &VendorGuid,
      Data.data (),
      Data.size (),
      Attributes
      ),
    EFI_SECURITY_VIOLATION
    );
  EXPECT_EQ (mUpdateVariableCallCount, (UINTN)0);
}

/** Replacing KEK must not fall back to KEK self-authorization. **/
TEST_F (KekSelfSignedAppendTest, RejectsReplaceWithoutPkWhenNotAppending) {
  std::vector<UINT8>  Data       = BuildAuthenticatedX509SignatureListData ();
  CONST UINT32        Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                                   EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;

  // Not an append: the KEK self-signed fallback must never be attempted.
  EXPECT_CALL (BaseCryptLibMock, CmsGetSignerInfoNum (_, _))
    .WillOnce (Return (1));
  EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
    .WillOnce (Return (FALSE));
  EXPECT_CALL (BaseCryptLibMock, Pkcs7Verify (_, _, _, _, _, _))
    .Times (0);

  EXPECT_EQ (
    AuthVariableLibProcessVariable (
      mKekVariableName,
      &VendorGuid,
      Data.data (),
      Data.size (),
      Attributes
      ),
    EFI_SECURITY_VIOLATION
    );
  EXPECT_EQ (mUpdateVariableCallCount, (UINTN)0);
}

TEST_F (KekSelfSignedAppendTest, RejectsUnsupportedX509PublicKey) {
  std::vector<UINT8>  Data = BuildAuthenticatedX509SignatureListData ();

  mPlatformMode = SETUP_MODE;
  EXPECT_CALL (BaseCryptLibMock, X509IsPublicKeySupported (_, _))
    .WillOnce (Return (FALSE));
  EXPECT_EQ (
    ProcessVarWithPk (
      mKekVariableName,
      &VendorGuid,
      Data.data (),
      Data.size (),
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
      FALSE
      ),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (mUpdateVariableCallCount, (UINTN)0);
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
