/** @file
  Unit tests for the ImageSecureBootVerificationResultTableLib write side:
  ImageVerificationResultCreateImage / AppendSignature / AppendImage.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

#include <vector>
#include <cstring>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/ImageSecureBootVerificationResultTableLib.h>
}

//
// GoogleTest entry point for the whole suite (both source files link into one
// host application).
//
int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}

// ---------------------------------------------------------------------------
// Shared test data.
// ---------------------------------------------------------------------------

// A minimal "End Entire" device path node (type 0x7F, subtype 0xFF, length 4).
static const UINT8  kDevicePath[] = { 0x7F, 0xFF, 0x04, 0x00 };

// CHAR16 image name built from ints (host CHAR16 is 2 bytes; never use L"...").
static const CHAR16  kName[] = { 'I', 'm', 'g', 0 };

static const EFI_GUID  kSha256 = {
  0x51aa59de, 0xfdf2, 0x4ea3, { 0xbc, 0x63, 0x87, 0x5f, 0xb7, 0x84, 0x2e, 0xe9 }
};

static const UINT8  kThumbprint[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

// Read an image record's fixed header (a plain pool allocation is 8-aligned).
static const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *
AsImage (
  const VOID  *Image
  )
{
  return (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)Image;
}

// ---------------------------------------------------------------------------
// CreateImage
// ---------------------------------------------------------------------------

TEST (CreateImageTest, NullImageOut_ReturnsInvalidParameter) {
  EXPECT_EQ (
    ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST (CreateImageTest, NullDevicePath_ReturnsInvalidParameter) {
  VOID  *Image = NULL;

  EXPECT_EQ (ImageVerificationResultCreateImage (NULL, NULL, 4, &Image), EFI_INVALID_PARAMETER);
}

TEST (CreateImageTest, ZeroDevicePathSize_ReturnsInvalidParameter) {
  VOID  *Image = NULL;

  EXPECT_EQ (ImageVerificationResultCreateImage (NULL, kDevicePath, 0, &Image), EFI_INVALID_PARAMETER);
}

TEST (CreateImageTest, DevicePathSizeOverflow_ReturnsBadBufferSize) {
  VOID  *Image = NULL;

  EXPECT_EQ (
    ImageVerificationResultCreateImage (NULL, kDevicePath, (UINTN)MAX_UINT32, &Image),
    EFI_BAD_BUFFER_SIZE
    );
}

TEST (CreateImageTest, PopulatesHeader) {
  VOID  *Image = NULL;

  ASSERT_EQ (
    ImageVerificationResultCreateImage (kName, kDevicePath, sizeof (kDevicePath), &Image),
    EFI_SUCCESS
    );

  // Name and device path are recorded up front; status is stamped later.
  EXPECT_EQ (AsImage (Image)->NumberOfSignatures, 0u);
  EXPECT_EQ (AsImage (Image)->NameLength, (UINT32)StrSize (kName));
  EXPECT_EQ (AsImage (Image)->DevicePathLength, (UINT32)sizeof (kDevicePath));
  EXPECT_EQ (AsImage (Image)->ImageStatus, 0u);
  EXPECT_EQ (
    AsImage (Image)->Length,
    (UINT32)(sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT) + StrSize (kName) + sizeof (kDevicePath))
    );

  FreePool (Image);
}

// ---------------------------------------------------------------------------
// AppendSignature
// ---------------------------------------------------------------------------

TEST (AppendSignatureTest, NullImage_ReturnsInvalidParameter) {
  VOID  *Image = NULL;

  EXPECT_EQ (
    ImageVerificationResultAppendSignature (NULL, 0, EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY, NULL, NULL, 0),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (
    ImageVerificationResultAppendSignature (&Image, 0, EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY, NULL, NULL, 0),
    EFI_INVALID_PARAMETER
    );
}

TEST (AppendSignatureTest, ThumbprintWithoutSize_ReturnsInvalidParameter) {
  VOID  *Image = NULL;

  ASSERT_EQ (ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), &Image), EFI_SUCCESS);
  EXPECT_EQ (
    ImageVerificationResultAppendSignature (&Image, 0, EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY, &kSha256, kThumbprint, 0),
    EFI_INVALID_PARAMETER
    );
  FreePool (Image);
}

TEST (AppendSignatureTest, SizeWithoutThumbprint_ReturnsInvalidParameter) {
  VOID  *Image = NULL;

  ASSERT_EQ (ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), &Image), EFI_SUCCESS);
  EXPECT_EQ (
    ImageVerificationResultAppendSignature (&Image, 0, EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY, NULL, NULL, sizeof (kThumbprint)),
    EFI_INVALID_PARAMETER
    );
  FreePool (Image);
}

TEST (AppendSignatureTest, ExtendsImage) {
  VOID  *Image = NULL;

  ASSERT_EQ (ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), &Image), EFI_SUCCESS);

  UINT32  LengthBefore = AsImage (Image)->Length;

  ASSERT_EQ (
    ImageVerificationResultAppendSignature (&Image, 3, EFI_SIGNATURE_VERIFICATION_TBS_HASH_AUTHORITY, &kSha256, kThumbprint, sizeof (kThumbprint)),
    EFI_SUCCESS
    );

  // The image grew by exactly one signature record and counts it.
  EXPECT_EQ (
    AsImage (Image)->Length,
    (UINT32)(LengthBefore + sizeof (EFI_SIGNATURE_VERIFICATION_RESULT) + sizeof (kThumbprint))
    );
  EXPECT_EQ (AsImage (Image)->NumberOfSignatures, 1u);

  FreePool (Image);
}

// ---------------------------------------------------------------------------
// AppendImage
// ---------------------------------------------------------------------------

TEST (AppendImageTest, NullImage_ReturnsInvalidParameter) {
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *Table;

  EXPECT_EQ (
    ImageVerificationResultAppendImage (NULL, NULL, EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY, NULL, &Table),
    EFI_INVALID_PARAMETER
    );
}

TEST (AppendImageTest, NullNewTable_ReturnsInvalidParameter) {
  VOID  *Image = NULL;

  ASSERT_EQ (ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), &Image), EFI_SUCCESS);
  EXPECT_EQ (
    ImageVerificationResultAppendImage (NULL, Image, EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY, NULL, NULL),
    EFI_INVALID_PARAMETER
    );
  FreePool (Image);
}

TEST (AppendImageTest, BadOldTable_ReturnsInvalidParameter) {
  VOID                                             *Image = NULL;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *Table;
  UINT8                                            Junk[sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE)] = { 0 };

  ASSERT_EQ (ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), &Image), EFI_SUCCESS);
  EXPECT_EQ (
    ImageVerificationResultAppendImage (Junk, Image, EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY, NULL, &Table),
    EFI_INVALID_PARAMETER
    );
  FreePool (Image);
}

TEST (AppendImageTest, NullOldTable_ProducesSingleImageTable) {
  VOID                                             *Image = NULL;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *Table;

  ASSERT_EQ (ImageVerificationResultCreateImage (kName, kDevicePath, sizeof (kDevicePath), &Image), EFI_SUCCESS);
  ASSERT_EQ (
    ImageVerificationResultAppendImage (NULL, Image, EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_DIGEST, &kSha256, &Table),
    EFI_SUCCESS
    );

  EXPECT_EQ (Table->Signature, (UINT32)EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE);
  EXPECT_EQ (Table->Version, (UINT32)EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION);
  EXPECT_EQ (Table->NumberOfImages, 1u);

  // Status/digest are stamped into the table copy; the source image is unchanged.
  const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT  *Rec =
    (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)((const UINT8 *)Table +
                                                        sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE));

  EXPECT_EQ (Rec->ImageStatus, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_DIGEST);
  EXPECT_EQ (Rec->NameLength, (UINT32)StrSize (kName));
  EXPECT_EQ (0, memcmp (&Rec->ImageDigestAlgorithm, &kSha256, sizeof (EFI_GUID)));
  EXPECT_EQ (AsImage (Image)->ImageStatus, 0u);

  FreePool (Image);
  FreePool (Table);
}

TEST (AppendImageTest, AppendsToExistingTable) {
  VOID                                             *Image0 = NULL;
  VOID                                             *Image1 = NULL;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *Table0;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *Table1;

  ASSERT_EQ (ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), &Image0), EFI_SUCCESS);
  ASSERT_EQ (
    ImageVerificationResultAppendImage (NULL, Image0, EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY, NULL, &Table0),
    EFI_SUCCESS
    );

  ASSERT_EQ (ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), &Image1), EFI_SUCCESS);
  ASSERT_EQ (
    ImageVerificationResultAppendImage (Table0, Image1, EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY, NULL, &Table1),
    EFI_SUCCESS
    );

  // The old table is untouched; the new one has both images and is larger.
  EXPECT_EQ (Table0->NumberOfImages, 1u);
  EXPECT_EQ (Table1->NumberOfImages, 2u);
  EXPECT_GT (Table1->Length, Table0->Length);

  FreePool (Image0);
  FreePool (Image1);
  FreePool (Table0);
  FreePool (Table1);
}

// ---------------------------------------------------------------------------
// Build / iterate round trip.
// ---------------------------------------------------------------------------

TEST (WriteRoundTripTest, TwoImagesWithSignatures) {
  VOID                                             *Image0 = NULL;
  VOID                                             *Image1 = NULL;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *Table0;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *Table;

  // Image 0: named, digest-approved, two signatures.
  ASSERT_EQ (ImageVerificationResultCreateImage (kName, kDevicePath, sizeof (kDevicePath), &Image0), EFI_SUCCESS);
  ASSERT_EQ (
    ImageVerificationResultAppendSignature (&Image0, 0, EFI_SIGNATURE_VERIFICATION_REJECTED_NO_AUTHORITY, NULL, NULL, 0),
    EFI_SUCCESS
    );
  ASSERT_EQ (
    ImageVerificationResultAppendSignature (&Image0, 1, EFI_SIGNATURE_VERIFICATION_TBS_HASH_AUTHORITY, &kSha256, kThumbprint, sizeof (kThumbprint)),
    EFI_SUCCESS
    );
  ASSERT_EQ (
    ImageVerificationResultAppendImage (NULL, Image0, EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_DIGEST, &kSha256, &Table0),
    EFI_SUCCESS
    );

  // Image 1: no name, rejected, one signature.
  ASSERT_EQ (ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), &Image1), EFI_SUCCESS);
  ASSERT_EQ (
    ImageVerificationResultAppendSignature (&Image1, 0, EFI_SIGNATURE_VERIFICATION_AUTHORITY_REVOKED_BY_CERTIFICATE, &kSha256, kThumbprint, sizeof (kThumbprint)),
    EFI_SUCCESS
    );
  ASSERT_EQ (
    ImageVerificationResultAppendImage (Table0, Image1, EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY, NULL, &Table),
    EFI_SUCCESS
    );

  EXPECT_EQ (Table->NumberOfImages, 2u);

  // Walk it back and confirm the structure round-trips cleanly.
  IMAGE_VERIFICATION_RESULT_ITERATOR               Iter;
  const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT  *Rec;
  const EFI_SIGNATURE_VERIFICATION_RESULT          *Sig;

  EXPECT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table));

  // Image 0.
  Rec = ImageVerificationResultIteratorNextImage (&Iter);
  ASSERT_NE (Rec, (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Rec->ImageStatus, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_DIGEST);
  EXPECT_EQ (Rec->NumberOfSignatures, 2u);
  EXPECT_EQ (Rec->NameLength, (UINT32)StrSize (kName));
  EXPECT_EQ (Rec->DevicePathLength, (UINT32)sizeof (kDevicePath));
  EXPECT_EQ (0, memcmp (&Rec->ImageDigestAlgorithm, &kSha256, sizeof (EFI_GUID)));

  Sig = ImageVerificationResultIteratorNextSignature (&Iter);
  ASSERT_NE (Sig, (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Sig->SignatureIndex, 0u);
  EXPECT_EQ (Sig->Status, (UINT32)EFI_SIGNATURE_VERIFICATION_REJECTED_NO_AUTHORITY);
  EXPECT_EQ (Sig->Length, (UINT32)sizeof (EFI_SIGNATURE_VERIFICATION_RESULT));

  Sig = ImageVerificationResultIteratorNextSignature (&Iter);
  ASSERT_NE (Sig, (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Sig->SignatureIndex, 1u);
  EXPECT_EQ (Sig->Status, (UINT32)EFI_SIGNATURE_VERIFICATION_TBS_HASH_AUTHORITY);
  EXPECT_EQ (Sig->Length, (UINT32)(sizeof (EFI_SIGNATURE_VERIFICATION_RESULT) + sizeof (kThumbprint)));
  EXPECT_EQ (0, memcmp ((const UINT8 *)Sig + sizeof (EFI_SIGNATURE_VERIFICATION_RESULT), kThumbprint, sizeof (kThumbprint)));

  EXPECT_EQ (ImageVerificationResultIteratorNextSignature (&Iter), (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);

  // Image 1.
  Rec = ImageVerificationResultIteratorNextImage (&Iter);
  ASSERT_NE (Rec, (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Rec->ImageStatus, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY);
  EXPECT_EQ (Rec->NumberOfSignatures, 1u);
  EXPECT_EQ (Rec->NameLength, 0u);

  Sig = ImageVerificationResultIteratorNextSignature (&Iter);
  ASSERT_NE (Sig, (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Sig->SignatureIndex, 0u);
  EXPECT_EQ (Sig->Status, (UINT32)EFI_SIGNATURE_VERIFICATION_AUTHORITY_REVOKED_BY_CERTIFICATE);

  EXPECT_EQ (ImageVerificationResultIteratorNextImage (&Iter), (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);

  FreePool (Image0);
  FreePool (Image1);
  FreePool (Table0);
  FreePool (Table);
}
