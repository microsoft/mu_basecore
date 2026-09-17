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
  #include "../AuthServiceInternal.h"

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

STATIC
EFI_STATUS
EFIAPI
FindVariableNotFound (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  (VOID)VariableName;
  (VOID)VendorGuid;
  (VOID)AuthVariableInfo;
  return EFI_NOT_FOUND;
}

class VerifyTimeBasedPayloadSignerInfoTest : public ::testing::Test {
protected:
  MockBaseCryptLib BaseCryptLibMock;
  AUTH_VAR_LIB_CONTEXT_IN AuthVarContext;
  EFI_GUID VendorGuid = { 0 };
  UINT8 *Payload;
  UINTN PayloadSize;
  UINT32 OriginalPlatformMode;

  VOID
  SetUp (
    ) override
  {
    ZeroMem (&AuthVarContext, sizeof (AuthVarContext));
    AuthVarContext.FindVariable     = FindVariableNotFound;
    AuthVarContext.GetScratchBuffer = GetScratchBufferOutOfResources;
    mAuthVarLibContextIn            = &AuthVarContext;
    Payload                         = NULL;
    PayloadSize                     = 0;
    OriginalPlatformMode            = mPlatformMode;
  }

  VOID
  TearDown (
    ) override
  {
    mAuthVarLibContextIn = NULL;
    mPlatformMode        = OriginalPlatformMode;
  }

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

TEST_F (VerifyTimeBasedPayloadSignerInfoTest, RejectsZeroSignerInfos) {
  EXPECT_EQ (VerifyWithSignerInfoCount (0), EFI_SECURITY_VIOLATION);
}

TEST_F (VerifyTimeBasedPayloadSignerInfoTest, RejectsMultipleSignerInfos) {
  EXPECT_EQ (VerifyWithSignerInfoCount (2), EFI_SECURITY_VIOLATION);
}

TEST_F (VerifyTimeBasedPayloadSignerInfoTest, AcceptsOneSignerInfo) {
  EXPECT_EQ (VerifyWithSignerInfoCount (1), EFI_OUT_OF_RESOURCES);
}

TEST_F (VerifyTimeBasedPayloadSignerInfoTest, RejectsPrivateTimeBasedVariable) {
  UINT8  Data = 0;

  EXPECT_EQ (
    ProcessVariable (
      mVariableName,
      &VendorGuid,
      &Data,
      sizeof (Data),
      EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
      ),
    EFI_UNSUPPORTED
    );
}

TEST_F (VerifyTimeBasedPayloadSignerInfoTest, RejectsUnsupportedX509PublicKey) {
  std::vector<UINT8>  Data              = BuildAuthenticatedX509SignatureListData ();
  CHAR16              KekVariableName[] = { 'K', 'E', 'K', 0 };

  mPlatformMode = SETUP_MODE;
  EXPECT_CALL (BaseCryptLibMock, X509IsPublicKeySupported (_, _))
    .WillOnce (Return (FALSE));

  // Use the KEK variable name/GUID so CheckSignatureListFormat actually
  // walks the appended X509 signature list and enforces the public key
  // capability check; an unrelated name/GUID pair is treated as opaque
  // payload and skips the check entirely.
  EXPECT_EQ (
    ProcessVarWithPk (
      KekVariableName,
      &gEfiGlobalVariableGuid,
      Data.data (),
      Data.size (),
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
      FALSE
      ),
    EFI_INVALID_PARAMETER
    );
}

//
// KEK self-signed append (edk2#12159): when updating KEK with
// EFI_VARIABLE_APPEND_WRITE set, a signature that fails PK verification
// shall be retried against the existing KEK database before being rejected.
//
STATIC CHAR16  mKekVariableName[] = { 'K', 'E', 'K', 0 };

STATIC std::vector<UINT8>  mExistingKekSignatureList;

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

class KekSelfSignedAppendTest : public ::testing::Test {
protected:
  MockBaseCryptLib BaseCryptLibMock;
  AUTH_VAR_LIB_CONTEXT_IN AuthVarContext;
  EFI_GUID VendorGuid = gEfiGlobalVariableGuid;
  UINT32 OriginalPlatformMode;

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
  }

  VOID
  TearDown (
    ) override
  {
    mAuthVarLibContextIn = NULL;
    mPlatformMode        = OriginalPlatformMode;
  }
};

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

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
