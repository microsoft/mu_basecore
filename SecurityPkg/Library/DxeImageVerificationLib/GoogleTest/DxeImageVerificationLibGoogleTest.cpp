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
#include <GoogleTest/Library/MockDevicePathLib.h>

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
    IN     CONST VOID               *AuthenticodeImage,
    IN     UINTN                    AuthenticodeImageSize,
    IN     CONST UINT8              *WinCertificates,
    IN     UINTN                    WinCertificatesLength,
    IN OUT MEASURED_AUTHORITIES     *Measured,
    IN OUT IMAGE_VERIFICATION_INFO  *Info
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

static EFI_DEVICE_PATH_PROTOCOL  mDevicePath = {
  END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
};
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
  IMAGE_VERIFICATION_INFO Info  = { };

  void
  SetUp (
    ) override
  {
    ASSERT_EQ (
      ImageVerificationResultCreateImage (NULL, &mDevicePath, sizeof (mDevicePath), &Info.Image),
      EFI_SUCCESS
      );
  }

  void
  TearDown (
    ) override
  {
    FreeMeasuredAuthorities (Measured);
    if (Info.Image != NULL) {
      FreePool (Info.Image);
    }
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
      &Measured,
      &Info
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
      Certificate.data (),
      Certificate.size (),
      &Measured,
      &Info
      ),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Info.Status, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_REJECTED_BY_DIGEST);
  EXPECT_TRUE (CompareGuid (&Info.DigestAlgorithm, &gEfiHashAlgorithmSha256Guid));
  EXPECT_EQ (((EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Info.Image)->NumberOfSignatures, (UINT32)0);
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
      &Measured,
      &Info
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Measured.Count, (UINTN)0);
  EXPECT_EQ (Info.Status, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_DIGEST);
  EXPECT_TRUE (CompareGuid (&Info.DigestAlgorithm, &gEfiHashAlgorithmSha256Guid));
}

TEST_F (ValidateImageTest, MalformedCertificateTable_DbHashAuthorizesBeforeCertificateEvaluation) {
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
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      CertificateTable.data (),
      CertificateTable.size (),
      &Measured,
      &Info
      ),
    EFI_SUCCESS
    );
}

TEST_F (ValidateImageTest, UnusableCertificate_DbHashAuthorizesBeforeCertificateEvaluation) {
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
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      Certificate.data (),
      Certificate.size (),
      &Measured,
      &Info
      ),
    EFI_SUCCESS
    );
}

TEST_F (ValidateImageTest, DbHashFailure_SignatureWithoutTrustAnchorDeniesImage) {
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

  //
  // The db hash lookup fails before signature evaluation. Signature evaluation can recompute the
  // image hash, but this db contains no certificate trust anchor, so the signature cannot authorize.
  //
  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      Certificate.data (),
      Certificate.size (),
      &Measured,
      &Info
      ),
    EFI_ACCESS_DENIED
    );
}

TEST_F (ValidateImageTest, UnalignedCertificateTablePassesRawSignatureToVerifier) {
  std::vector<UINT8>  Certificate = BuildWinCertificate (
                                      WIN_CERT_TYPE_PKCS_SIGNED_DATA,
                                      std::vector<UINT8>(16, 0x36)
                                      );
  std::vector<UINT8>  Storage (Certificate.size () + 1);
  const UINTN         Offset = ((UINTN)Storage.data () % ALIGNOF (WIN_CERTIFICATE) == 0) ? 1 : 0;
  CONST UINT8         *Table = Storage.data () + Offset;

  ASSERT_NE ((UINTN)Table % ALIGNOF (WIN_CERTIFICATE), (UINTN)0);
  CopyMem (Storage.data () + Offset, Certificate.data (), Certificate.size ());

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_NOT_FOUND))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (
       Invoke (
         [Table] (CONST UINT8 *Data, UINTN DataSize, EFI_GUID *) -> EFI_STATUS {
    EXPECT_EQ (Data, Table + sizeof (WIN_CERTIFICATE));
    EXPECT_EQ (DataSize, (UINTN)16);
    return EFI_UNSUPPORTED;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _)).Times (0);
  EXPECT_CALL (TpmMeasurementLibMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      Table,
      Certificate.size (),
      &Measured,
      &Info
      ),
    EFI_ACCESS_DENIED
    );
}

TEST_F (ValidateImageTest, AuthorityConstructionFailure_DiscardsRecordAndDeniesImage) {
  std::vector<UINT8>  ImageDigest (kSha256DigestSize, 0x55);
  std::vector<UINT8>  Db = BuildSignatureDatabase (
                             gEfiCertX509Sha256Guid,
                             std::vector<UINT8>(kSha256DigestSize + sizeof (EFI_TIME), 0xA7)
                             );
  std::vector<UINT8>  Certificate = BuildWinCertificate (
                                      WIN_CERT_TYPE_PKCS_SIGNED_DATA,
                                      std::vector<UINT8>(8, 0x6B)
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
  EXPECT_CALL (BaseCryptLibMock, GetTrustAnchorX509FromAuthData (_, _, _, _, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (VOID **, CONST EFI_GUID *HashType, CONST UINT8 *, UINTN HashSize, CONST UINT8 *, UINTN, UINT8 **Anchor, UINTN *AnchorSize) -> EFI_STATUS {
    EXPECT_TRUE (CompareGuid (HashType, &gEfiHashAlgorithmSha256Guid));
    EXPECT_EQ (HashSize, kSha256DigestSize);
    *Anchor     = (UINT8 *)AllocateZeroPool (1);
    *AnchorSize = MAX_UINTN;
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));
  EXPECT_CALL (TpmMeasurementLibMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      Certificate.data (),
      Certificate.size (),
      &Measured,
      &Info
      ),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Info.Image, nullptr);
  EXPECT_EQ (Info.Status, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY);
  EXPECT_EQ (Measured.Count, (UINTN)0);
}

TEST_F (ValidateImageTest, SignatureEvaluationFailure_AnotherSignatureCanAuthorize) {
  std::vector<UINT8>  ImageDigest (kSha256DigestSize, 0x55);
  std::vector<UINT8>  Db           = BuildSignatureDatabase (gEfiCertX509Guid, std::vector<UINT8>(16, 0xA7));
  std::vector<UINT8>  Certificates = BuildWinCertificate (
                                       WIN_CERT_TYPE_PKCS_SIGNED_DATA,
                                       std::vector<UINT8>(8, 0x6B)
                                       );
  auto  Second = BuildWinCertificate (
                   WIN_CERT_TYPE_PKCS_SIGNED_DATA,
                   std::vector<UINT8>(8, 0x7C)
                   );

  Certificates.insert (Certificates.end (), Second.begin (), Second.end ());
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .Times (2)
    .WillRepeatedly (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *HashType) -> EFI_STATUS {
    CopyGuid (HashType, &gEfiHashAlgorithmSha256Guid);
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR))
    .WillOnce (ReturnSha256Digest (ImageDigest));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (Return (FALSE));
  EXPECT_CALL (TpmMeasurementLibMock, TpmMeasureAndLogData ((UINT32)7, (UINT32)EV_EFI_VARIABLE_AUTHORITY, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      Certificates.data (),
      Certificates.size (),
      &Measured,
      &Info
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Info.Image, nullptr);
  EXPECT_EQ (Info.Status, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY);
  EXPECT_EQ (Measured.Count, (UINTN)1);
}

TEST_F (ValidateImageTest, SignatureEvaluationFailure_OptionalReportMayBeNull) {
  auto  Certificate = BuildWinCertificate (WIN_CERT_TYPE_PKCS_SIGNED_DATA, std::vector<UINT8>(8, 0x6B));

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillRepeatedly (Return (EFI_NOT_FOUND));
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
    .WillOnce (Return (EFI_DEVICE_ERROR));
  EXPECT_CALL (TpmMeasurementLibMock, TpmMeasureAndLogData (_, _, _, _, _, _)).Times (0);

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      Certificate.data (),
      Certificate.size (),
      &Measured,
      NULL
      ),
    EFI_ACCESS_DENIED
    );
  EXPECT_NE (Info.Image, nullptr);
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
    .WillOnce (ReturnSha256Digest (ImageDigest))
    .WillOnce (ReturnSha256Digest (std::vector<UINT8> (kSha256DigestSize, 0x7D)));
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *Cert, UINTN CertSize, UINT8 **Tbs, UINTN *TbsSize) -> BOOLEAN {
    *Tbs     = (UINT8 *)Cert;
    *TbsSize = CertSize;
    return TRUE;
  }
         )
       );
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
      Certificate.data (),
      Certificate.size (),
      &Measured,
      &Info
      ),
    EFI_SUCCESS
    );
  ASSERT_EQ (Measured.Count, (UINTN)1);
  EXPECT_EQ (Info.Status, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY);
  EXPECT_TRUE (IsZeroGuid (&Info.DigestAlgorithm));
  auto  ImageRecord = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Info.Image;

  ASSERT_EQ (ImageRecord->NumberOfSignatures, (UINT32)1);
  auto  Signature = (EFI_SIGNATURE_VERIFICATION_RESULT *)(
                                                          (UINT8 *)ImageRecord + ALIGN_VALUE (sizeof (*ImageRecord) + ImageRecord->NameLength + ImageRecord->DevicePathLength, 8)
                                                          );

  EXPECT_EQ (Signature->Status, (UINT32)EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY);
  EXPECT_TRUE (CompareGuid (&Signature->ThumbprintAlgorithm, &gEfiHashAlgorithmSha256Guid));
  ASSERT_EQ (Signature->ThumbprintSize, kSha256DigestSize);
  EXPECT_EQ (((UINT8 *)(Signature + 1))[0], 0x7D);
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
      &Measured,
      &Info
      ),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Info.Status, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY);
  EXPECT_TRUE (IsZeroGuid (&Info.DigestAlgorithm));
}

TEST_F (ValidateImageTest, EvaluatedSignatures_RecordOriginalIndicesAndStatuses) {
  auto                First        = BuildWinCertificate (WIN_CERT_TYPE_EFI_PKCS115, std::vector<UINT8> (8, 0x11));
  auto                Second       = BuildWinCertificate (WIN_CERT_TYPE_PKCS_SIGNED_DATA, std::vector<UINT8> (8, 0x22));
  auto                Third        = BuildWinCertificate (WIN_CERT_TYPE_PKCS_SIGNED_DATA, std::vector<UINT8> (8, 0x33));
  std::vector<UINT8>  Certificates = First;

  Certificates.insert (Certificates.end (), Second.begin (), Second.end ());
  Certificates.insert (Certificates.end (), Third.begin (), Third.end ());
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillRepeatedly (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHashAlgorithm (_, _, _))
    .WillOnce (Return (EFI_UNSUPPORTED))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *, UINTN, EFI_GUID *Algorithm) -> EFI_STATUS {
    CopyGuid (Algorithm, &gEfiHashAlgorithmSha256Guid);
    return EFI_SUCCESS;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (std::vector<UINT8> (kSha256DigestSize, 0x44)));

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      Certificates.data (),
      Certificates.size (),
      &Measured,
      &Info
      ),
    EFI_ACCESS_DENIED
    );
  auto  Image = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Info.Image;

  ASSERT_EQ (Image->NumberOfSignatures, (UINT32)3);
  auto  Signature = (EFI_SIGNATURE_VERIFICATION_RESULT *)(
                                                          (UINT8 *)Image + ALIGN_VALUE (sizeof (*Image) + Image->NameLength + Image->DevicePathLength, 8)
                                                          );

  for (UINT32 Index = 0; Index < 3; Index++) {
    EXPECT_EQ (Signature->SignatureIndex, Index);
    EXPECT_EQ (
      Signature->Status,
      (UINT32)((Index < 2) ? EFI_SIGNATURE_VERIFICATION_MALFORMED : EFI_SIGNATURE_VERIFICATION_REJECTED_NO_AUTHORITY)
      );
    EXPECT_EQ (Signature->ThumbprintSize, (UINT32)0);
    EXPECT_TRUE (IsZeroGuid (&Signature->ThumbprintAlgorithm));
    Signature = (EFI_SIGNATURE_VERIFICATION_RESULT *)((UINT8 *)Signature + Signature->Length);
  }
}

TEST_F (ValidateImageTest, MalformedCertificateTail_RecordsMalformedSignature) {
  std::vector<UINT8>  Certificates (1, 0);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillRepeatedly (Return (EFI_NOT_FOUND));

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      Certificates.data (),
      Certificates.size (),
      &Measured,
      &Info
      ),
    EFI_ACCESS_DENIED
    );
  auto  Image = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Info.Image;

  ASSERT_EQ (Image->NumberOfSignatures, (UINT32)1);
  auto  Signature = (EFI_SIGNATURE_VERIFICATION_RESULT *)(
                                                          (UINT8 *)Image + ALIGN_VALUE (sizeof (*Image) + Image->NameLength + Image->DevicePathLength, 8)
                                                          );

  EXPECT_EQ (Signature->SignatureIndex, (UINT32)0);
  EXPECT_EQ (Signature->Status, (UINT32)EFI_SIGNATURE_VERIFICATION_MALFORMED);
}

TEST_F (ValidateImageTest, SignatureReportingFailure_DiscardsIncompleteRecord) {
  auto  Certificate = BuildWinCertificate (WIN_CERT_TYPE_EFI_PKCS115, std::vector<UINT8> (8, 0x11));

  ((EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Info.Image)->Length = MAX_UINT32;
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillRepeatedly (Return (EFI_NOT_FOUND));

  EXPECT_EQ (
    ValidateImage (
      mAuthenticodeImage,
      sizeof (mAuthenticodeImage),
      Certificate.data (),
      Certificate.size (),
      &Measured,
      &Info
      ),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Info.Image, nullptr);
  EXPECT_EQ (Info.Status, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY);
}

TEST_F (ValidateImageTest, MalformedDbx_DoesNotClaimDigestMatch) {
  std::vector<UINT8>  Dbx (1, 0);

  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (Return (EFI_NOT_FOUND))
    .WillOnce (ReturnVariablePayload (Dbx));

  EXPECT_EQ (
    ValidateImage (mAuthenticodeImage, sizeof (mAuthenticodeImage), NULL, 0, &Measured, &Info),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Info.Status, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY);
  EXPECT_TRUE (IsZeroGuid (&Info.DigestAlgorithm));
}

class DxeImageVerificationHandlerTest : public ::testing::Test {
protected:
  MockBaseCryptLib BaseCryptLibMock;
  MockSecureBootVariableLib SecureBootVariableLibMock;
  MockUefiBootServicesTableLib UefiBootServicesTableLibMock;
  MockUefiLib UefiLibMock;
  MockDevicePathLib DevicePathLibMock;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE *Table = NULL;

  void
  SetUp (
    ) override
  {
    ON_CALL (DevicePathLibMock, IsDevicePathEnd (_))
      .WillByDefault (Return (TRUE));
    ON_CALL (DevicePathLibMock, GetDevicePathSize (_))
      .WillByDefault (Return (sizeof (mDevicePath)));
    ON_CALL (UefiLibMock, EfiGetSystemConfigurationTable (_, _))
      .WillByDefault (
         Invoke (
           [this] (EFI_GUID *Guid, VOID **Out) -> EFI_STATUS {
      EXPECT_TRUE (CompareGuid (Guid, &gEfiImageSecureBootVerificationResultTableGuid));
      *Out = Table;
      return (Table == NULL) ? EFI_NOT_FOUND : EFI_SUCCESS;
    }
           )
         );
    ON_CALL (UefiBootServicesTableLibMock, gBS_InstallConfigurationTable (_, _))
      .WillByDefault (
         Invoke (
           [this] (EFI_GUID *Guid, VOID *NewTable) -> EFI_STATUS {
      EXPECT_TRUE (CompareGuid (Guid, &gEfiImageSecureBootVerificationResultTableGuid));
      Table = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE *)NewTable;
      return EFI_SUCCESS;
    }
           )
         );
  }

  void
  TearDown (
    ) override
  {
    if (Table != NULL) {
      FreePool (Table);
    }

    FreeMeasuredAuthorities (*GetMeasuredAuthorities ());
  }

  void
  ExpectNoPathLookups (
    )
  {
    EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _)).Times (0);
    EXPECT_CALL (UefiBootServicesTableLibMock, gBS_OpenProtocol (_, _, _, _, _, _)).Times (0);
    EXPECT_CALL (DevicePathLibMock, ConvertDevicePathToText (_, _, _)).Times (0);
    EXPECT_CALL (DevicePathLibMock, GetDevicePathSize (_)).Times (0);
  }

  void
  ExpectUnknownPathRecord (
    UINT32          Status,
    UINT32          SignatureCount,
    CONST EFI_GUID  *DigestAlgorithm
    )
  {
    IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;

    ASSERT_NE (Table, nullptr);
    EXPECT_EQ (Table->NumberOfImages, (UINT32)1);
    ASSERT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table));
    auto  Result = ImageVerificationResultIteratorNextImage (&Iter);

    ASSERT_NE (Result, nullptr);
    EXPECT_EQ (Result->ImageStatus, Status);
    EXPECT_EQ (Result->NumberOfSignatures, SignatureCount);
    EXPECT_EQ (Result->NameLength, (UINT32)0);
    ASSERT_EQ (Result->DevicePathLength, sizeof (mDevicePath));
    EXPECT_EQ (CompareMem (Result + 1, &mDevicePath, sizeof (mDevicePath)), 0);
    if (DigestAlgorithm == NULL) {
      EXPECT_TRUE (IsZeroGuid (&Result->ImageDigestAlgorithm));
    } else {
      EXPECT_TRUE (CompareGuid (&Result->ImageDigestAlgorithm, DigestAlgorithm));
    }

    EXPECT_EQ (ImageVerificationResultIteratorNextImage (&Iter), nullptr);
  }
};

TEST_F (DxeImageVerificationHandlerTest, NullFileAndBuffer_ReturnsInvalidParameter) {
  ExpectNoPathLookups ();
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ()).Times (0);
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_InstallConfigurationTable (_, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (0, NULL, NULL, 0, FALSE),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (DxeImageVerificationHandlerTest, NullFile_SecureBootDisabled_BypassesVerification) {
  std::vector<UINT8>  Image = BuildPe32PlusImage ();

  ExpectNoPathLookups ();
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ()).WillOnce (Return (FALSE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _)).Times (0);
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_InstallConfigurationTable (_, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (0, NULL, Image.data (), Image.size (), FALSE),
    EFI_SUCCESS
    );
  EXPECT_EQ (Table, nullptr);
}

TEST_F (DxeImageVerificationHandlerTest, NullFile_UnenrolledImage_IsDeniedAndReported) {
  std::vector<UINT8>  Image = BuildPe32PlusImage ();

  ExpectNoPathLookups ();
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ()).WillOnce (Return (TRUE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .Times (2)
    .WillRepeatedly (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (0, NULL, Image.data (), Image.size (), FALSE),
    EFI_ACCESS_DENIED
    );
  ExpectUnknownPathRecord (EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY, 0, NULL);
}

TEST_F (DxeImageVerificationHandlerTest, NullFile_AllowedDigest_AuthorizesAndReportsImage) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xC3);
  std::vector<UINT8>  Db    = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  std::vector<UINT8>  Image = BuildPe32PlusImage ();

  ExpectNoPathLookups ();
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ()).WillOnce (Return (TRUE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (Digest));

  EXPECT_EQ (
    DxeImageVerificationHandler (0, NULL, Image.data (), Image.size (), FALSE),
    EFI_SUCCESS
    );
  ExpectUnknownPathRecord (EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_DIGEST, 0, &gEfiHashAlgorithmSha256Guid);
}

TEST_F (DxeImageVerificationHandlerTest, NullFile_RevokedDigest_DeniesEvenWhenAllowed) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xC3);
  std::vector<UINT8>  Db    = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  std::vector<UINT8>  Image = BuildPe32PlusImage ();

  ExpectNoPathLookups ();
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ()).WillOnce (Return (TRUE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (ReturnVariablePayload (Db));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (Digest));

  EXPECT_EQ (
    DxeImageVerificationHandler (0, NULL, Image.data (), Image.size (), FALSE),
    EFI_ACCESS_DENIED
    );
  ExpectUnknownPathRecord (EFI_IMAGE_VERIFICATION_STATUS_REJECTED_BY_DIGEST, 0, &gEfiHashAlgorithmSha256Guid);
}

TEST_F (DxeImageVerificationHandlerTest, NullFile_AllowedCertificate_AuthorizesAndReportsSignature) {
  MockTpmMeasurementLib  TpmMeasurementLibMock;
  std::vector<UINT8>     ImageDigest (kSha256DigestSize, 0x55);
  std::vector<UINT8>     Anchor (16, 0xAB);
  std::vector<UINT8>     Db          = BuildSignatureDatabase (gEfiCertX509Guid, Anchor);
  std::vector<UINT8>     Certificate = BuildWinCertificate (
                                         WIN_CERT_TYPE_PKCS_SIGNED_DATA,
                                         std::vector<UINT8>(8, 0x6B)
                                         );
  std::vector<UINT8>  Image = BuildPe32PlusImage ();

  Image.insert (Image.end (), Certificate.begin (), Certificate.end ());
  auto  Nt = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = kSizeOfImage;
  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size           = (UINT32)Certificate.size ();

  ExpectNoPathLookups ();
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ()).WillOnce (Return (TRUE));
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
    .WillOnce (ReturnSha256Digest (ImageDigest))
    .WillOnce (ReturnSha256Digest (std::vector<UINT8>(kSha256DigestSize, 0x7D)));
  EXPECT_CALL (BaseCryptLibMock, AuthenticodeVerifyEx (_, _, _, _, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));
  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (
       Invoke (
         [] (CONST UINT8 *Cert, UINTN CertSize, UINT8 **Tbs, UINTN *TbsSize) -> BOOLEAN {
    *Tbs     = (UINT8 *)Cert;
    *TbsSize = CertSize;
    return TRUE;
  }
         )
       );
  EXPECT_CALL (TpmMeasurementLibMock, TpmMeasureAndLogData ((UINT32)7, (UINT32)EV_EFI_VARIABLE_AUTHORITY, _, _, _, _))
    .WillOnce (Return (EFI_SUCCESS));

  EXPECT_EQ (
    DxeImageVerificationHandler (0, NULL, Image.data (), Image.size (), FALSE),
    EFI_SUCCESS
    );
  ExpectUnknownPathRecord (EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY, 1, NULL);

  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;

  ASSERT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table));
  ASSERT_NE (ImageVerificationResultIteratorNextImage (&Iter), nullptr);
  auto  Signature = ImageVerificationResultIteratorNextSignature (&Iter);

  ASSERT_NE (Signature, nullptr);
  EXPECT_EQ (Signature->Status, (UINT32)EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY);
  EXPECT_TRUE (CompareGuid (&Signature->ThumbprintAlgorithm, &gEfiHashAlgorithmSha256Guid));
  ASSERT_EQ (Signature->ThumbprintSize, kSha256DigestSize);
  EXPECT_EQ (((CONST UINT8 *)(Signature + 1))[0], 0x7D);
}

TEST_F (DxeImageVerificationHandlerTest, NullFile_MalformedImage_IsDeniedAndReported) {
  ExpectNoPathLookups ();
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ()).WillOnce (Return (TRUE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (0, NULL, mAuthenticodeImage, sizeof (mAuthenticodeImage), FALSE),
    EFI_ACCESS_DENIED
    );
  ExpectUnknownPathRecord (EFI_IMAGE_VERIFICATION_STATUS_REJECTED_MALFORMED, 0, NULL);
}

TEST_F (DxeImageVerificationHandlerTest, NullFile_EmptyBuffer_IsDeniedAndReported) {
  ExpectNoPathLookups ();
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ()).WillOnce (Return (TRUE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (0, NULL, mAuthenticodeImage, 0, FALSE),
    EFI_ACCESS_DENIED
    );
  ExpectUnknownPathRecord (EFI_IMAGE_VERIFICATION_STATUS_REJECTED_MALFORMED, 0, NULL);
}

TEST_F (DxeImageVerificationHandlerTest, FirmwareVolumeImage_BypassesVerification) {
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_InstallConfigurationTable (_, _)).Times (0);
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
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_InstallConfigurationTable (_, _)).Times (0);
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
  ASSERT_NE (Table, nullptr);
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;

  ASSERT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table));
  auto  Result = ImageVerificationResultIteratorNextImage (&Iter);

  ASSERT_NE (Result, nullptr);
  EXPECT_EQ (Result->ImageStatus, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_REJECTED_MALFORMED);
  EXPECT_TRUE (IsZeroGuid (&Result->ImageDigestAlgorithm));
  EXPECT_EQ (Result->NumberOfSignatures, (UINT32)0);
}

TEST_F (DxeImageVerificationHandlerTest, ValidUnsignedImageHashInDb_AuthorizesImage) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xC3);
  std::vector<UINT8>  Db    = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  std::vector<UINT8>  Image = BuildPe32PlusImage ();

  EXPECT_CALL (DevicePathLibMock, ConvertDevicePathToText (&mDevicePath, FALSE, TRUE))
    .WillOnce (Return (nullptr));
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
  ASSERT_NE (Table, nullptr);
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;

  ASSERT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table));
  auto  Result = ImageVerificationResultIteratorNextImage (&Iter);

  ASSERT_NE (Result, nullptr);
  EXPECT_EQ (Result->ImageStatus, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_DIGEST);
  EXPECT_TRUE (CompareGuid (&Result->ImageDigestAlgorithm, &gEfiHashAlgorithmSha256Guid));
  EXPECT_EQ (Result->NameLength, (UINT32)0);
  EXPECT_EQ (Result->DevicePathLength, sizeof (mDevicePath));
  EXPECT_EQ (CompareMem (Result + 1, &mDevicePath, sizeof (mDevicePath)), 0);
}

TEST_F (DxeImageVerificationHandlerTest, RepeatedVerification_AppendsToExistingTable) {
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillRepeatedly (Return (EFI_NOT_FOUND));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ())
    .WillRepeatedly (Return (TRUE));

  for (UINTN Index = 0; Index < 2; Index++) {
    EXPECT_EQ (
      DxeImageVerificationHandler (0, &mDevicePath, mAuthenticodeImage, sizeof (mAuthenticodeImage), FALSE),
      EFI_ACCESS_DENIED
      );
  }

  ASSERT_NE (Table, nullptr);
  EXPECT_EQ (Table->NumberOfImages, (UINT32)2);
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;

  ASSERT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table));
  EXPECT_NE (ImageVerificationResultIteratorNextImage (&Iter), nullptr);
  EXPECT_NE (ImageVerificationResultIteratorNextImage (&Iter), nullptr);
  EXPECT_EQ (ImageVerificationResultIteratorNextImage (&Iter), nullptr);
}

TEST_F (DxeImageVerificationHandlerTest, PublishFailure_PreservesExistingTableAndDecision) {
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillRepeatedly (Return (EFI_NOT_FOUND));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ())
    .WillRepeatedly (Return (TRUE));

  ASSERT_EQ (
    DxeImageVerificationHandler (0, &mDevicePath, mAuthenticodeImage, sizeof (mAuthenticodeImage), FALSE),
    EFI_ACCESS_DENIED
    );
  auto  OldTable = Table;

  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_InstallConfigurationTable (_, _))
    .WillOnce (Return (EFI_OUT_OF_RESOURCES));

  EXPECT_EQ (
    DxeImageVerificationHandler (0, &mDevicePath, mAuthenticodeImage, sizeof (mAuthenticodeImage), FALSE),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Table, OldTable);
  EXPECT_EQ (Table->NumberOfImages, (UINT32)1);
}

TEST_F (DxeImageVerificationHandlerTest, PublishFailure_DoesNotRejectAuthorizedImage) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xA1);
  auto                Db    = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  auto                Image = BuildPe32PlusImage ();

  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ())
    .WillOnce (Return (TRUE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (Digest));
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_InstallConfigurationTable (_, _))
    .WillOnce (Return (EFI_OUT_OF_RESOURCES));

  EXPECT_EQ (
    DxeImageVerificationHandler (0, &mDevicePath, Image.data (), Image.size (), FALSE),
    EFI_SUCCESS
    );
  EXPECT_EQ (Table, nullptr);
}

TEST_F (DxeImageVerificationHandlerTest, MissingImageRecord_DoesNotRejectAuthorizedImage) {
  std::vector<UINT8>  Digest (kSha256DigestSize, 0xA1);
  auto                Db    = BuildSignatureDatabase (gEfiCertSha256Guid, Digest);
  auto                Image = BuildPe32PlusImage ();

  EXPECT_CALL (DevicePathLibMock, GetDevicePathSize (_))
    .WillOnce (Return ((UINTN)0));
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ())
    .WillOnce (Return (TRUE));
  EXPECT_CALL (UefiLibMock, GetVariable2 (_, _, _, _))
    .WillOnce (ReturnVariablePayload (Db))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (ReturnSha256Digest (Digest));
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_InstallConfigurationTable (_, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (0, &mDevicePath, Image.data (), Image.size (), FALSE),
    EFI_SUCCESS
    );
}

TEST_F (DxeImageVerificationHandlerTest, TextualDevicePathName_IsCopiedIntoRecord) {
  CONST CHAR16  Name[]        = { 'P', 'c', 'i', '(', '0', ')', '/', 't', 'e', 's', 't', '.', 'e', 'f', 'i', 0 };
  auto          ConvertedName = (CHAR16 *)AllocateCopyPool (sizeof (Name), Name);

  ASSERT_NE (ConvertedName, nullptr);
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ())
    .WillOnce (Return (TRUE));
  EXPECT_CALL (DevicePathLibMock, ConvertDevicePathToText (&mDevicePath, FALSE, TRUE))
    .WillOnce (Return (ConvertedName));

  EXPECT_EQ (
    DxeImageVerificationHandler (0, &mDevicePath, mAuthenticodeImage, sizeof (mAuthenticodeImage), FALSE),
    EFI_ACCESS_DENIED
    );
  ASSERT_NE (Table, nullptr);
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;

  ASSERT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table));
  auto  Result = ImageVerificationResultIteratorNextImage (&Iter);

  ASSERT_NE (Result, nullptr);
  ASSERT_EQ (Result->NameLength, sizeof (Name));
  EXPECT_EQ (CompareMem (Result + 1, Name, sizeof (Name)), 0);
  EXPECT_EQ (
    CompareMem ((UINT8 *)(Result + 1) + Result->NameLength, &mDevicePath, sizeof (mDevicePath)),
    0
    );
}

TEST_F (DxeImageVerificationHandlerTest, InvalidExistingTable_IsPreserved) {
  Table = (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE *)AllocateZeroPool (sizeof (*Table));
  ASSERT_NE (Table, nullptr);
  auto  OldTable = Table;

  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_LocateDevicePath (_, _, _))
    .WillOnce (Return (EFI_NOT_FOUND));
  EXPECT_CALL (SecureBootVariableLibMock, IsSecureBootEnabled ())
    .WillOnce (Return (TRUE));
  EXPECT_CALL (UefiBootServicesTableLibMock, gBS_InstallConfigurationTable (_, _)).Times (0);

  EXPECT_EQ (
    DxeImageVerificationHandler (0, &mDevicePath, mAuthenticodeImage, sizeof (mAuthenticodeImage), FALSE),
    EFI_ACCESS_DENIED
    );
  EXPECT_EQ (Table, OldTable);
}

TEST (DxeImageVerificationLibConstructorTest, RegistersSecurity2HandlerAndReturnsStatus) {
  MockSecurityManagementLib              SecurityManagementLibMock;
  MockBaseCryptLib                       BaseCryptLibMock;
  SECURITY2_FILE_AUTHENTICATION_HANDLER  RegisteredHandler;

  RegisteredHandler = NULL;
  EXPECT_CALL (BaseCryptLibMock, GetCryptoOpCapability (_, _, _)).Times (0);
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

TEST (DxeImageVerificationLibConstructorTest, SuccessfulRegistrationReportsCapabilities) {
  MockSecurityManagementLib  SecurityManagementLibMock;
  MockBaseCryptLib           BaseCryptLibMock;

  EXPECT_CALL (
    SecurityManagementLibMock,
    RegisterSecurity2Handler (
      _,
      (UINT32)(EFI_AUTH_OPERATION_VERIFY_IMAGE | EFI_AUTH_OPERATION_IMAGE_REQUIRED)
      )
    )
    .WillOnce (Return (EFI_SUCCESS));
  EXPECT_CALL (BaseCryptLibMock, GetCryptoOpCapability (_, _, _))
    .WillOnce (Return (EFI_UNSUPPORTED));

  EXPECT_EQ (DxeImageVerificationLibConstructor (NULL, NULL), EFI_SUCCESS);
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
