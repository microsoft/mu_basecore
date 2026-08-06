/** @file
  Unit tests for the ImageSecureBootVerificationResultTableLib iterator:
  ImageVerificationResultIteratorInit / NextImage / NextSignature.

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

// ---------------------------------------------------------------------------
// Shared test data and helpers.
// ---------------------------------------------------------------------------

static const UINT8  kDevicePath[] = { 0x7F, 0xFF, 0x04, 0x00 };

static const EFI_GUID  kSha256 = {
  0x51aa59de, 0xfdf2, 0x4ea3, { 0xbc, 0x63, 0x87, 0x5f, 0xb7, 0x84, 0x2e, 0xe9 }
};

// Byte offsets of the table header fields.
static const size_t  kTableHeaderSize = sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE);

// Test-local descriptor for a signature to append. The library appends
// signatures one at a time, so the helpers below iterate this array and call
// AppendSignature.
typedef struct {
  UINT32          SignatureIndex;
  UINT32          Status;
  const EFI_GUID  *ThumbprintAlgorithm;
  const void      *Thumbprint;
  UINTN           ThumbprintSize;
} TEST_SIGNATURE_INFO;

static void
SetU32 (
  std::vector<UINT8>  &Buffer,
  size_t              Offset,
  UINT32              Value
  )
{
  Buffer[Offset + 0] = (UINT8)(Value);
  Buffer[Offset + 1] = (UINT8)(Value >> 8);
  Buffer[Offset + 2] = (UINT8)(Value >> 16);
  Buffer[Offset + 3] = (UINT8)(Value >> 24);
}

// Build a valid empty table (header only, zero images) by hand, so tests can
// mutate the bytes freely.
static std::vector<UINT8>
MakeEmptyTable (
  void
  )
{
  std::vector<UINT8>  Table (kTableHeaderSize, 0);

  SetU32 (Table, OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE, Signature), EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_SIGNATURE);
  SetU32 (Table, OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE, Version), EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE_VERSION);
  SetU32 (Table, OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE, Length), (UINT32)kTableHeaderSize);
  SetU32 (Table, OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE, NumberOfImages), 0);
  return Table;
}

// Append one image (with the shared device path and the given signatures) onto
// an existing table (Old empty => a fresh table) via the real API, returning the
// resulting table as an owned byte vector.
static std::vector<UINT8>
AppendImageToTable (
  const std::vector<UINT8>   &Old,
  UINT32                     ImageStatus,
  const TEST_SIGNATURE_INFO  *Signatures,
  UINTN                      SignatureCount
  )
{
  VOID                                             *Image    = NULL;
  EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE  *NewTable = NULL;
  std::vector<UINT8>                               Bytes;
  const VOID                                       *OldPtr = Old.empty () ? NULL : (const VOID *)Old.data ();

  if (EFI_ERROR (ImageVerificationResultCreateImage (NULL, kDevicePath, sizeof (kDevicePath), &Image))) {
    return Bytes;
  }

  for (UINTN Index = 0; Index < SignatureCount; Index++) {
    ImageVerificationResultAppendSignature (
      &Image,
      Signatures[Index].SignatureIndex,
      Signatures[Index].Status,
      Signatures[Index].ThumbprintAlgorithm,
      Signatures[Index].Thumbprint,
      Signatures[Index].ThumbprintSize
      );
  }

  if (!EFI_ERROR (
         ImageVerificationResultAppendImage (OldPtr, Image, ImageStatus, NULL, &NewTable)
         ))
  {
    Bytes.assign ((UINT8 *)NewTable, (UINT8 *)NewTable + NewTable->Length);
    FreePool (NewTable);
  }

  FreePool (Image);
  return Bytes;
}

// Build a table with one image (no name, 4-byte device path) carrying
// SignatureCount signatures (indices 0..N-1, no thumbprints).
static std::vector<UINT8>
BuildSingleImageTable (
  UINT32  SignatureCount
  )
{
  std::vector<TEST_SIGNATURE_INFO>  Signatures (SignatureCount);

  for (UINT32 Index = 0; Index < SignatureCount; Index++) {
    ZeroMem (&Signatures[Index], sizeof (TEST_SIGNATURE_INFO));
    Signatures[Index].SignatureIndex = Index;
    Signatures[Index].Status         = EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY;
  }

  return AppendImageToTable (
           std::vector<UINT8> (),
           EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY,
           (SignatureCount == 0) ? NULL : Signatures.data (),
           SignatureCount
           );
}

// ---------------------------------------------------------------------------
// Init: unusable / empty inputs.
// ---------------------------------------------------------------------------

TEST (IteratorInitTest, NullIterator_ReturnsFalse) {
  UINT8  Dummy[kTableHeaderSize] = { 0 };

  EXPECT_FALSE (ImageVerificationResultIteratorInit (NULL, Dummy));
}

TEST (IteratorInitTest, NullTable_IsCleanEmpty) {
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;

  EXPECT_TRUE (ImageVerificationResultIteratorInit (&Iter, NULL));
  EXPECT_EQ (ImageVerificationResultIteratorNextImage (&Iter), (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (ImageVerificationResultIteratorNextSignature (&Iter), (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
}

TEST (IteratorInitTest, BadSignature_ReturnsFalse) {
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;
  std::vector<UINT8>                  Table = BuildSingleImageTable (0);

  SetU32 (Table, 0, 0x11223344);
  EXPECT_FALSE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));
}

TEST (IteratorInitTest, BadVersion_ReturnsFalse) {
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;
  std::vector<UINT8>                  Table = BuildSingleImageTable (0);

  SetU32 (Table, OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE, Version), 0xDEAD);
  EXPECT_FALSE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));
}

TEST (IteratorInitTest, LengthBelowHeader_ReturnsFalse) {
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;
  std::vector<UINT8>                  Table = BuildSingleImageTable (0);

  SetU32 (Table, OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE, Length), 4);
  EXPECT_FALSE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));
}

// ---------------------------------------------------------------------------
// Init: well-formed tables.
// ---------------------------------------------------------------------------

TEST (IteratorInitTest, EmptyTable_IsClean) {
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;
  std::vector<UINT8>                  Table = MakeEmptyTable ();

  EXPECT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));
  EXPECT_EQ (ImageVerificationResultIteratorNextImage (&Iter), (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
}

TEST (IteratorTest, SingleImageNoSignatures) {
  IMAGE_VERIFICATION_RESULT_ITERATOR              Iter;
  std::vector<UINT8>                              Table = BuildSingleImageTable (0);
  const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *Image;

  EXPECT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));

  Image = ImageVerificationResultIteratorNextImage (&Iter);
  ASSERT_NE (Image, (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Image->NumberOfSignatures, 0u);
  EXPECT_EQ (Image->DevicePathLength, (UINT32)sizeof (kDevicePath));
  EXPECT_EQ (ImageVerificationResultIteratorNextSignature (&Iter), (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (ImageVerificationResultIteratorNextImage (&Iter), (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
}

TEST (IteratorTest, SingleImageWalksAllSignatures) {
  IMAGE_VERIFICATION_RESULT_ITERATOR              Iter;
  std::vector<UINT8>                              Table = BuildSingleImageTable (3);
  const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *Image;
  const EFI_SIGNATURE_VERIFICATION_RESULT         *Sig;

  EXPECT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));

  Image = ImageVerificationResultIteratorNextImage (&Iter);
  ASSERT_NE (Image, (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Image->NumberOfSignatures, 3u);

  for (UINT32 Index = 0; Index < 3; Index++) {
    Sig = ImageVerificationResultIteratorNextSignature (&Iter);
    ASSERT_NE (Sig, (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
    EXPECT_EQ (Sig->SignatureIndex, Index);
  }

  EXPECT_EQ (ImageVerificationResultIteratorNextSignature (&Iter), (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
}

TEST (IteratorTest, NextSignatureBeforeNextImage_ReturnsNull) {
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;
  std::vector<UINT8>                  Table = BuildSingleImageTable (2);

  EXPECT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));
  // No image selected yet, so the inner cursor yields nothing.
  EXPECT_EQ (ImageVerificationResultIteratorNextSignature (&Iter), (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
}

TEST (IteratorTest, NextImageResetsInnerCursor) {
  IMAGE_VERIFICATION_RESULT_ITERATOR              Iter;
  const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *Image;
  const EFI_SIGNATURE_VERIFICATION_RESULT         *Sig;
  TEST_SIGNATURE_INFO                             Sigs0[2];
  TEST_SIGNATURE_INFO                             Sigs1[2];

  ZeroMem (Sigs0, sizeof (Sigs0));
  Sigs0[0].SignatureIndex = 0;
  Sigs0[0].Status         = EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY;
  Sigs0[1].SignatureIndex = 1;
  Sigs0[1].Status         = EFI_SIGNATURE_VERIFICATION_TBS_HASH_AUTHORITY;

  ZeroMem (Sigs1, sizeof (Sigs1));
  Sigs1[0].SignatureIndex = 0;
  Sigs1[0].Status         = EFI_SIGNATURE_VERIFICATION_AUTHORITY_REVOKED_BY_CERTIFICATE;
  Sigs1[1].SignatureIndex = 1;
  Sigs1[1].Status         = EFI_SIGNATURE_VERIFICATION_AUTHORITY_REVOKED_BY_TBS_HASH;

  // Image 0 with two signatures, then image 1 with two distinct-status signatures.
  std::vector<UINT8>  Table = AppendImageToTable (std::vector<UINT8> (), EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY, Sigs0, 2);
  Table = AppendImageToTable (Table, EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY, Sigs1, 2);

  EXPECT_TRUE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));

  // Select image 0 and consume only its first signature.
  Image = ImageVerificationResultIteratorNextImage (&Iter);
  ASSERT_NE (Image, (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  Sig = ImageVerificationResultIteratorNextSignature (&Iter);
  ASSERT_NE (Sig, (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Sig->Status, (UINT32)EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY);

  // Advancing to image 1 must reset the inner cursor to image 1's signatures,
  // NOT continue with image 0's unconsumed second signature.
  Image = ImageVerificationResultIteratorNextImage (&Iter);
  ASSERT_NE (Image, (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Image->ImageStatus, (UINT32)EFI_IMAGE_VERIFICATION_STATUS_REJECTED_NO_AUTHORITY);

  Sig = ImageVerificationResultIteratorNextSignature (&Iter);
  ASSERT_NE (Sig, (const EFI_SIGNATURE_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (Sig->Status, (UINT32)EFI_SIGNATURE_VERIFICATION_AUTHORITY_REVOKED_BY_CERTIFICATE);
  EXPECT_EQ (Sig->SignatureIndex, 0u);
}

// ---------------------------------------------------------------------------
// Init: truncation clamps to the valid prefix.
// ---------------------------------------------------------------------------

TEST (IteratorTruncationTest, TrailingPartialImage_IteratesValidPrefix) {
  IMAGE_VERIFICATION_RESULT_ITERATOR              Iter;
  const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *Image;

  // Two well-formed images (no signatures).
  std::vector<UINT8>  Table = AppendImageToTable (std::vector<UINT8> (), EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY, NULL, 0);
  Table = AppendImageToTable (Table, EFI_IMAGE_VERIFICATION_STATUS_AUTHORIZED_BY_AUTHORITY, NULL, 0);
  size_t              ValidSize = Table.size ();

  // Append 4 junk bytes and claim them as a third (partial) image record.
  Table.resize (ValidSize + 4, 0);
  SetU32 (Table, OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE, Length), (UINT32)(ValidSize + 4));
  SetU32 (Table, OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE, NumberOfImages), 3);

  // Truncated (FALSE), but the two well-formed images still iterate.
  EXPECT_FALSE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));

  Image = ImageVerificationResultIteratorNextImage (&Iter);
  EXPECT_NE (Image, (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  Image = ImageVerificationResultIteratorNextImage (&Iter);
  EXPECT_NE (Image, (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  Image = ImageVerificationResultIteratorNextImage (&Iter);
  EXPECT_EQ (Image, (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
}

TEST (IteratorTruncationTest, ImageCountMismatch_ReturnsFalse) {
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;
  std::vector<UINT8>                  Table = BuildSingleImageTable (0);

  // Region still tiles into one image, but the header over-claims the count.
  SetU32 (Table, OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT_TABLE, NumberOfImages), 5);
  EXPECT_FALSE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));

  // The one real image is still exposed.
  EXPECT_NE (ImageVerificationResultIteratorNextImage (&Iter), (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
  EXPECT_EQ (ImageVerificationResultIteratorNextImage (&Iter), (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
}

TEST (IteratorTruncationTest, SignatureCountMismatch_DropsImage) {
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;
  std::vector<UINT8>                  Table = BuildSingleImageTable (1);
  size_t                              ImageOffset = kTableHeaderSize;

  // The image physically carries one signature; claim two.
  SetU32 (
    Table,
    ImageOffset + OFFSET_OF (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT, NumberOfSignatures),
    2
    );

  // The malformed image is dropped, leaving an empty (but truncated) prefix.
  EXPECT_FALSE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));
  EXPECT_EQ (ImageVerificationResultIteratorNextImage (&Iter), (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
}

TEST (IteratorTruncationTest, ShortSignatureLength_DropsImage) {
  IMAGE_VERIFICATION_RESULT_ITERATOR  Iter;
  std::vector<UINT8>                  Table = BuildSingleImageTable (1);
  size_t                              ImageOffset = kTableHeaderSize;
  size_t                              SigOffset;

  // Signatures begin after the image header + (0-byte) name + device path.
  SigOffset = ImageOffset + sizeof (EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT) + sizeof (kDevicePath);
  SetU32 (Table, SigOffset + OFFSET_OF (EFI_SIGNATURE_VERIFICATION_RESULT, Length), 4);

  EXPECT_FALSE (ImageVerificationResultIteratorInit (&Iter, Table.data ()));
  EXPECT_EQ (ImageVerificationResultIteratorNextImage (&Iter), (const EFI_IMAGE_SECURE_BOOT_VERIFICATION_RESULT *)NULL);
}
