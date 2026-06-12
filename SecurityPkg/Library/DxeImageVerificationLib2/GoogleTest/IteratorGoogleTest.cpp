/** @file
  Unit tests for the iterators in Iterator.c:
  DatabaseIterInit/Next, SigListIterInit/Next, and WinCertIterInit/Next.

  All three iterators follow the same contract: Init validates the
  container and clamps the iteration range to the valid prefix, returning
  TRUE when it had to truncate (drop trailing entries) and FALSE when the
  whole container parsed cleanly. Next is infallible over that range.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

#include <vector>
#include <cstring>

extern "C" {
  #include <Uefi.h>
  #include <Guid/ImageAuthentication.h>
  #include <IndustryStandard/PeImage.h>
  #include <Library/BaseMemoryLib.h>
  #include "../Iterator.h"
}

// ---------------------------------------------------------------------------
// Helpers for constructing synthetic signature-list buffers.
// ---------------------------------------------------------------------------

// SHA-256 entry size: 16-byte owner GUID + 32-byte digest.
static constexpr UINT32  kSha256EntrySize = sizeof (EFI_GUID) + 32;

//
// Append one EFI_SIGNATURE_LIST containing SignatureCount entries of
// EntrySize bytes each (entry payloads are zero-initialized) to Buffer.
// Returns the offset of the new list within Buffer.
//
static size_t
AppendSignatureList (
  std::vector<UINT8>  &Buffer,
  const EFI_GUID      &SignatureType,
  UINT32              SignatureHeaderSize,
  UINT32              EntrySize,
  UINT32              SignatureCount
  )
{
  const size_t  PayloadBytes = (size_t)EntrySize * (size_t)SignatureCount;
  const size_t  ListBytes    = sizeof (EFI_SIGNATURE_LIST) + SignatureHeaderSize + PayloadBytes;
  const size_t  Offset       = Buffer.size ();

  Buffer.resize (Offset + ListBytes, 0);

  EFI_SIGNATURE_LIST  *List = (EFI_SIGNATURE_LIST *)(Buffer.data () + Offset);

  CopyMem (&List->SignatureType, &SignatureType, sizeof (EFI_GUID));
  List->SignatureListSize   = (UINT32)ListBytes;
  List->SignatureHeaderSize = SignatureHeaderSize;
  List->SignatureSize       = EntrySize;

  return Offset;
}

// ---------------------------------------------------------------------------
// DatabaseIterInit
// ---------------------------------------------------------------------------

TEST (DatabaseIterInitTest, NullIter_ReturnsTruncated) {
  std::vector<UINT8>  Buffer (16, 0);

  EXPECT_FALSE (DatabaseIterInit (NULL, Buffer.data (), Buffer.size ()));
}

TEST (DatabaseIterInitTest, NullBufferWithNonZeroSize_ReturnsTruncated) {
  SIG_DATABASE_ITER  Iter;

  EXPECT_FALSE (DatabaseIterInit (&Iter, NULL, 16));
  EXPECT_EQ (Iter.Remaining, (UINTN)0);
  EXPECT_EQ (DatabaseIterNext (&Iter), nullptr);
}

TEST (DatabaseIterInitTest, NullBufferWithZeroSize_NotTruncated) {
  SIG_DATABASE_ITER  Iter;

  EXPECT_TRUE (DatabaseIterInit (&Iter, NULL, 0));
  EXPECT_EQ (Iter.Remaining, (UINTN)0);
  EXPECT_EQ (DatabaseIterNext (&Iter), nullptr);
}

TEST (DatabaseIterInitTest, EmptyBuffer_NotTruncated) {
  std::vector<UINT8>  Buffer;
  SIG_DATABASE_ITER   Iter;

  EXPECT_TRUE (DatabaseIterInit (&Iter, Buffer.data (), 0));
  EXPECT_EQ (DatabaseIterNext (&Iter), nullptr);
}

TEST (DatabaseIterInitTest, SingleWellFormedList_NotTruncated) {
  std::vector<UINT8>  Buffer;
  SIG_DATABASE_ITER   Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);

  EXPECT_TRUE (DatabaseIterInit (&Iter, Buffer.data (), Buffer.size ()));
}

// A well-formed list followed by stray bytes too small to be a header: the
// range is clamped to the valid list and the trailing bytes are dropped.
TEST (DatabaseIterInitTest, TrailingBytesBelowHeader_TruncatesToValidPrefix) {
  std::vector<UINT8>  Buffer;
  SIG_DATABASE_ITER   Iter;
  size_t              Off1;

  Off1 = AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  // Append a few stray bytes too small to be even a list header.
  Buffer.resize (Buffer.size () + sizeof (EFI_SIGNATURE_LIST) - 1, 0);

  EXPECT_FALSE (DatabaseIterInit (&Iter, Buffer.data (), Buffer.size ()));
  // The one well-formed list preceding the stray bytes is still iterable.
  EXPECT_EQ ((CONST UINT8 *)DatabaseIterNext (&Iter), Buffer.data () + Off1);
  EXPECT_EQ (DatabaseIterNext (&Iter), nullptr);
}

// The only list has a SignatureListSize below the header size: nothing can be
// parsed, so the range is clamped to empty.
TEST (DatabaseIterInitTest, ListSizeBelowHeaderSize_TruncatesToEmpty) {
  std::vector<UINT8>  Buffer;
  SIG_DATABASE_ITER   Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  ((EFI_SIGNATURE_LIST *)Buffer.data ())->SignatureListSize = sizeof (EFI_SIGNATURE_LIST) - 1;

  EXPECT_FALSE (DatabaseIterInit (&Iter, Buffer.data (), Buffer.size ()));
  EXPECT_EQ (Iter.Remaining, (UINTN)0);
  EXPECT_EQ (DatabaseIterNext (&Iter), nullptr);
}

// The only list claims a size larger than the buffer: nothing can be parsed,
// so the range is clamped to empty.
TEST (DatabaseIterInitTest, ListSizeOverrunsBuffer_TruncatesToEmpty) {
  std::vector<UINT8>  Buffer;
  SIG_DATABASE_ITER   Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  ((EFI_SIGNATURE_LIST *)Buffer.data ())->SignatureListSize = (UINT32)(Buffer.size () + 1);

  EXPECT_FALSE (DatabaseIterInit (&Iter, Buffer.data (), Buffer.size ()));
  EXPECT_EQ (Iter.Remaining, (UINTN)0);
  EXPECT_EQ (DatabaseIterNext (&Iter), nullptr);
}

// Init only checks the outer tiling; bad internals belong to SigListIterInit.
TEST (DatabaseIterInitTest, MalformedInternalsButValidTiling_NotTruncated) {
  std::vector<UINT8>  Buffer;
  SIG_DATABASE_ITER   Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  // SignatureSize < sizeof(EFI_GUID) is internal corruption that
  // DatabaseIterInit deliberately ignores.
  ((EFI_SIGNATURE_LIST *)Buffer.data ())->SignatureSize = 1;

  EXPECT_TRUE (DatabaseIterInit (&Iter, Buffer.data (), Buffer.size ()));
}

// ---------------------------------------------------------------------------
// DatabaseIterNext
// ---------------------------------------------------------------------------

TEST (DatabaseIterNextTest, NullIter_ReturnsNull) {
  EXPECT_EQ (DatabaseIterNext (NULL), nullptr);
}

TEST (DatabaseIterNextTest, EmptyDatabase_ReturnsNull) {
  SIG_DATABASE_ITER  Iter;

  ASSERT_TRUE (DatabaseIterInit (&Iter, NULL, 0));
  EXPECT_EQ (DatabaseIterNext (&Iter), nullptr);
}

TEST (DatabaseIterNextTest, IteratesAllListsInOrder) {
  std::vector<UINT8>  Buffer;
  SIG_DATABASE_ITER   Iter;
  size_t              Off1, Off2, Off3;

  Off1 = AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  Off2 = AppendSignatureList (Buffer, gEfiCertX509Guid, 0, sizeof (EFI_GUID) + 8, 2);
  Off3 = AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 3);

  ASSERT_TRUE (DatabaseIterInit (&Iter, Buffer.data (), Buffer.size ()));

  CONST EFI_SIGNATURE_LIST  *L1 = DatabaseIterNext (&Iter);
  CONST EFI_SIGNATURE_LIST  *L2 = DatabaseIterNext (&Iter);
  CONST EFI_SIGNATURE_LIST  *L3 = DatabaseIterNext (&Iter);
  CONST EFI_SIGNATURE_LIST  *L4 = DatabaseIterNext (&Iter);

  EXPECT_EQ ((CONST UINT8 *)L1, Buffer.data () + Off1);
  EXPECT_EQ ((CONST UINT8 *)L2, Buffer.data () + Off2);
  EXPECT_EQ ((CONST UINT8 *)L3, Buffer.data () + Off3);
  EXPECT_EQ (L4, nullptr);

  // Drained iterator stays drained.
  EXPECT_EQ (DatabaseIterNext (&Iter), nullptr);
}

// ---------------------------------------------------------------------------
// SigListIterInit
// ---------------------------------------------------------------------------

TEST (SigListIterInitTest, NullIter_ReturnsTruncated) {
  std::vector<UINT8>  Buffer;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);

  EXPECT_FALSE (SigListIterInit (NULL, (EFI_SIGNATURE_LIST *)Buffer.data ()));
}

TEST (SigListIterInitTest, NullList_ReturnsTruncated) {
  SIG_LIST_ITER  Iter;

  EXPECT_FALSE (SigListIterInit (&Iter, NULL));
  EXPECT_EQ (Iter.Remaining, (UINTN)0);
  EXPECT_EQ (SigListIterNext (&Iter), nullptr);
}

TEST (SigListIterInitTest, ListSizeBelowHeader_TruncatesToEmpty) {
  std::vector<UINT8>  Buffer;
  SIG_LIST_ITER       Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  ((EFI_SIGNATURE_LIST *)Buffer.data ())->SignatureListSize = sizeof (EFI_SIGNATURE_LIST) - 1;

  EXPECT_FALSE (SigListIterInit (&Iter, (EFI_SIGNATURE_LIST *)Buffer.data ()));
  EXPECT_EQ (Iter.Remaining, (UINTN)0);
  EXPECT_EQ (SigListIterNext (&Iter), nullptr);
}

TEST (SigListIterInitTest, SignatureSizeBelowGuid_TruncatesToEmpty) {
  std::vector<UINT8>  Buffer;
  SIG_LIST_ITER       Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  ((EFI_SIGNATURE_LIST *)Buffer.data ())->SignatureSize = sizeof (EFI_GUID) - 1;

  EXPECT_FALSE (SigListIterInit (&Iter, (EFI_SIGNATURE_LIST *)Buffer.data ()));
  EXPECT_EQ (Iter.Remaining, (UINTN)0);
  EXPECT_EQ (SigListIterNext (&Iter), nullptr);
}

TEST (SigListIterInitTest, HeaderSizeExceedsList_TruncatesToEmpty) {
  std::vector<UINT8>  Buffer;
  SIG_LIST_ITER       Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 1);
  EFI_SIGNATURE_LIST  *List = (EFI_SIGNATURE_LIST *)Buffer.data ();

  List->SignatureHeaderSize = List->SignatureListSize;  // leaves no room

  EXPECT_FALSE (SigListIterInit (&Iter, List));
  EXPECT_EQ (Iter.Remaining, (UINTN)0);
  EXPECT_EQ (SigListIterNext (&Iter), nullptr);
}

// A payload that does not divide evenly into whole entries: the trailing
// partial entry is dropped and only the whole entries are iterated.
TEST (SigListIterInitTest, PayloadNotMultipleOfSignatureSize_TruncatesPartialEntry) {
  std::vector<UINT8>  Buffer;
  SIG_LIST_ITER       Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 2);
  // Trim one byte off SignatureListSize so payload no longer divides
  // evenly into SignatureSize chunks.
  ((EFI_SIGNATURE_LIST *)Buffer.data ())->SignatureListSize -= 1;

  EXPECT_FALSE (SigListIterInit (&Iter, (EFI_SIGNATURE_LIST *)Buffer.data ()));
  // The one whole entry preceding the partial tail is still iterable.
  EXPECT_EQ (Iter.Remaining, (UINTN)1);
  EXPECT_NE (SigListIterNext (&Iter), nullptr);
  EXPECT_EQ (SigListIterNext (&Iter), nullptr);
}

TEST (SigListIterInitTest, WellFormed_NotTruncated) {
  std::vector<UINT8>  Buffer;
  SIG_LIST_ITER       Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 3);

  EXPECT_TRUE (SigListIterInit (&Iter, (EFI_SIGNATURE_LIST *)Buffer.data ()));
  EXPECT_EQ (Iter.Stride, (UINTN)kSha256EntrySize);
  EXPECT_EQ (Iter.Remaining, (UINTN)3);
}

// ---------------------------------------------------------------------------
// SigListIterNext
// ---------------------------------------------------------------------------

TEST (SigListIterNextTest, NullIter_ReturnsNull) {
  EXPECT_EQ (SigListIterNext (NULL), nullptr);
}

TEST (SigListIterNextTest, EmptyList_ReturnsNull) {
  std::vector<UINT8>  Buffer;
  SIG_LIST_ITER       Iter;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, 0);
  ASSERT_TRUE (SigListIterInit (&Iter, (EFI_SIGNATURE_LIST *)Buffer.data ()));

  EXPECT_EQ (SigListIterNext (&Iter), nullptr);
}

TEST (SigListIterNextTest, IteratesEntriesWithCorrectStride) {
  std::vector<UINT8>  Buffer;
  SIG_LIST_ITER       Iter;
  const UINT32        EntryCount = 4;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, 0, kSha256EntrySize, EntryCount);

  EFI_SIGNATURE_LIST  *List       = (EFI_SIGNATURE_LIST *)Buffer.data ();
  UINT8               *FirstEntry =
    (UINT8 *)List + sizeof (EFI_SIGNATURE_LIST) + List->SignatureHeaderSize;

  ASSERT_TRUE (SigListIterInit (&Iter, List));

  for (UINT32 i = 0; i < EntryCount; i++) {
    CONST EFI_SIGNATURE_DATA  *Entry = SigListIterNext (&Iter);
    ASSERT_NE (Entry, nullptr) << "entry " << i;
    EXPECT_EQ ((CONST UINT8 *)Entry, FirstEntry + (size_t)i * kSha256EntrySize);
  }

  EXPECT_EQ (SigListIterNext (&Iter), nullptr);
  EXPECT_EQ (SigListIterNext (&Iter), nullptr);
}

TEST (SigListIterNextTest, RespectsSignatureHeaderSize) {
  std::vector<UINT8>  Buffer;
  SIG_LIST_ITER       Iter;
  const UINT32        HeaderSize = 12;

  AppendSignatureList (Buffer, gEfiCertSha256Guid, HeaderSize, kSha256EntrySize, 2);

  EFI_SIGNATURE_LIST  *List = (EFI_SIGNATURE_LIST *)Buffer.data ();

  ASSERT_TRUE (SigListIterInit (&Iter, List));

  CONST EFI_SIGNATURE_DATA  *Entry = SigListIterNext (&Iter);

  ASSERT_NE (Entry, nullptr);
  EXPECT_EQ (
    (CONST UINT8 *)Entry,
    (CONST UINT8 *)List + sizeof (EFI_SIGNATURE_LIST) + HeaderSize
    );
}

// ---------------------------------------------------------------------------
// WinCertIter helpers
// ---------------------------------------------------------------------------

//
// Build a synthetic PE/COFF "file" whose security directory immediately
// follows a header area. The returned dir VA/Size point into FileBuffer.
//
struct SyntheticImage {
  std::vector<UINT8>          FileBuffer;
  EFI_IMAGE_DATA_DIRECTORY    Dir;
};

//
// Append a single WIN_CERTIFICATE entry of total length dwLength
// (including the header) to Dir. dwLength is written as-is, so callers
// may inject malformed values for negative tests.
//
static void
AppendWinCert (
  std::vector<UINT8>  &Dir,
  UINT32              dwLength,
  UINT16              wRevision,
  UINT16              wCertificateType
  )
{
  const UINT32  Padded = (UINT32)ALIGN_VALUE (dwLength, 8);
  const size_t  Offset = Dir.size ();

  // Reserve the padded size so the next entry starts on an 8-byte boundary.
  Dir.resize (Offset + Padded, 0);

  WIN_CERTIFICATE  *Cert = (WIN_CERTIFICATE *)(Dir.data () + Offset);

  Cert->dwLength         = dwLength;
  Cert->wRevision        = wRevision;
  Cert->wCertificateType = wCertificateType;
}

static SyntheticImage
BuildImageWithDir (
  const std::vector<UINT8>  &DirContents
  )
{
  SyntheticImage  Img;

  // 64 bytes of leading "header" so the dir VA is non-zero.
  Img.FileBuffer.assign (64, 0);
  Img.Dir.VirtualAddress = (UINT32)Img.FileBuffer.size ();
  Img.Dir.Size           = (UINT32)DirContents.size ();
  Img.FileBuffer.insert (
                   Img.FileBuffer.end (),
                   DirContents.begin (),
                   DirContents.end ()
                   );

  return Img;
}

// ---------------------------------------------------------------------------
// WinCertIterInit
// ---------------------------------------------------------------------------

TEST (WinCertIterInitTest, NullParams_ReturnTruncated) {
  WIN_CERT_ITER             Iter;
  std::vector<UINT8>        File (64, 0);
  EFI_IMAGE_DATA_DIRECTORY  Dir = { 0, 0 };

  EXPECT_FALSE (WinCertIterInit (NULL, File.data (), File.size (), &Dir));
  EXPECT_FALSE (WinCertIterInit (&Iter, NULL, File.size (), &Dir));
  EXPECT_FALSE (WinCertIterInit (&Iter, File.data (), File.size (), NULL));
}

TEST (WinCertIterInitTest, EmptyDirectory_NotTruncated) {
  WIN_CERT_ITER             Iter;
  std::vector<UINT8>        File (64, 0);
  EFI_IMAGE_DATA_DIRECTORY  Dir = { 0, 0 };

  EXPECT_TRUE (WinCertIterInit (&Iter, File.data (), File.size (), &Dir));
  EXPECT_EQ (WinCertIterNext (&Iter), nullptr);
}

TEST (WinCertIterInitTest, DirVirtualAddressPastEnd_TruncatesToEmpty) {
  WIN_CERT_ITER             Iter;
  std::vector<UINT8>        File (64, 0);
  EFI_IMAGE_DATA_DIRECTORY  Dir;

  Dir.VirtualAddress = (UINT32)(File.size () + 1);
  Dir.Size           = 0;

  EXPECT_FALSE (WinCertIterInit (&Iter, File.data (), File.size (), &Dir));
  EXPECT_EQ (WinCertIterNext (&Iter), nullptr);
}

TEST (WinCertIterInitTest, DirSizeOverrunsFile_TruncatesToEmpty) {
  WIN_CERT_ITER             Iter;
  std::vector<UINT8>        File (128, 0);
  EFI_IMAGE_DATA_DIRECTORY  Dir;

  Dir.VirtualAddress = 64;
  Dir.Size           = (UINT32)(File.size () - 64 + 1);

  EXPECT_FALSE (WinCertIterInit (&Iter, File.data (), File.size (), &Dir));
  EXPECT_EQ (WinCertIterNext (&Iter), nullptr);
}

TEST (WinCertIterInitTest, EntryDwLengthBelowHeader_TruncatesToEmpty) {
  std::vector<UINT8>  Dir;

  AppendWinCert (Dir, sizeof (WIN_CERTIFICATE), 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA);
  // Corrupt dwLength after the fact.
  ((WIN_CERTIFICATE *)Dir.data ())->dwLength = sizeof (WIN_CERTIFICATE) - 1;

  SyntheticImage  Img = BuildImageWithDir (Dir);
  WIN_CERT_ITER   Iter;

  EXPECT_FALSE (WinCertIterInit (&Iter, Img.FileBuffer.data (), Img.FileBuffer.size (), &Img.Dir));
  EXPECT_EQ (WinCertIterNext (&Iter), nullptr);
}

TEST (WinCertIterInitTest, EntryDwLengthOverrunsRemaining_TruncatesToEmpty) {
  std::vector<UINT8>  Dir;

  AppendWinCert (Dir, sizeof (WIN_CERTIFICATE) + 8, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA);
  ((WIN_CERTIFICATE *)Dir.data ())->dwLength = (UINT32)Dir.size () + 1;

  SyntheticImage  Img = BuildImageWithDir (Dir);
  WIN_CERT_ITER   Iter;

  EXPECT_FALSE (WinCertIterInit (&Iter, Img.FileBuffer.data (), Img.FileBuffer.size (), &Img.Dir));
  EXPECT_EQ (WinCertIterNext (&Iter), nullptr);
}

TEST (WinCertIterInitTest, WellFormedEntries_NotTruncated) {
  std::vector<UINT8>  Dir;

  AppendWinCert (Dir, sizeof (WIN_CERTIFICATE) + 16, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA);
  AppendWinCert (Dir, sizeof (WIN_CERTIFICATE) + 32, 0x0200, WIN_CERT_TYPE_EFI_GUID);

  SyntheticImage  Img = BuildImageWithDir (Dir);
  WIN_CERT_ITER   Iter;

  EXPECT_TRUE (WinCertIterInit (&Iter, Img.FileBuffer.data (), Img.FileBuffer.size (), &Img.Dir));
}

// A well-formed entry followed by one with a malformed dwLength: the range is
// clamped to the valid entry and the malformed tail is dropped.
TEST (WinCertIterInitTest, MalformedSecondEntry_TruncatesToValidPrefix) {
  std::vector<UINT8>  Dir;

  AppendWinCert (Dir, sizeof (WIN_CERTIFICATE) + 16, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA);
  const size_t  SecondOffset = Dir.size ();

  AppendWinCert (Dir, sizeof (WIN_CERTIFICATE) + 16, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA);
  // Corrupt the second entry's dwLength so it cannot be parsed.
  ((WIN_CERTIFICATE *)(Dir.data () + SecondOffset))->dwLength = sizeof (WIN_CERTIFICATE) - 1;

  SyntheticImage  Img   = BuildImageWithDir (Dir);
  CONST UINT8     *Base = Img.FileBuffer.data () + Img.Dir.VirtualAddress;
  WIN_CERT_ITER   Iter;

  EXPECT_FALSE (WinCertIterInit (&Iter, Img.FileBuffer.data (), Img.FileBuffer.size (), &Img.Dir));
  // The first, well-formed entry is still iterable; the second is dropped.
  EXPECT_EQ ((CONST UINT8 *)WinCertIterNext (&Iter), Base);
  EXPECT_EQ (WinCertIterNext (&Iter), nullptr);
}

//
// A directory smaller than a WIN_CERTIFICATE header: the walk sees a
// non-zero Remaining that cannot hold another header, so the range is
// clamped to empty.
//
TEST (WinCertIterInitTest, TrailingBytesBelowHeader_TruncatesToEmpty) {
  std::vector<UINT8>  Dir (sizeof (WIN_CERTIFICATE) - 1, 0);

  SyntheticImage  Img = BuildImageWithDir (Dir);
  WIN_CERT_ITER   Iter;

  EXPECT_FALSE (WinCertIterInit (&Iter, Img.FileBuffer.data (), Img.FileBuffer.size (), &Img.Dir));
  EXPECT_EQ (WinCertIterNext (&Iter), nullptr);
}

// ---------------------------------------------------------------------------
// WinCertIterNext
// ---------------------------------------------------------------------------

TEST (WinCertIterNextTest, NullIter_ReturnsNull) {
  EXPECT_EQ (WinCertIterNext (NULL), nullptr);
}

TEST (WinCertIterNextTest, IteratesAllEntriesWithAlignment) {
  std::vector<UINT8>  Dir;

  // Three entries with different dwLength values (some not multiples of 8)
  // to exercise the ALIGN_VALUE advance.
  const UINT32  Len1 = sizeof (WIN_CERTIFICATE) + 5;   // 13 -> aligned to 16
  const UINT32  Len2 = sizeof (WIN_CERTIFICATE) + 16;  // 24
  const UINT32  Len3 = sizeof (WIN_CERTIFICATE) + 1;   // 9  -> aligned to 16

  AppendWinCert (Dir, Len1, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA);
  AppendWinCert (Dir, Len2, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA);
  AppendWinCert (Dir, Len3, 0x0200, WIN_CERT_TYPE_EFI_GUID);

  SyntheticImage  Img = BuildImageWithDir (Dir);
  WIN_CERT_ITER   Iter;

  ASSERT_TRUE (
    WinCertIterInit (&Iter, Img.FileBuffer.data (), Img.FileBuffer.size (), &Img.Dir)
    );

  CONST UINT8            *Base = Img.FileBuffer.data () + Img.Dir.VirtualAddress;
  CONST WIN_CERTIFICATE  *C1   = WinCertIterNext (&Iter);
  CONST WIN_CERTIFICATE  *C2   = WinCertIterNext (&Iter);
  CONST WIN_CERTIFICATE  *C3   = WinCertIterNext (&Iter);
  CONST WIN_CERTIFICATE  *C4   = WinCertIterNext (&Iter);

  ASSERT_NE (C1, nullptr);
  ASSERT_NE (C2, nullptr);
  ASSERT_NE (C3, nullptr);
  EXPECT_EQ ((CONST UINT8 *)C1, Base);
  EXPECT_EQ ((CONST UINT8 *)C2, Base + ALIGN_VALUE (Len1, 8));
  EXPECT_EQ ((CONST UINT8 *)C3, Base + ALIGN_VALUE (Len1, 8) + ALIGN_VALUE (Len2, 8));
  EXPECT_EQ (C1->dwLength, Len1);
  EXPECT_EQ (C2->dwLength, Len2);
  EXPECT_EQ (C3->dwLength, Len3);
  EXPECT_EQ (C4, nullptr);

  // Drained iterator stays drained.
  EXPECT_EQ (WinCertIterNext (&Iter), nullptr);
}

//
// A single entry whose dwLength consumes the entire (unpadded) directory:
// ALIGN_VALUE(dwLength, 8) exceeds the remaining bytes, exercising the
// clamp in both WinCertIterInit and WinCertIterNext.
//
TEST (WinCertIterNextTest, LastEntryUnpaddedClampsToRemaining) {
  const UINT32        Len = sizeof (WIN_CERTIFICATE) + 4;   // 12; ALIGN(12,8)=16 > 12
  std::vector<UINT8>  Dir (Len, 0);

  WIN_CERTIFICATE  *Cert = (WIN_CERTIFICATE *)Dir.data ();

  Cert->dwLength         = Len;
  Cert->wRevision        = 0x0200;
  Cert->wCertificateType = WIN_CERT_TYPE_PKCS_SIGNED_DATA;

  SyntheticImage  Img = BuildImageWithDir (Dir);
  WIN_CERT_ITER   Iter;

  ASSERT_TRUE (
    WinCertIterInit (&Iter, Img.FileBuffer.data (), Img.FileBuffer.size (), &Img.Dir)
    );

  CONST WIN_CERTIFICATE  *C = WinCertIterNext (&Iter);

  ASSERT_NE (C, nullptr);
  EXPECT_EQ (C->dwLength, Len);
  EXPECT_EQ (WinCertIterNext (&Iter), nullptr);
}
