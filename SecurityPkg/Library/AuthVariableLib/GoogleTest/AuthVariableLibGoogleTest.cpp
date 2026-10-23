/** @file
  Unit tests for AuthVariableLib authentication and signature-list policy.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockBaseCryptLib.h>

#include <vector>
#include <tuple>

extern "C" {
  #include <Uefi.h>
  #include <Guid/GlobalVariable.h>
  #include <Guid/ImageAuthentication.h>
  #include <Guid/WinCertificate.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/PcdLib.h>
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
BuildAuthenticatedVariableDeleteData (
  VOID
  )
{
  CONST UINTN                    SigDataSize = 22;
  std::vector<UINT8>             Data (OFFSET_OF_AUTHINFO2_CERT_DATA + SigDataSize, 0);
  EFI_VARIABLE_AUTHENTICATION_2  *Authentication;

  Authentication                                = reinterpret_cast<EFI_VARIABLE_AUTHENTICATION_2 *>(Data.data ());
  Authentication->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData) + SigDataSize;
  Authentication->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  Authentication->AuthInfo.CertType             = gEfiCertPkcs7Guid;

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

STATIC std::vector<UINT8>
BuildSignatureList (
  IN CONST EFI_GUID  *SignatureType,
  IN UINT32          SignatureSize
  )
{
  CONST UINTN         SignatureListSize = sizeof (EFI_SIGNATURE_LIST) + SignatureSize;
  std::vector<UINT8>  Data (SignatureListSize, 0);
  EFI_SIGNATURE_LIST  *SignatureList;

  SignatureList                      = reinterpret_cast<EFI_SIGNATURE_LIST *>(Data.data ());
  SignatureList->SignatureType       = *SignatureType;
  SignatureList->SignatureListSize   = (UINT32)SignatureListSize;
  SignatureList->SignatureHeaderSize = 0;
  SignatureList->SignatureSize       = SignatureSize;
  return Data;
}

STATIC CHAR16  mDbVariableName[] = { 'd', 'b', 0 };

TEST (CheckSignatureListFormatTest, AcceptsV1X509Sha512) {
  std::vector<UINT8>  Data = BuildSignatureList (
                               &gEfiCertX509Sha512Guid,
                               sizeof (EFI_GUID) + SHA512_DIGEST_SIZE + sizeof (EFI_TIME)
                               );

  EXPECT_EQ (
    CheckSignatureListFormat (
      mDbVariableName,
      &gEfiImageSecurityDatabaseGuid,
      Data.data (),
      Data.size ()
      ),
    EFI_SUCCESS
    );
}

TEST (CheckSignatureListFormatTest, AcceptsV2X509Sha512WithoutOwner) {
  std::vector<UINT8>  Data = BuildSignatureList (
                               &gEfiCertV2X509Sha512Guid,
                               SHA512_DIGEST_SIZE
                               );

  EXPECT_EQ (
    CheckSignatureListFormat (
      mDbVariableName,
      &gEfiImageSecurityDatabaseGuid,
      Data.data (),
      Data.size ()
      ),
    EFI_SUCCESS
    );
}

TEST (CheckSignatureListFormatTest, RejectsV2X509Sha512WithOwner) {
  std::vector<UINT8>  Data = BuildSignatureList (
                               &gEfiCertV2X509Sha512Guid,
                               sizeof (EFI_GUID) + SHA512_DIGEST_SIZE
                               );

  EXPECT_EQ (
    CheckSignatureListFormat (
      mDbVariableName,
      &gEfiImageSecurityDatabaseGuid,
      Data.data (),
      Data.size ()
      ),
    EFI_INVALID_PARAMETER
    );
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

STATIC
EFI_STATUS
EFIAPI
UpdateVariableSuccess (
  IN AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  (VOID)AuthVariableInfo;
  return EFI_SUCCESS;
}

STATIC UINT8   mSetupModeValue   = USER_MODE;
STATIC UINT8   mSecureBootValue  = SECURE_BOOT_MODE_ENABLE;
STATIC CHAR16  mSetupModeName[]  = { 'S', 'e', 't', 'u', 'p', 'M', 'o', 'd', 'e', 0 };
STATIC CHAR16  mSecureBootName[] = { 'S', 'e', 'c', 'u', 'r', 'e', 'B', 'o', 'o', 't', 0 };

STATIC
EFI_STATUS
EFIAPI
FindPlatformStateVariable (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, mSetupModeName) == 0)) {
    AuthVariableInfo->Data     = &mSetupModeValue;
    AuthVariableInfo->DataSize = sizeof (mSetupModeValue);
    return EFI_SUCCESS;
  }

  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) && (StrCmp (VariableName, mSecureBootName) == 0)) {
    AuthVariableInfo->Data     = &mSecureBootValue;
    AuthVariableInfo->DataSize = sizeof (mSecureBootValue);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
BOOLEAN
EFIAPI
AtRuntimeFalse (
  VOID
  )
{
  return FALSE;
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

TEST_F (VerifyTimeBasedPayloadSignerInfoTest, AllowsPkDeletionWhenVariablePolicyDisabled) {
  std::vector<UINT8>  Data             = BuildAuthenticatedVariableDeleteData ();
  CHAR16              PkVariableName[] = { 'P', 'K', 0 };

  AuthVarContext.FindVariable   = FindPlatformStateVariable;
  AuthVarContext.UpdateVariable = UpdateVariableSuccess;
  AuthVarContext.AtRuntime      = AtRuntimeFalse;
  mPlatformMode                 = USER_MODE;

  EXPECT_EQ (
    ProcessVarWithPk (
      PkVariableName,
      &gEfiGlobalVariableGuid,
      Data.data (),
      Data.size (),
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
      TRUE
      ),
    EFI_SUCCESS
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

STATIC UINT8               mScratchBuffer[512];
STATIC UINTN               mUpdateVariableCallCount;
STATIC std::vector<UINT8>  mUpdatedAuthenticatedPayload;
STATIC UINT32              mUpdatedAuthenticatedAttributes;

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
  mUpdateVariableCallCount++;
  if (AuthVariableInfo->TimeStamp != NULL) {
    UINT8  *Data = static_cast<UINT8 *>(AuthVariableInfo->Data);
    mUpdatedAuthenticatedPayload.assign (Data, Data + AuthVariableInfo->DataSize);
    mUpdatedAuthenticatedAttributes = AuthVariableInfo->Attributes;
  }

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

STATIC CHAR16  mPkVariableName[]        = { 'P', 'K', 0 };
STATIC CHAR16  mSetupModeVariableName[] = { 'S', 'e', 't', 'u', 'p', 'M', 'o', 'd', 'e', 0 };
STATIC CHAR16  mDbxVariableName[]       = { 'd', 'b', 'x', 0 };
STATIC CHAR16  mDbtVariableName[]       = { 'd', 'b', 't', 0 };

STATIC struct {
  CHAR16      *Name;
  EFI_GUID    *Guid;
} mSignatureListVariables[] = {
  { mPkVariableName,  &gEfiGlobalVariableGuid        },
  { mKekVariableName, &gEfiGlobalVariableGuid        },
  { mDbVariableName,  &gEfiImageSecurityDatabaseGuid },
  { mDbxVariableName, &gEfiImageSecurityDatabaseGuid },
  { mDbtVariableName, &gEfiImageSecurityDatabaseGuid }
};

// These bytes are not DER certificates: this suite tests the mocked crypto boundary.
STATIC CONST UINTN  mMockCertificateSize = 3;

STATIC std::vector<UINT8>
BuildMockCertificateList (
  IN bool   V2,
  IN UINTN  Count
  )
{
  UINTN               OwnerSize = V2 ? 0 : sizeof (EFI_GUID);
  UINTN               EntrySize = OwnerSize + mMockCertificateSize;
  std::vector<UINT8>  Data (sizeof (EFI_SIGNATURE_LIST) + Count * EntrySize, 0xA5);
  EFI_SIGNATURE_LIST  *List = reinterpret_cast<EFI_SIGNATURE_LIST *>(Data.data ());

  List->SignatureType       = V2 ? gEfiCertV2X509Guid : gEfiCertX509Guid;
  List->SignatureListSize   = (UINT32)Data.size ();
  List->SignatureHeaderSize = 0;
  List->SignatureSize       = (UINT32)EntrySize;
  for (UINTN Index = 0; Index < Count; Index++) {
    SetMem (Data.data () + sizeof (*List) + Index * EntrySize + OwnerSize, mMockCertificateSize, (UINT8)(Index + 1));
  }

  return Data;
}

STATIC
EFI_STATUS
EFIAPI
FindVariableTrustedKeys (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  STATIC UINT8  SetupMode;

  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) &&
      (StrCmp (VariableName, mSetupModeVariableName) == 0))
  {
    SetupMode                   = (UINT8)mPlatformMode;
    AuthVariableInfo->Data      = &SetupMode;
    AuthVariableInfo->DataSize  = sizeof (SetupMode);
    AuthVariableInfo->TimeStamp = NULL;
    return EFI_SUCCESS;
  }

  if (CompareGuid (VendorGuid, &gEfiGlobalVariableGuid) &&
      ((StrCmp (VariableName, mPkVariableName) == 0) || (StrCmp (VariableName, mKekVariableName) == 0)))
  {
    AuthVariableInfo->Data      = mExistingKekSignatureList.data ();
    AuthVariableInfo->DataSize  = mExistingKekSignatureList.size ();
    AuthVariableInfo->TimeStamp = NULL;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

class FullX509SignatureListTest : public ::testing::TestWithParam<std::tuple<UINTN, bool> > {
protected:
  MockBaseCryptLib BaseCryptLibMock;
  AUTH_VAR_LIB_CONTEXT_IN AuthVarContext;
  AUTH_VAR_LIB_CONTEXT_IN *OriginalContext;
  UINT32 OriginalPlatformMode;

  VOID
  SetUp (
    ) override
  {
    OriginalContext      = mAuthVarLibContextIn;
    OriginalPlatformMode = mPlatformMode;
    ZeroMem (&AuthVarContext, sizeof (AuthVarContext));
    AuthVarContext.FindVariable     = FindVariableTrustedKeys;
    AuthVarContext.GetScratchBuffer = GetScratchBufferSuccess;
    AuthVarContext.UpdateVariable   = UpdateVariableCountingSuccess;
    AuthVarContext.AtRuntime        = AtRuntimeFalse;
    mAuthVarLibContextIn            = &AuthVarContext;
    mExistingKekSignatureList       = BuildX509SignatureListEntry (0xAA);
    mUpdateVariableCallCount        = 0;
  }

  VOID
  TearDown (
    ) override
  {
    mAuthVarLibContextIn = OriginalContext;
    mPlatformMode        = OriginalPlatformMode;
  }

  EFI_STATUS
  Check (
    IN std::vector<UINT8>  &Data
    )
  {
    auto  &Variable = mSignatureListVariables[std::get<0> (GetParam ())];

    return CheckSignatureListFormat (Variable.Name, Variable.Guid, Data.data (), Data.size ());
  }

  VOID
  ExpectCertificates (
    IN UINT8  *Data,
    IN UINTN  Count,
    IN bool   V2,
    IN bool   LastSupported
    )
  {
    UINTN  OwnerSize = V2 ? 0 : sizeof (EFI_GUID);

    for (UINTN Index = 0; Index < Count; Index++) {
      EXPECT_CALL (
        BaseCryptLibMock,
        X509IsPublicKeySupported (
          Data + sizeof (EFI_SIGNATURE_LIST) + Index * (OwnerSize + mMockCertificateSize) + OwnerSize,
          mMockCertificateSize
          )
        )
        .WillOnce (Return (((Index + 1 < Count) || LastSupported) ? TRUE : FALSE));
    }
  }

  VOID
  CheckThroughProcessVariable (
    IN std::vector<UINT8>  &Payload,
    IN UINTN               Count,
    IN bool                LaterList,
    IN bool                Supported = false
    )
  {
    auto          &Variable  = mSignatureListVariables[std::get<0> (GetParam ())];
    bool          V2         = std::get<1> (GetParam ());
    CONST UINT32  Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                               EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;

    for (UINTN Mode = 0; Mode < 2; Mode++) {
      SCOPED_TRACE (Mode == 0 ? "SetupMode" : "authenticated UserMode");
      std::vector<UINT8>  Data = BuildAuthenticatedVariableData ();
      Data.pop_back ();
      UINTN  PayloadOffset = Data.size ();
      Data.insert (Data.end (), Payload.begin (), Payload.end ());
      mPlatformMode            = Mode == 0 ? SETUP_MODE : USER_MODE;
      mUpdateVariableCallCount = 0;
      mUpdatedAuthenticatedPayload.clear ();
      mUpdatedAuthenticatedAttributes = 0;

      // db/dbx/dbt retry with KEK after PK rejects the payload.
      UINTN  Passes = (!Supported && (std::get<0> (GetParam ()) >= 2)) ? 2 : 1;
      if ((Mode != 0) || ((std::get<0> (GetParam ()) == 0) && FeaturePcdGet (PcdRequireSelfSignedPk))) {
        EXPECT_CALL (BaseCryptLibMock, CmsGetSignerInfoNum (_, _))
          .Times (Passes).WillRepeatedly (Return (1));
        EXPECT_CALL (BaseCryptLibMock, Pkcs7Verify (_, _, _, _, _, _))
          .Times (Passes).WillRepeatedly (Return (TRUE));
        if (Mode != 0) {
          UINT8  *TrustedCert = mExistingKekSignatureList.data () + sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_GUID);
          EXPECT_CALL (BaseCryptLibMock, Pkcs7GetSigners (_, _, _, _, _, _))
            .WillOnce (
               ::testing::DoAll (
                            ::testing::SetArgPointee<4> (TrustedCert),
                            ::testing::SetArgPointee<5> ((UINTN)1),
                            Return (TRUE)
                            )
               );
          EXPECT_CALL (BaseCryptLibMock, Pkcs7FreeSigners (TrustedCert)).Times (1);
        }
      } else {
        EXPECT_CALL (BaseCryptLibMock, CmsGetSignerInfoNum (_, _)).Times (0);
        EXPECT_CALL (BaseCryptLibMock, Pkcs7Verify (_, _, _, _, _, _)).Times (0);
      }

      {
        ::testing::InSequence  Sequence;
        if (Count == 0) {
          EXPECT_CALL (BaseCryptLibMock, X509IsPublicKeySupported (_, _)).Times (0);
        }

        for (UINTN Pass = 0; Pass < Passes; Pass++) {
          UINT8  *List = Data.data () + PayloadOffset;
          if (LaterList) {
            ExpectCertificates (List, 1, V2, true);
            List += reinterpret_cast<EFI_SIGNATURE_LIST *>(List)->SignatureListSize;
            ExpectCertificates (List, Count, !V2, Supported);
          } else if (Count != 0) {
            ExpectCertificates (List, Count, V2, Supported);
          }
        }
      }

      EXPECT_EQ (
        AuthVariableLibProcessVariable (Variable.Name, Variable.Guid, Data.data (), Data.size (), Attributes),
        Supported ? EFI_SUCCESS : EFI_INVALID_PARAMETER
        );
      if (Supported) {
        EXPECT_GT (mUpdateVariableCallCount, (UINTN)0);
        EXPECT_EQ (mUpdatedAuthenticatedPayload, Payload);
        EXPECT_EQ (mUpdatedAuthenticatedAttributes, Attributes);
      } else {
        EXPECT_EQ (mUpdateVariableCallCount, (UINTN)0);
      }

      ::testing::Mock::VerifyAndClearExpectations (&BaseCryptLibMock);
    }
  }
};

TEST_P (FullX509SignatureListTest, AcceptsSupportedCertificateWithExactCryptoSpan) {
  bool                V2   = std::get<1> (GetParam ());
  std::vector<UINT8>  Data = BuildMockCertificateList (V2, 1);

  ExpectCertificates (Data.data (), 1, V2, true);
  EXPECT_EQ (Check (Data), EFI_SUCCESS);
}

TEST_P (FullX509SignatureListTest, RejectsUnsupportedFirstCertificate) {
  bool                V2   = std::get<1> (GetParam ());
  std::vector<UINT8>  Data = BuildMockCertificateList (V2, 1);

  ExpectCertificates (Data.data (), 1, V2, false);
  EXPECT_EQ (Check (Data), EFI_INVALID_PARAMETER);
}

TEST_P (FullX509SignatureListTest, ChecksEveryCertificateInLaterMixedFormatList) {
  bool                V2          = std::get<1> (GetParam ());
  std::vector<UINT8>  Data        = BuildMockCertificateList (V2, 1);
  UINTN               LaterCount  = V2 ? 2 : 1;
  std::vector<UINT8>  Later       = BuildMockCertificateList (!V2, LaterCount);
  UINTN               LaterOffset = Data.size ();

  Data.insert (Data.end (), Later.begin (), Later.end ());
  ::testing::InSequence  Sequence;

  ExpectCertificates (Data.data (), 1, V2, true);
  ExpectCertificates (Data.data () + LaterOffset, LaterCount, !V2, false);
  EXPECT_EQ (Check (Data), EFI_INVALID_PARAMETER);
}

TEST_P (FullX509SignatureListTest, AcceptsSupportedMixedFormatListsExceptMultiplePk) {
  bool                V2          = std::get<1> (GetParam ());
  std::vector<UINT8>  Data        = BuildMockCertificateList (V2, 1);
  UINTN               LaterCount  = V2 ? 2 : 1;
  std::vector<UINT8>  Later       = BuildMockCertificateList (!V2, LaterCount);
  UINTN               LaterOffset = Data.size ();

  Data.insert (Data.end (), Later.begin (), Later.end ());
  ::testing::InSequence  Sequence;

  ExpectCertificates (Data.data (), 1, V2, true);
  ExpectCertificates (Data.data () + LaterOffset, LaterCount, !V2, true);
  EXPECT_EQ (Check (Data), std::get<0> (GetParam ()) == 0 ? EFI_INVALID_PARAMETER : EFI_SUCCESS);
}

TEST_P (FullX509SignatureListTest, RejectsMalformedListsBeforeCrypto) {
  bool  V2 = std::get<1> (GetParam ());

  EXPECT_CALL (BaseCryptLibMock, X509IsPublicKeySupported (_, _)).Times (0);

  for (UINTN Case = 0; Case < (V2 ? 15 : 13); Case++) {
    SCOPED_TRACE (Case);
    std::vector<UINT8>  Data  = BuildMockCertificateList (V2, 1);
    EFI_SIGNATURE_LIST  *List = reinterpret_cast<EFI_SIGNATURE_LIST *>(Data.data ());
    switch (Case) {
      case 0:
        List->SignatureListSize = 0;
        break;
      case 1:
        List->SignatureSize = 0;
        break;
      case 2:
        Data.resize (sizeof (EFI_SIGNATURE_LIST) - 1);
        break;
      case 3:
        List->SignatureListSize = sizeof (EFI_SIGNATURE_LIST) - 1;
        break;
      case 4:
        List->SignatureListSize++;
        break;
      case 5:
        List->SignatureHeaderSize = List->SignatureListSize;
        break;
      case 6:
        List->SignatureHeaderSize = MAX_UINT32;
        break;
      case 7:
        List->SignatureSize = MAX_UINT32;
        break;
      case 8:
        // A complete owner with no certificate (or zero-sized V2 entry).
        List->SignatureSize     = V2 ? 0 : sizeof (EFI_GUID);
        List->SignatureListSize = sizeof (EFI_SIGNATURE_LIST) + List->SignatureSize;
        break;
      case 9:
        // V1 owner is truncated; V2 certificate entry exceeds the list.
        List->SignatureSize     = V2 ? mMockCertificateSize + 1 : sizeof (EFI_GUID) - 1;
        List->SignatureListSize = sizeof (EFI_SIGNATURE_LIST) + (V2 ? mMockCertificateSize : List->SignatureSize);
        break;
      case 10:
        Data.push_back (0);
        reinterpret_cast<EFI_SIGNATURE_LIST *>(Data.data ())->SignatureListSize++;
        break;
      case 11:
        List->SignatureHeaderSize = 1;
        break;
      case 12:
        Data = BuildMockCertificateList (V2, 0);
        break;
      default:
        // V2 X.509 permits one certificate, not two or three.
        Data = BuildMockCertificateList (true, Case - 11);
        break;
    }

    EXPECT_EQ (Check (Data), EFI_INVALID_PARAMETER);
  }
}

TEST_P (FullX509SignatureListTest, RejectsTruncatedTrailingList) {
  bool                V2   = std::get<1> (GetParam ());
  std::vector<UINT8>  Data = BuildMockCertificateList (V2, 1);

  Data.push_back (0);

  ExpectCertificates (Data.data (), 1, V2, true);
  EXPECT_EQ (Check (Data), EFI_INVALID_PARAMETER);
}

TEST_P (FullX509SignatureListTest, AcceptsEmptyDeletionWithoutCrypto) {
  std::vector<UINT8>  Data;

  EXPECT_CALL (BaseCryptLibMock, X509IsPublicKeySupported (_, _)).Times (0);
  EXPECT_EQ (Check (Data), EFI_SUCCESS);
}

TEST_P (FullX509SignatureListTest, ProcessVariableRejectsUnsupportedFirstWithoutUpdate) {
  std::vector<UINT8>  Data = BuildMockCertificateList (std::get<1> (GetParam ()), 1);

  CheckThroughProcessVariable (Data, 1, false);
}

TEST_P (FullX509SignatureListTest, ProcessVariableWritesSupportedCertificate) {
  std::vector<UINT8>  Data = BuildMockCertificateList (std::get<1> (GetParam ()), 1);

  CheckThroughProcessVariable (Data, 1, false, true);
}

TEST_P (FullX509SignatureListTest, ProcessVariableRejectsEmptyCertificateListWithoutUpdate) {
  std::vector<UINT8>  Data = BuildMockCertificateList (std::get<1> (GetParam ()), 0);

  CheckThroughProcessVariable (Data, 0, false);
}

TEST_P (FullX509SignatureListTest, ProcessVariableRejectsUnsupportedLaterV1OrMalformedV2WithoutUpdate) {
  bool                V2   = std::get<1> (GetParam ());
  std::vector<UINT8>  Data = BuildMockCertificateList (V2, 3);

  CheckThroughProcessVariable (Data, V2 ? 0 : 3, false);
}

TEST_P (FullX509SignatureListTest, ProcessVariableRejectsUnsupportedLaterListWithoutUpdate) {
  bool                V2         = std::get<1> (GetParam ());
  std::vector<UINT8>  Data       = BuildMockCertificateList (V2, 1);
  UINTN               LaterCount = V2 ? 2 : 1;
  std::vector<UINT8>  Later      = BuildMockCertificateList (!V2, LaterCount);

  Data.insert (Data.end (), Later.begin (), Later.end ());

  CheckThroughProcessVariable (Data, LaterCount, true);
}

TEST_P (FullX509SignatureListTest, ProcessVariableRejectsMalformedListWithoutUpdate) {
  std::vector<UINT8>  Data = BuildMockCertificateList (std::get<1> (GetParam ()), 1);

  reinterpret_cast<EFI_SIGNATURE_LIST *>(Data.data ())->SignatureSize = 0;

  CheckThroughProcessVariable (Data, 0, false);
}

INSTANTIATE_TEST_SUITE_P (
  PkKekDbDbxDbtV1V2,
  FullX509SignatureListTest,
  ::testing::Combine (
               ::testing::Values ((UINTN)0, (UINTN)1, (UINTN)2, (UINTN)3, (UINTN)4),
               ::testing::Bool ()
               )
  );

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
