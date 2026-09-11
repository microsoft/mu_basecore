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
  CONST UINTN                         SigDataSize = 22;
  std::vector<UINT8>                  Data (OFFSET_OF_AUTHINFO2_CERT_DATA + SigDataSize + 1, 0);
  EFI_VARIABLE_AUTHENTICATION_2       *Authentication;

  Authentication                                      = reinterpret_cast<EFI_VARIABLE_AUTHENTICATION_2 *>(Data.data ());
  Authentication->AuthInfo.Hdr.dwLength                = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData) + SigDataSize;
  Authentication->AuthInfo.Hdr.wCertificateType        = WIN_CERT_TYPE_EFI_GUID;
  Authentication->AuthInfo.CertType                    = gEfiCertPkcs7Guid;
  Authentication->AuthInfo.CertData[1]                 = TWO_BYTE_ENCODE;
  CopyMem (Authentication->AuthInfo.CertData + 13, mSha256Oid, sizeof (mSha256Oid));

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
  MockBaseCryptLib  BaseCryptLibMock;
  AUTH_VAR_LIB_CONTEXT_IN  AuthVarContext;
  EFI_GUID          VendorGuid = { 0 };
  UINT8             *Payload;
  UINTN             PayloadSize;

  VOID
  SetUp (
    ) override
  {
    ZeroMem (&AuthVarContext, sizeof (AuthVarContext));
    AuthVarContext.FindVariable     = FindVariableNotFound;
    AuthVarContext.GetScratchBuffer = GetScratchBufferOutOfResources;
    mAuthVarLibContextIn             = &AuthVarContext;
    Payload     = NULL;
    PayloadSize = 0;
  }

  VOID
  TearDown (
    ) override
  {
    mAuthVarLibContextIn = NULL;
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

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}