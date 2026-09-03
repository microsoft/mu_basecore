/** @file
  Unit and integration tests for the top-level image verification flow.

  These tests exercise ValidateImage, DxeImageVerificationHandler, and
  DxeImageVerificationLibConstructor using synthetic signature databases,
  WIN_CERTIFICATE entries, and PE/COFF images.

  Copyright (c) 2025, Yandex. All rights reserved.
  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockBaseCryptLib.h>
#include <GoogleTest/Library/MockSecureBootVariableLib.h>
#include <GoogleTest/Library/MockSecurityManagementLib.h>
#include <GoogleTest/Library/MockTpmMeasurementLib.h>
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>
#include <GoogleTest/Library/MockUefiLib.h>

#include <vector>

extern "C" {
  #include <Uefi.h>
  #include <Guid/ImageAuthentication.h>
  #include <Guid/WinCertificate.h>
  #include <IndustryStandard/PeImage.h>
  #include <IndustryStandard/UefiTcgPlatform.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include "../DxeImageVerificationLib.h"

  EFI_STATUS
  ValidateImage (
    IN     CONST VOID             *AuthenticodeImage,
    IN     UINTN                  AuthenticodeImageSize,
    IN     CONST WIN_CERTIFICATE  *WinCertificates,
    IN     UINTN                  WinCertificatesLength,
    IN OUT MEASURED_AUTHORITIES   *Measured
    );
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

static constexpr UINT32  kDosHeaderSize    = sizeof (EFI_IMAGE_DOS_HEADER);
static constexpr UINT32  kPeCoffOffset     = kDosHeaderSize;
static constexpr UINT32  kSizeOfHeaders    = 0x200;
static constexpr UINT32  kSizeOfImage      = 0x400;
static constexpr UINTN   kSha256DigestSize = 32;

static EFI_DEVICE_PATH_PROTOCOL  mDevicePath;
static UINT8                     mAuthenticodeImage[16] = { 0 };

/**
  Build a single-list signature database containing one V1 entry.

  @param[in]  SignatureType  Signature type for the list.
  @param[in]  Payload        Entry payload placed after the SignatureOwner.

  @return  The complete EFI_SIGNATURE_LIST byte buffer.
**/
static std::vector<UINT8>
BuildSignatureDatabase (
  IN CONST EFI_GUID            &SignatureType,
  IN CONST std::vector<UINT8>  &Payload
  )
{
  UINT32              SignatureSize;
  UINT32              ListSize;
  std::vector<UINT8>  Database;
  EFI_SIGNATURE_LIST  *List;
  EFI_SIGNATURE_DATA  *Entry;

  SignatureSize = (UINT32)(sizeof (EFI_GUID) + Payload.size ());
  ListSize      = (UINT32)(sizeof (EFI_SIGNATURE_LIST) + SignatureSize);
  Database.resize (ListSize, 0);

  List = (EFI_SIGNATURE_LIST *)Database.data ();
  CopyGuid (&List->SignatureType, &SignatureType);
  List->SignatureListSize   = ListSize;
  List->SignatureHeaderSize = 0;
  List->SignatureSize       = SignatureSize;

  Entry = (EFI_SIGNATURE_DATA *)(Database.data () + sizeof (EFI_SIGNATURE_LIST));
  CopyMem (Entry->SignatureData, Payload.data (), Payload.size ());

  return Database;
}

/**
  Return a GetVariable2 mock action that allocates a copy of Payload.

  @param[in]  Payload  Variable contents returned by the action.

  @return  A GoogleMock action for GetVariable2.
**/
static auto
ReturnVariablePayload (
  IN CONST std::vector<UINT8>  &Payload
  )
{
  return Invoke (
           [Payload] (
                      IN CONST CHAR16    *Name,
                      IN CONST EFI_GUID  *Guid,
                      OUT      VOID      **Value,
                      OUT      UINTN     *Size
           ) -> EFI_STATUS {
    (VOID)Name;
    (VOID)Guid;

    *Value = AllocateCopyPool (Payload.size (), Payload.data ());
    if (*Value == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *Size = Payload.size ();
    return EFI_SUCCESS;
  }
           );
}

/**
  Return a HashAllByGuid mock action that emits Digest as a SHA-256 hash.

  @param[in]  Digest  Digest bytes returned by the action.

  @return  A GoogleMock action for HashAllByGuid.
**/
static auto
ReturnSha256Digest (
  IN CONST std::vector<UINT8>  &Digest
  )
{
  return Invoke (
           [Digest] (
                     IN  CONST EFI_GUID  *HashType,
                     IN  CONST VOID      *Buffer,
                     IN  UINTN           BufferSize,
                     OUT UINT8           *OutDigest,
                     OUT UINTN           *DigestSize
           ) -> EFI_STATUS {
    EXPECT_TRUE (CompareGuid (HashType, &gEfiHashAlgorithmSha256Guid));
    EXPECT_NE (Buffer, nullptr);
    EXPECT_GT (BufferSize, (UINTN)0);

    CopyMem (OutDigest, Digest.data (), Digest.size ());
    *DigestSize = Digest.size ();
    return EFI_SUCCESS;
  }
           );
}

/**
  Build a WIN_CERTIFICATE with Payload bytes following its fixed header.

  @param[in]  CertificateType  WIN_CERT_TYPE_* value.
  @param[in]  Payload          Certificate payload.

  @return  The complete WIN_CERTIFICATE byte buffer.
**/
static std::vector<UINT8>
BuildWinCertificate (
  IN UINT16                    CertificateType,
  IN CONST std::vector<UINT8>  &Payload
  )
{
  std::vector<UINT8>  Buffer (sizeof (WIN_CERTIFICATE) + Payload.size (), 0);
  WIN_CERTIFICATE     *Certificate;

  Certificate                   = (WIN_CERTIFICATE *)Buffer.data ();
  Certificate->dwLength         = (UINT32)Buffer.size ();
  Certificate->wRevision        = 0x0200;
  Certificate->wCertificateType = CertificateType;
  CopyMem (Buffer.data () + sizeof (WIN_CERTIFICATE), Payload.data (), Payload.size ());

  return Buffer;
}

/**
  Build a minimal valid unsigned PE32+ image.

  @return  A PE32+ image accepted by AuthenticodeLib and PeCoffLib.
**/
static std::vector<UINT8>
BuildPe32PlusImage (
  VOID
  )
{
  std::vector<UINT8>      Image (kSizeOfImage, 0);
  EFI_IMAGE_DOS_HEADER    *Dos;
  EFI_IMAGE_NT_HEADERS64  *Nt;

  Dos = (EFI_IMAGE_DOS_HEADER *)Image.data ();
  Nt  = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Dos->e_magic  = EFI_IMAGE_DOS_SIGNATURE;
  Dos->e_lfanew = kPeCoffOffset;

  Nt->Signature                          = EFI_IMAGE_NT_SIGNATURE;
  Nt->FileHeader.Machine                 = IMAGE_FILE_MACHINE_X64;
  Nt->FileHeader.NumberOfSections        = 1;
  Nt->FileHeader.SizeOfOptionalHeader    = sizeof (EFI_IMAGE_OPTIONAL_HEADER64);
  Nt->OptionalHeader.Magic               = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
  Nt->OptionalHeader.SizeOfImage         = kSizeOfImage;
  Nt->OptionalHeader.SizeOfHeaders       = kSizeOfHeaders;
  Nt->OptionalHeader.SectionAlignment    = 0x200;
  Nt->OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;

  return Image;
}

/**
  Release allocations owned by Measured and reset it to empty.

  @param[in,out]  Measured  Measurement state to release.
**/
static void
FreeMeasuredAuthorities (
  IN OUT MEASURED_AUTHORITIES  &Measured
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

class ValidateImageTest : public ::testing::Test {
protected:
  MockBaseCryptLib BaseCryptLibMock;
  MockTpmMeasurementLib TpmMeasurementLibMock;
  MockUefiLib UefiLibMock;
  MEASURED_AUTHORITIES Measured = { NULL, 0, 0 };

  void
  TearDown (
    ) override
  {
    FreeMeasuredAuthorities (Measured);
  }
};

TEST_F (ValidateImageTest, DatabaseLoadFailure_DeniesImage) {
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      NULL,
      0,
      &Measured
      ),
    EFI_ACCESS_DENIED
    );
}

TEST_F (ValidateImageTest, ImageHashInDbx_DeniesBeforeCertificateEvaluation) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xA5);
  std::vector<UINT8>  Db          = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  std::vector<UINT8>  Dbx         = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  std::vector<UINT8>  Certificate = BuildWinCertificate (
                                      WIN_CERT_TYPE_PKCS_SIGNED_DATA,
                                      std::vector<UINT8>(16, 0x5A)
                                      );

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (ReturnVariablePayload (Dbx));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (Digest));
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _)).Times (0);
  EXPECT_CALL (TpmMeasurementLibMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      (CONST WIN_CERTIFICATE *)Certificate.data (),
      Certificate.size (),
      &Measured
      ),
    EFI_ACCESS_DENIED
    );
}

TEST_F (ValidateImageTest, UnsignedImageHashInDb_AuthorizesImage) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0x3C);
  std::vector<UINT8>  Db = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (Digest));
  EXPECT_CALL (TpmMeasurementLibMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      NULL,
      0,
      &Measured
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Measured.Count, (UINTN)0);
}

TEST_F (ValidateImageTest, MalformedCertificateTable_DbHashStillAuthorizesImage) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0x42);
  std::vector<UINT8>  Db               = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  std::vector<UINT8>  CertificateTable = BuildWinCertificate (
                                           WIN_CERT_TYPE_EFI_PKCS115,
                                           std::vector<UINT8>(8, 0x19)
                                           );

  CertificateTable.push_back (0);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (Digest));

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      (CONST WIN_CERTIFICATE *)CertificateTable.data (),
      CertificateTable.size (),
      &Measured
      ),
    EFI_SUCCESS
    );
}

TEST_F (ValidateImageTest, UnusableCertificate_DbHashFallbackAuthorizesImage) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0x24);
  std::vector<UINT8>  Db          = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  std::vector<UINT8>  Certificate = BuildWinCertificate (
                                      WIN_CERT_TYPE_EFI_PKCS115,
                                      std::vector<UINT8>(8, 0x9A)
                                      );

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (Digest));

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      (CONST WIN_CERTIFICATE *)Certificate.data (),
      Certificate.size (),
      &Measured
      ),
    EFI_SUCCESS
    );
}

TEST_F (ValidateImageTest, CertificateHashFailure_DbHashRetryAuthorizesImage) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0x81);
  std::vector<UINT8>  Db          = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  std::vector<UINT8>  Certificate = BuildWinCertificate (
                                      WIN_CERT_TYPE_PKCS_SIGNED_DATA,
                                      std::vector<UINT8>(16, 0x36)
                                      );

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *HashType) -> EFI_STATUS {
    CopyGuid (HashType, &gEfiHashAlgorithmSha256Guid);
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR))
    .WillOnce (ReturnSha256Digest (Digest));
  EXPECT_CALL (TpmMeasurementLibMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      (CONST WIN_CERTIFICATE *)Certificate.data (),
      Certificate.size (),
      &Measured
      ),
    EFI_SUCCESS
    );
}

TEST_F (ValidateImageTest, CertificateAuthority_AuthorizesAndMeasuresImage) {
  std::vector<UINT8>  ImageDigest (kSha256DigestSize, 0x55);
  std::vector<UINT8>  Anchor (16, 0xA7);
  std::vector<UINT8>  Db          = BuildSignatureDatabase (gEfiCertX509Guid, Anchor);
  std::vector<UINT8>  Certificate = BuildWinCertificate (
                                      WIN_CERT_TYPE_PKCS_SIGNED_DATA,
                                      std::vector<UINT8>(16, 0x6B)
                                      );

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *HashType) -> EFI_STATUS {
    CopyGuid (HashType, &gEfiHashAlgorithmSha256Guid);
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (ImageDigest));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [Anchor] (
                   CONST UINT8 *,
                   UINTN,
                   CONST UINT8 *TrustedCert,
                   UINTN TrustedCertSize,
                   CONST UINT8 *,
                   UINTN,
                   UINT8 **CertChain,
                   UINTN *CertChainSize
         ) -> EFI_STATUS {
    EXPECT_EQ (TrustedCertSize, Anchor.size ());
    EXPECT_EQ (CompareMem (TrustedCert, Anchor.data (), Anchor.size ()), 0);
    *CertChain     = NULL;
    *CertChainSize = 0;
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (
    TpmMeasurementLibMock,
    TpmMeasureAndLogData ((UINT32)7, (UINT32)EV_EFI_VARIABLE_AUTHORITY, _, _, _, _)
    )
    .WillOnce (Return (EFI_SUCCESS));

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      (CONST WIN_CERTIFICATE *)Certificate.data (),
      Certificate.size (),
      &Measured
      ),
    EFI_SUCCESS
    );
  ASSERT_EQ (Measured.Count, (UINTN)1);
  ASSERT_NE (Measured.List, nullptr);
  ASSERT_NE (Measured.List[0].Data, nullptr);
  ASSERT_EQ (Measured.List[0].Size, sizeof (EFI_GUID) + Anchor.size ());
  EXPECT_EQ (
    CompareMem (
      ((EFI_SIGNATURE_DATA *)Measured.List[0].Data)->SignatureData,
      Anchor.data (),
      Anchor.size ()
      ),
    0
    );
}

TEST_F (ValidateImageTest, NoDbAuthorization_DeniesImage) {
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_NOT_FOUND))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      NULL,
      0,
      &Measured
      ),
    EFI_ACCESS_DENIED
    );
}

class DxeImageVerificationHandlerTest : public ::testing::Test {
protected:
  MockBaseCryptLib BaseCryptLibMock;
  MockSecureBootVariableLib SecureBootVariableLibMock;
  MockUefiBootServicesTableLib UefiBootServicesTableLibMock;
  MockUefiLib UefiLibMock;
};

TEST_F (DxeImageVerificationHandlerTest, NullFile_ReturnsInvalidParameter) {
  EXPECT_EQ (
    DxeImageVerificationHandler (0, NULL, mAuthenticodeImage, sizeof (mAuthenticodeImage), FALSE),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (DxeImageVerificationHandlerTest, FirmwareVolumeImage_BypassesVerification) {
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillOnce (Return (EFI_SUCCESS));
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_OpenProtocol (_, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ()).Times (0);
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (
      0,
      &mDevicePath,
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      FALSE
      ),
    EFI_SUCCESS
    );
}

TEST_F (DxeImageVerificationHandlerTest, SecureBootDisabled_BypassesVerification) {
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ())
    .WillOnce (Return (FALSE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (
      0,
      &mDevicePath,
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      FALSE
      ),
    EFI_SUCCESS
    );
}

TEST_F (DxeImageVerificationHandlerTest, MalformedImage_FailsClosed) {
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ())
    .WillOnce (Return (TRUE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (
      0,
      &mDevicePath,
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      FALSE
      ),
    EFI_ACCESS_DENIED
    );
}

TEST_F (DxeImageVerificationHandlerTest, ValidUnsignedImageHashInDb_AuthorizesImage) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xC3);
  std::vector<UINT8>  Db    = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  std::vector<UINT8>  Image = BuildPe32PlusImage ();

  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ())
    .WillOnce (Return (TRUE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (Digest));

  EXPECT_EQ (
    DxeImageVerificationHandler (
      0,
      &mDevicePath,
      Image.data (),
      Image.size (),
      FALSE
      ),
    EFI_SUCCESS
    );
}

TEST (DxeImageVerificationLibConstructorTest, RegistersSecurity2HandlerAndReturnsStatus) {
  MockSecurityManagementLib              SecurityManagementLibMock;
  SECURITY2_FILE_AUTHENTICATION_HANDLER  RegisteredHandler;

  RegisteredHandler = NULL;
  EXPECT_CALL (
    SecurityManagementLibMock,
    RegisterSecurity2Handler (
      _,
      (UINT32)(EFI_AUTH_OPERATION_VERIFY_IMAGE | EFI_AUTH_OPERATION_IMAGE_REQUIRED)
      )
    )
    .WillOnce (
       Invoke (
         [&RegisteredHandler] (
                               SECURITY2_FILE_AUTHENTICATION_HANDLER Handler,
                               UINT32 AuthenticationOperation
         ) -> EFI_STATUS {
    (VOID)AuthenticationOperation;
    RegisteredHandler = Handler;
    return EFI_DEVICE_ERROR;
  }
         )
       );

  EXPECT_EQ (DxeImageVerificationLibConstructor (NULL, NULL), EFI_DEVICE_ERROR);
  EXPECT_TRUE (RegisteredHandler == DxeImageVerificationHandler);
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
