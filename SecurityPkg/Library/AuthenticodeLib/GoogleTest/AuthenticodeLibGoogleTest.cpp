/** @file
  Unit tests for Authenticode image assembly and certificate-table lookup.

  These tests construct minimal-but-valid PE/COFF images in memory and
  exercise the real PeCoffLib through GetWinCertificates so that the
  embedded WIN_CERTIFICATE table lookup path is covered end-to-end.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

// MU_CHANGE - NEW FILE

#include <Library/GoogleTestLib.h>

#include <cstring>
#include <vector>
#include <tuple>

extern "C" {
  #include <Uefi.h>
  #include <IndustryStandard/PeImage.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/AuthenticodeLib.h>
  #include <Library/MemoryAllocationLib.h>
}

//
// Layout constants for the synthetic PE32+ image.
//
// The buffer is a single contiguous blob laid out as:
//   [DOS hdr][NT64 hdrs][1 section hdr][padding to SizeOfHeaders][image body]
//
// All sizes are picked to satisfy PeCoffLoaderGetImageInfo's
// consistency checks (NumberOfRvaAndSizes, SizeOfOptionalHeader,
// SectionHeaderOffset, SizeOfHeaders < SizeOfImage, last-byte reads of
// SizeOfHeaders and the security directory).
//
static constexpr UINT32  kDosHdrSize    = sizeof (EFI_IMAGE_DOS_HEADER);
static constexpr UINT32  kPeCoffOffset  = kDosHdrSize;
static constexpr UINT32  kSizeOfHeaders = 0x200;
static constexpr UINT32  kSizeOfImage   = 0x400;
static constexpr UINT32  kSecDirVa      = 0x300;
static constexpr UINT32  kSecDirSize    = 0x100;

/**
  Build a minimal valid PE32+ image in heap-backed storage.
  When IncludeSecurityDir is true, the returned image's
  EFI_IMAGE_DIRECTORY_ENTRY_SECURITY entry points at [kSecDirVa,
  kSecDirVa + kSecDirSize). Otherwise that entry is left zero.
**/
static std::vector<UINT8>
BuildPe32PlusImage (
  bool  IncludeSecurityDir
  )
{
  std::vector<UINT8>  Image (kSizeOfImage, 0);

  EFI_IMAGE_DOS_HEADER    *Dos = (EFI_IMAGE_DOS_HEADER *)Image.data ();
  EFI_IMAGE_NT_HEADERS64  *Nt  = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

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

  if (IncludeSecurityDir) {
    Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = kSecDirVa;
    Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size           = kSecDirSize;
  }

  return Image;
}

class GetWinCertificatesTest : public ::testing::Test {
};

// ---------------------------------------------------------------------------
// Argument validation
// ---------------------------------------------------------------------------

TEST_F (GetWinCertificatesTest, NullFileBuffer_ReturnsInvalidParameter) {
  CONST UINT8  *WinCertificates;
  UINTN        Length;

  EXPECT_EQ (
    GetWinCertificates (NULL, 0x100, &WinCertificates, &Length),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (GetWinCertificatesTest, NullWinCertificates_ReturnsInvalidParameter) {
  std::vector<UINT8>  Image = BuildPe32PlusImage (false);
  UINTN               Length;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), NULL, &Length),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (GetWinCertificatesTest, NullLength_ReturnsInvalidParameter) {
  std::vector<UINT8>  Image = BuildPe32PlusImage (false);
  CONST UINT8         *WinCertificates;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (GetWinCertificatesTest, NullFileBuffer_DoesNotTouchOutputs) {
  //
  // Caller-supplied outputs must be untouched on the early
  // invalid-parameter return; only the parsing path is allowed to
  // overwrite them.
  //
  CONST UINT8  *WinCertificates;
  CONST UINT8  *ExpectedWinCertificates;
  UINTN        Length;
  UINTN        ExpectedLength;

  SetMem (&WinCertificates, sizeof (WinCertificates), 0xAA);
  SetMem (&ExpectedWinCertificates, sizeof (ExpectedWinCertificates), 0xAA);
  SetMem (&Length, sizeof (Length), 0xAA);
  SetMem (&ExpectedLength, sizeof (ExpectedLength), 0xAA);

  EXPECT_EQ (
    GetWinCertificates (NULL, 0x100, &WinCertificates, &Length),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (WinCertificates, ExpectedWinCertificates);
  EXPECT_EQ (Length, ExpectedLength);
}

// ---------------------------------------------------------------------------
// Malformed images
// ---------------------------------------------------------------------------

TEST_F (GetWinCertificatesTest, GarbageBuffer_ReturnsVolumeCorrupted) {
  //
  // A buffer that is large enough to attempt header parsing but does
  // not actually carry valid DOS/NT signatures must be rejected.
  //
  std::vector<UINT8>  Buffer (kSizeOfImage, 0xAB);
  CONST UINT8         *WinCertificates;
  UINTN               Length;

  EXPECT_EQ (
    GetWinCertificates (Buffer.data (), Buffer.size (), &WinCertificates, &Length),
    EFI_VOLUME_CORRUPTED
    );
}

TEST_F (GetWinCertificatesTest, TinyBuffer_ReturnsVolumeCorrupted) {
  //
  // A buffer too small to contain a DOS header must be rejected
  // without dereferencing any of the bytes.
  //
  UINT8        TinyBuffer[4] = { 0 };
  CONST UINT8  *WinCertificates;
  UINTN        Length;

  EXPECT_EQ (
    GetWinCertificates (TinyBuffer, sizeof (TinyBuffer), &WinCertificates, &Length),
    EFI_VOLUME_CORRUPTED
    );
}

TEST_F (GetWinCertificatesTest, ZeroFileSize_ReturnsVolumeCorrupted) {
  UINT8        Byte = 0;
  CONST UINT8  *WinCertificates;
  UINTN        Length;

  EXPECT_EQ (
    GetWinCertificates (&Byte, 0, &WinCertificates, &Length),
    EFI_VOLUME_CORRUPTED
    );
}

TEST_F (GetWinCertificatesTest, ElfanewPastEof_ReturnsVolumeCorrupted) {
  //
  // Valid DOS signature but e_lfanew points well past the end of the
  // buffer. The bounded reader must refuse to fetch the NT headers.
  //
  std::vector<UINT8>    Image = BuildPe32PlusImage (false);
  EFI_IMAGE_DOS_HEADER  *Dos  = (EFI_IMAGE_DOS_HEADER *)Image.data ();

  Dos->e_lfanew = (UINT32)(Image.size () + 0x1000);

  CONST UINT8  *WinCertificates;
  UINTN        Length;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, &Length),
    EFI_VOLUME_CORRUPTED
    );
}

TEST_F (GetWinCertificatesTest, TruncatedAtNtHeaders_ReturnsVolumeCorrupted) {
  //
  // Buffer is truncated to the DOS header only, so the read of the NT
  // headers at e_lfanew straddles EOF.
  //
  std::vector<UINT8>  Image = BuildPe32PlusImage (false);

  Image.resize (sizeof (EFI_IMAGE_DOS_HEADER));

  CONST UINT8  *WinCertificates;
  UINTN        Length;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, &Length),
    EFI_VOLUME_CORRUPTED
    );
}

TEST_F (GetWinCertificatesTest, ValidDosBadNtSignature_ReturnsVolumeCorrupted) {
  //
  // Valid DOS header but corrupted NT signature -- exercises the NT
  // signature validation path inside PeCoffLib.
  //
  std::vector<UINT8>      Image = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->Signature = 0xDEADBEEF;

  CONST UINT8  *WinCertificates;
  UINTN        Length;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, &Length),
    EFI_VOLUME_CORRUPTED
    );
}

// ---------------------------------------------------------------------------
// Happy paths exercised through real PeCoffLib security-directory capture
// ---------------------------------------------------------------------------

TEST_F (GetWinCertificatesTest, ValidImageWithoutSecurityDir_ReturnsNoCertificates) {
  std::vector<UINT8>  Image = BuildPe32PlusImage (false);
  CONST UINT8         *WinCertificates;
  UINTN               Length;

  //
  // Pre-poison the outputs so we can prove the function actually wrote
  // back the "no certificates" result (rather than leaving stale
  // caller-supplied bytes).
  //
  SetMem (&WinCertificates, sizeof (WinCertificates), 0xCD);
  SetMem (&Length, sizeof (Length), 0xCD);

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, &Length),
    EFI_SUCCESS
    );
  EXPECT_EQ (WinCertificates, (CONST UINT8 *)NULL);
  EXPECT_EQ (Length, (UINTN)0);
}

TEST_F (GetWinCertificatesTest, ValidImageWithSecurityDir_ReturnsCertificates) {
  std::vector<UINT8>  Image = BuildPe32PlusImage (true);
  CONST UINT8         *WinCertificates;
  UINTN               Length;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, &Length),
    EFI_SUCCESS
    );
  EXPECT_EQ (WinCertificates, Image.data () + kSecDirVa);
  EXPECT_EQ (Length, (UINTN)kSecDirSize);
}

TEST_F (GetWinCertificatesTest, OtherDataDirectoriesDoNotLeak) {
  //
  // Populate a non-SECURITY data directory entry (export table) with
  // a recognizable value to prove PeCoffLib only records the SECURITY
  // entry and ignores every other directory it walks.
  //
  std::vector<UINT8>      Image = BuildPe32PlusImage (true);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = 0xCAFEBABE;
  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].Size           = 0xBEEFu;

  CONST UINT8  *WinCertificates;
  UINTN        Length;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, &Length),
    EFI_SUCCESS
    );
  EXPECT_EQ (WinCertificates, Image.data () + kSecDirVa);
  EXPECT_EQ (Length, (UINTN)kSecDirSize);
}

class BuildAuthenticodeImageTest : public ::testing::TestWithParam<std::tuple<bool, bool> > {
};

TEST_P (BuildAuthenticodeImageTest, AssemblesHeadersSortedSectionsAndTrailingData) {
  const bool          Pe32   = std::get<0> (GetParam ());
  const bool          Signed = std::get<1> (GetParam ());
  std::vector<UINT8>  Image  = BuildPe32PlusImage (Signed);
  UINTN               ChecksumOffset;
  UINTN               SecurityDirectoryOffset;
  UINTN               SectionTableOffset;

  if (Pe32) {
    EFI_IMAGE_NT_HEADERS32  *Nt = (EFI_IMAGE_NT_HEADERS32 *)(Image.data () + kPeCoffOffset);

    ZeroMem (Nt, sizeof (*Nt));
    Nt->Signature                          = EFI_IMAGE_NT_SIGNATURE;
    Nt->FileHeader.Machine                 = IMAGE_FILE_MACHINE_I386;
    Nt->FileHeader.NumberOfSections        = 2;
    Nt->FileHeader.SizeOfOptionalHeader    = sizeof (EFI_IMAGE_OPTIONAL_HEADER32);
    Nt->OptionalHeader.Magic               = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    Nt->OptionalHeader.SizeOfImage         = kSizeOfImage;
    Nt->OptionalHeader.SizeOfHeaders       = kSizeOfHeaders;
    Nt->OptionalHeader.SectionAlignment    = 0x200;
    Nt->OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;
    Nt->OptionalHeader.CheckSum            = 0xAABBCCDD;
    if (Signed) {
      Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = kSecDirVa;
      Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size           = kSecDirSize;
    }

    ChecksumOffset          = (UINT8 *)&Nt->OptionalHeader.CheckSum - Image.data ();
    SecurityDirectoryOffset = (UINT8 *)&Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY] - Image.data ();
    SectionTableOffset      = kPeCoffOffset + sizeof (*Nt);
  } else {
    EFI_IMAGE_NT_HEADERS64  *Nt = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

    Nt->FileHeader.NumberOfSections = 2;
    Nt->OptionalHeader.CheckSum     = 0xAABBCCDD;
    ChecksumOffset                  = (UINT8 *)&Nt->OptionalHeader.CheckSum - Image.data ();
    SecurityDirectoryOffset         = (UINT8 *)&Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY] - Image.data ();
    SectionTableOffset              = kPeCoffOffset + sizeof (*Nt);
  }

  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + SectionTableOffset);

  Sections[0].PointerToRawData = 0x280;
  Sections[0].SizeOfRawData    = 0x80;
  Sections[0].VirtualAddress   = 0x200;
  Sections[1].PointerToRawData = 0x200;
  Sections[1].SizeOfRawData    = 0x80;
  Sections[1].VirtualAddress   = 0x300;
  for (UINTN Index = kSizeOfHeaders; Index < Image.size (); Index++) {
    Image[Index] = (UINT8)(Index * 7 + 3);
  }

  std::vector<UINT8>  Expected;

  Expected.insert (Expected.end (), Image.begin (), Image.begin () + ChecksumOffset);
  Expected.insert (Expected.end (), Image.begin () + ChecksumOffset + sizeof (UINT32), Image.begin () + SecurityDirectoryOffset);
  Expected.insert (Expected.end (), Image.begin () + SecurityDirectoryOffset + sizeof (EFI_IMAGE_DATA_DIRECTORY), Image.begin () + kSizeOfHeaders);
  Expected.insert (Expected.end (), Image.begin () + 0x200, Image.begin () + 0x280);
  Expected.insert (Expected.end (), Image.begin () + 0x280, Image.begin () + 0x300);
  if (!Signed) {
    Expected.insert (Expected.end (), Image.begin () + 0x300, Image.end ());
  }

  UINT8  *AuthImage    = NULL;
  UINTN  AuthImageSize = 0;

  ASSERT_EQ (BuildAuthenticodeImage (Image.data (), Image.size (), &AuthImage, &AuthImageSize), EFI_SUCCESS);
  ASSERT_NE (AuthImage, nullptr);
  EXPECT_EQ (AuthImageSize, Expected.size ());
  EXPECT_EQ (std::vector<UINT8> (AuthImage, AuthImage + AuthImageSize), Expected);
  FreePool (AuthImage);
}

INSTANTIATE_TEST_SUITE_P (
  Pe32AndPe32Plus,
  BuildAuthenticodeImageTest,
  ::testing::Combine (::testing::Bool (), ::testing::Bool ())
  );

TEST (BuildAuthenticodeImageErrorTest, InvalidArgumentsLeaveOutputsUntouched) {
  std::vector<UINT8>  Image         = BuildPe32PlusImage (false);
  UINT8               *AuthImage    = Image.data ();
  UINTN               AuthImageSize = 123;

  EXPECT_EQ (BuildAuthenticodeImage (NULL, Image.size (), &AuthImage, &AuthImageSize), EFI_INVALID_PARAMETER);
  EXPECT_EQ (BuildAuthenticodeImage (Image.data (), 0, &AuthImage, &AuthImageSize), EFI_INVALID_PARAMETER);
  EXPECT_EQ (BuildAuthenticodeImage (Image.data (), Image.size (), NULL, &AuthImageSize), EFI_INVALID_PARAMETER);
  EXPECT_EQ (BuildAuthenticodeImage (Image.data (), Image.size (), &AuthImage, NULL), EFI_INVALID_PARAMETER);
  EXPECT_EQ (AuthImage, Image.data ());
  EXPECT_EQ (AuthImageSize, (UINTN)123);
}

TEST (BuildAuthenticodeImageErrorTest, TruncatedImageClearsOutputs) {
  UINT8  Image[4]      = { 0 };
  UINT8  *AuthImage    = Image;
  UINTN  AuthImageSize = sizeof (Image);

  EXPECT_EQ (BuildAuthenticodeImage (Image, sizeof (Image), &AuthImage, &AuthImageSize), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (AuthImage, nullptr);
  EXPECT_EQ (AuthImageSize, (UINTN)0);
}

TEST (BuildAuthenticodeImageTest, NoSecurityDirectoryOrSectionsOmitsOnlyChecksum) {
  std::vector<UINT8>      Image = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->FileHeader.NumberOfSections        = 0;
  Nt->OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_DIRECTORY_ENTRY_SECURITY;
  Nt->FileHeader.SizeOfOptionalHeader   -= (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES - EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) * sizeof (EFI_IMAGE_DATA_DIRECTORY);

  const UINTN         ChecksumOffset = (UINT8 *)&Nt->OptionalHeader.CheckSum - Image.data ();
  std::vector<UINT8>  Expected (Image.begin (), Image.begin () + ChecksumOffset);

  Expected.insert (Expected.end (), Image.begin () + ChecksumOffset + sizeof (UINT32), Image.end ());

  UINT8  *AuthImage    = NULL;
  UINTN  AuthImageSize = 0;

  ASSERT_EQ (BuildAuthenticodeImage (Image.data (), Image.size (), &AuthImage, &AuthImageSize), EFI_SUCCESS);
  ASSERT_NE (AuthImage, nullptr);
  EXPECT_EQ (AuthImageSize, Expected.size ());
  EXPECT_EQ (std::vector<UINT8> (AuthImage, AuthImage + AuthImageSize), Expected);
  FreePool (AuthImage);
}

TEST (BuildAuthenticodeImageErrorTest, OverlappingSectionsCannotOverflowOutput) {
  std::vector<UINT8>        Image     = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64    *Nt       = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + kPeCoffOffset + sizeof (*Nt));

  Nt->FileHeader.NumberOfSections = 2;
  for (UINTN Index = 0; Index < 2; Index++) {
    Sections[Index].VirtualAddress   = kSizeOfHeaders;
    Sections[Index].PointerToRawData = kSizeOfHeaders;
    Sections[Index].SizeOfRawData    = kSizeOfImage - kSizeOfHeaders;
  }

  UINT8  *AuthImage    = Image.data ();
  UINTN  AuthImageSize = 123;

  EXPECT_EQ (BuildAuthenticodeImage (Image.data (), Image.size (), &AuthImage, &AuthImageSize), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (AuthImage, nullptr);
  EXPECT_EQ (AuthImageSize, (UINTN)0);
}

static std::vector<UINT8>
ExpectedLinearPeImage (
  const std::vector<UINT8>  &Image,
  UINTN                     PeOffset,
  UINTN                     DataEnd,
  bool                      Pe32 = false
  )
{
  const UINTN  Checksum = PeOffset + (Pe32
                                     ? OFFSET_OF (EFI_IMAGE_NT_HEADERS32, OptionalHeader.CheckSum)
                                     : OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader.CheckSum));
  const UINTN  Directory = PeOffset + (Pe32
                                      ? OFFSET_OF (EFI_IMAGE_NT_HEADERS32, OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY])
                                      : OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]));

  std::vector<UINT8>  Expected (Image.begin (), Image.begin () + Checksum);

  Expected.insert (Expected.end (), Image.begin () + Checksum + sizeof (UINT32), Image.begin () + Directory);
  Expected.insert (Expected.end (), Image.begin () + Directory + sizeof (EFI_IMAGE_DATA_DIRECTORY), Image.begin () + DataEnd);
  return Expected;
}

static void
ExpectAuthenticodeImage (
  const UINT8               *Image,
  UINTN                     ImageSize,
  const std::vector<UINT8>  &Expected
  )
{
  UINT8  *AuthImage    = NULL;
  UINTN  AuthImageSize = 0;

  ASSERT_EQ (BuildAuthenticodeImage (Image, ImageSize, &AuthImage, &AuthImageSize), EFI_SUCCESS);
  ASSERT_NE (AuthImage, nullptr);
  EXPECT_EQ (AuthImageSize, Expected.size ());
  if (AuthImageSize == Expected.size ()) {
    EXPECT_EQ (std::vector<UINT8> (AuthImage, AuthImage + AuthImageSize), Expected);
  }

  FreePool (AuthImage);
}

static void
ExpectCorruptedImage (
  const std::vector<UINT8>  &Image
  )
{
  UINT8  *AuthImage    = (UINT8 *)(UINTN)1;
  UINTN  AuthImageSize = 123;

  EXPECT_EQ (BuildAuthenticodeImage (Image.data (), Image.size (), &AuthImage, &AuthImageSize), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (AuthImage, nullptr);
  EXPECT_EQ (AuthImageSize, (UINTN)0);
}

static void
ExpectInvalidCertificateTable (
  const std::vector<UINT8>  &Image
  )
{
  CONST UINT8  *Certificates = (CONST UINT8 *)(UINTN)1;
  UINTN        Length        = 123;

  EXPECT_EQ (GetWinCertificates (Image.data (), Image.size (), &Certificates, &Length), EFI_VOLUME_CORRUPTED);
  EXPECT_EQ (Certificates, nullptr);
  EXPECT_EQ (Length, (UINTN)0);
}

static std::vector<UINT8>
BuildGappedPeImage (
  bool  Pe32,
  bool  Signed
  )
{
  std::vector<UINT8>  Image = BuildPe32PlusImage (false);

  Image.resize (Signed ? 0xA80 : 0xA00, 0);
  UINTN  SectionTableOffset;

  if (Pe32) {
    EFI_IMAGE_NT_HEADERS32  *Nt = (EFI_IMAGE_NT_HEADERS32 *)(Image.data () + kPeCoffOffset);

    ZeroMem (Nt, sizeof (*Nt));
    Nt->Signature                          = EFI_IMAGE_NT_SIGNATURE;
    Nt->FileHeader.Machine                 = IMAGE_FILE_MACHINE_I386;
    Nt->FileHeader.NumberOfSections        = 2;
    Nt->FileHeader.SizeOfOptionalHeader    = sizeof (EFI_IMAGE_OPTIONAL_HEADER32);
    Nt->OptionalHeader.Magic               = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    Nt->OptionalHeader.SizeOfImage         = 0x2000;
    Nt->OptionalHeader.SizeOfHeaders       = kSizeOfHeaders;
    Nt->OptionalHeader.SectionAlignment    = 0x200;
    Nt->OptionalHeader.FileAlignment       = 0x200;
    Nt->OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;
    if (Signed) {
      Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0xA00;
      Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size           = 0x80;
    }

    SectionTableOffset = kPeCoffOffset + sizeof (*Nt);
  } else {
    EFI_IMAGE_NT_HEADERS64  *Nt = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

    Nt->FileHeader.NumberOfSections  = 2;
    Nt->OptionalHeader.SizeOfImage   = 0x2000;
    Nt->OptionalHeader.FileAlignment = 0x200;
    if (Signed) {
      Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0xA00;
      Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size           = 0x80;
    }

    SectionTableOffset = kPeCoffOffset + sizeof (*Nt);
  }

  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + SectionTableOffset);

  Sections[0].VirtualAddress   = 0x400;
  Sections[0].PointerToRawData = 0x600;
  Sections[0].SizeOfRawData    = 0x200;
  Sections[1].VirtualAddress   = 0x200;
  Sections[1].PointerToRawData = 0x200;
  Sections[1].SizeOfRawData    = 0x200;
  for (UINTN Index = kSizeOfHeaders; Index < Image.size (); Index++) {
    Image[Index] = (UINT8)(Index * 29 + (Index >> 8));
  }

  return Image;
}

class GappedImageTest : public ::testing::TestWithParam<std::tuple<bool, bool> > {
};

TEST_P (GappedImageTest, IncludesEachGapAndSectionExactlyOnce) {
  const bool          Pe32    = std::get<0> (GetParam ());
  const bool          Signed  = std::get<1> (GetParam ());
  std::vector<UINT8>  Image   = BuildGappedPeImage (Pe32, Signed);
  const UINTN         DataEnd = Signed ? 0xA00 : Image.size ();

  ExpectAuthenticodeImage (Image.data (), Image.size (), ExpectedLinearPeImage (Image, kPeCoffOffset, DataEnd, Pe32));
  Image[0x450] ^= 0x5A;
  ExpectAuthenticodeImage (Image.data (), Image.size (), ExpectedLinearPeImage (Image, kPeCoffOffset, DataEnd, Pe32));
}

INSTANTIATE_TEST_SUITE_P (Pe32AndPe32Plus, GappedImageTest, ::testing::Combine (::testing::Bool (), ::testing::Bool ()));

class DirectoryCountTest : public ::testing::TestWithParam<UINT32> {
};

TEST_P (DirectoryCountTest, IncludesOnlyDeclaredHeaderFields) {
  std::vector<UINT8>      Image = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  const UINT32            Count = GetParam ();

  Nt->FileHeader.NumberOfSections        = 0;
  Nt->OptionalHeader.NumberOfRvaAndSizes = Count;
  Nt->FileHeader.SizeOfOptionalHeader    = (UINT16)(OFFSET_OF (EFI_IMAGE_OPTIONAL_HEADER64, DataDirectory) + Count * sizeof (EFI_IMAGE_DATA_DIRECTORY));

  std::vector<UINT8>  Expected;

  if (Count > EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
    Expected = ExpectedLinearPeImage (Image, kPeCoffOffset, Image.size ());
  } else {
    const UINTN  Checksum = kPeCoffOffset + OFFSET_OF (EFI_IMAGE_NT_HEADERS64, OptionalHeader.CheckSum);

    Expected.assign (Image.begin (), Image.begin () + Checksum);
    Expected.insert (Expected.end (), Image.begin () + Checksum + sizeof (UINT32), Image.end ());
  }

  ExpectAuthenticodeImage (Image.data (), Image.size (), Expected);
}

INSTANTIATE_TEST_SUITE_P (DirectoryBoundaries, DirectoryCountTest, ::testing::Values (0U, 1U, 4U, 5U, 16U));

TEST (BuildAuthenticodeImageErrorTest, RejectsDirectoryCountBeyondPeLimit) {
  std::vector<UINT8>      Image = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES + 1;
  ExpectCorruptedImage (Image);
}

TEST (BuildAuthenticodeImageErrorTest, RejectsNonTrailingCertificateTable) {
  std::vector<UINT8>      Image = BuildPe32PlusImage (true);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size = 0x80;
  ExpectCorruptedImage (Image);
}

TEST (BuildAuthenticodeImageErrorTest, RejectsCertificateTableInsideHeaders) {
  std::vector<UINT8>      Image = BuildPe32PlusImage (true);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0x180;
  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size           = 0x280;
  ExpectCorruptedImage (Image);
}

TEST (BuildAuthenticodeImageErrorTest, RejectsUnalignedCertificateTable) {
  std::vector<UINT8>      Image = BuildPe32PlusImage (true);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0x301;
  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size           = 0xFF;
  ExpectCorruptedImage (Image);
  ExpectInvalidCertificateTable (Image);
}

TEST (BuildAuthenticodeImageErrorTest, RejectsShortCertificateTable) {
  std::vector<UINT8>  Image = BuildPe32PlusImage (true);

  Image.resize (0x3FC);
  EFI_IMAGE_NT_HEADERS64  *Nt = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0x3F8;
  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size           = 4;
  ExpectCorruptedImage (Image);
  ExpectInvalidCertificateTable (Image);
}

TEST (BuildAuthenticodeImageErrorTest, RejectsSectionOverlappingCertificateTable) {
  std::vector<UINT8>        Image     = BuildPe32PlusImage (true);
  EFI_IMAGE_NT_HEADERS64    *Nt       = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + kPeCoffOffset + sizeof (*Nt));

  Sections[0].VirtualAddress   = kSizeOfHeaders;
  Sections[0].PointerToRawData = 0x280;
  Sections[0].SizeOfRawData    = 0x100;
  ExpectCorruptedImage (Image);
}

TEST (BuildAuthenticodeImageErrorTest, RejectsOverlapEvenWhenOutputWouldFit) {
  std::vector<UINT8>        Image     = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64    *Nt       = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + kPeCoffOffset + sizeof (*Nt));

  Nt->FileHeader.NumberOfSections = 2;
  for (UINTN Index = 0; Index < 2; Index++) {
    Sections[Index].VirtualAddress   = kSizeOfHeaders + (UINT32)Index * 0x100;
    Sections[Index].PointerToRawData = kSizeOfHeaders + (UINT32)Index * 0x40;
    Sections[Index].SizeOfRawData    = 0x80;
  }

  ExpectCorruptedImage (Image);
}

TEST (BuildAuthenticodeImageErrorTest, RejectsDuplicateSectionRanges) {
  std::vector<UINT8>        Image     = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64    *Nt       = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + kPeCoffOffset + sizeof (*Nt));

  Nt->FileHeader.NumberOfSections = 2;
  for (UINTN Index = 0; Index < 2; Index++) {
    Sections[Index].VirtualAddress   = kSizeOfHeaders + (UINT32)Index * 0x100;
    Sections[Index].PointerToRawData = kSizeOfHeaders;
    Sections[Index].SizeOfRawData    = 0x80;
  }

  ExpectCorruptedImage (Image);
}

TEST (BuildAuthenticodeImageErrorTest, RejectsSectionWhoseRawDataExceedsFile) {
  std::vector<UINT8>        Image     = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64    *Nt       = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + kPeCoffOffset + sizeof (*Nt));

  Sections[0].VirtualAddress   = kSizeOfHeaders;
  Sections[0].PointerToRawData = 0x3F0;
  Sections[0].SizeOfRawData    = 0x20;
  ExpectCorruptedImage (Image);
}

TEST (BuildAuthenticodeImageErrorTest, RejectsSectionRawRangeOverflow) {
  std::vector<UINT8>        Image     = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64    *Nt       = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + kPeCoffOffset + sizeof (*Nt));

  Sections[0].VirtualAddress   = kSizeOfHeaders;
  Sections[0].PointerToRawData = MAX_UINT32 - 7;
  Sections[0].SizeOfRawData    = 16;
  ExpectCorruptedImage (Image);
}

TEST (BuildAuthenticodeImageTest, EmptySectionDoesNotReadItsRawOffset) {
  std::vector<UINT8>        Image     = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64    *Nt       = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + kPeCoffOffset + sizeof (*Nt));

  Sections[0].PointerToRawData = MAX_UINT32;
  Sections[0].SizeOfRawData    = 0;
  ExpectAuthenticodeImage (Image.data (), Image.size (), ExpectedLinearPeImage (Image, kPeCoffOffset, Image.size ()));
}

TEST (BuildAuthenticodeImageTest, AcceptsHeadersEndingAtEndOfUnsignedFile) {
  std::vector<UINT8>  Image = BuildPe32PlusImage (false);

  Image.resize (kSizeOfHeaders);
  EFI_IMAGE_NT_HEADERS64  *Nt = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->FileHeader.NumberOfSections = 0;
  ExpectAuthenticodeImage (Image.data (), Image.size (), ExpectedLinearPeImage (Image, kPeCoffOffset, Image.size ()));
}

TEST (BuildAuthenticodeImageTest, AcceptsMinimumSizedTrailingCertificateTable) {
  std::vector<UINT8>      Image = BuildPe32PlusImage (true);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0x3F8;
  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size           = sizeof (WIN_CERTIFICATE);
  ExpectAuthenticodeImage (Image.data (), Image.size (), ExpectedLinearPeImage (Image, kPeCoffOffset, 0x3F8));

  CONST UINT8  *Certificates = NULL;
  UINTN        Length        = 0;

  ASSERT_EQ (GetWinCertificates (Image.data (), Image.size (), &Certificates, &Length), EFI_SUCCESS);
  EXPECT_EQ (Certificates, Image.data () + 0x3F8);
  EXPECT_EQ (Length, sizeof (WIN_CERTIFICATE));
}

TEST (BuildAuthenticodeImageTest, AcceptsUnalignedImageBuffer) {
  std::vector<UINT8>  Image = BuildPe32PlusImage (true);
  std::vector<UINT8>  Unaligned (Image.size () + 1);
  WIN_CERTIFICATE     Header          = { kSecDirSize, 0x0200, WIN_CERT_TYPE_PKCS_SIGNED_DATA };
  const UINTN         Offset          = ((UINTN)Unaligned.data () % ALIGNOF (WIN_CERTIFICATE) == 0) ? 1 : 0;
  CONST UINT8         *UnalignedImage = Unaligned.data () + Offset;

  ASSERT_NE ((UINTN)UnalignedImage % ALIGNOF (WIN_CERTIFICATE), (UINTN)0);
  CopyMem (Image.data () + kSecDirVa, &Header, sizeof (Header));
  CopyMem (Unaligned.data () + Offset, Image.data (), Image.size ());
  ExpectAuthenticodeImage (UnalignedImage, Image.size (), ExpectedLinearPeImage (Image, kPeCoffOffset, kSecDirVa));

  CONST UINT8  *Certificates = NULL;
  UINTN        Length        = 0;

  ASSERT_EQ (GetWinCertificates (UnalignedImage, Image.size (), &Certificates, &Length), EFI_SUCCESS);
  EXPECT_EQ (Certificates, UnalignedImage + kSecDirVa);
  EXPECT_EQ (Length, (UINTN)kSecDirSize);

  WIN_CERTIFICATE  CopiedHeader;

  CopyMem (&CopiedHeader, Certificates, sizeof (CopiedHeader));
  EXPECT_EQ (CopiedHeader.dwLength, Header.dwLength);
  EXPECT_EQ (CopiedHeader.wRevision, Header.wRevision);
  EXPECT_EQ (CopiedHeader.wCertificateType, Header.wCertificateType);
}

TEST (BuildAuthenticodeImageTest, AcceptsUnalignedPeHeaderOffset) {
  std::vector<UINT8>      Image = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  EFI_IMAGE_DOS_HEADER    *Dos  = (EFI_IMAGE_DOS_HEADER *)Image.data ();

  std::memmove (Image.data () + kPeCoffOffset + 1, Image.data () + kPeCoffOffset, sizeof (*Nt) + sizeof (EFI_IMAGE_SECTION_HEADER));
  Dos->e_lfanew = kPeCoffOffset + 1;
  ExpectAuthenticodeImage (Image.data (), Image.size (), ExpectedLinearPeImage (Image, kPeCoffOffset + 1, Image.size ()));
}

class NoDosHeaderTest : public ::testing::TestWithParam<bool> {
};

TEST_P (NoDosHeaderTest, AcceptsPeHeaderAtFileStart) {
  const bool          Signed = GetParam ();
  std::vector<UINT8>  Image  = BuildPe32PlusImage (Signed);

  std::memmove (Image.data (), Image.data () + kPeCoffOffset, sizeof (EFI_IMAGE_NT_HEADERS64) + sizeof (EFI_IMAGE_SECTION_HEADER));
  const UINTN  DataEnd = Signed ? kSecDirVa : Image.size ();

  ExpectAuthenticodeImage (Image.data (), Image.size (), ExpectedLinearPeImage (Image, 0, DataEnd));
}

INSTANTIATE_TEST_SUITE_P (SignedAndUnsigned, NoDosHeaderTest, ::testing::Bool ());

TEST (BuildAuthenticodeImageErrorTest, RejectsEveryTruncatedSignedImageWithoutReadingPastBuffer) {
  const std::vector<UINT8>  Image = BuildPe32PlusImage (true);

  for (UINTN Length = 0; Length < Image.size (); Length++) {
    std::vector<UINT8>  Truncated (Length == 0 ? 1 : Length);
    if (Length != 0) {
      CopyMem (Truncated.data (), Image.data (), Length);
    }

    UINT8  *AuthImage    = (UINT8 *)(UINTN)1;
    UINTN  AuthImageSize = 123;

    EXPECT_EQ (
      BuildAuthenticodeImage (Truncated.data (), Length, &AuthImage, &AuthImageSize),
      Length == 0 ? EFI_INVALID_PARAMETER : EFI_VOLUME_CORRUPTED
      ) << "truncated length " << Length;
    if (Length != 0) {
      EXPECT_EQ (AuthImage, nullptr);
      EXPECT_EQ (AuthImageSize, (UINTN)0);
    }

    CONST UINT8  *Certificates     = (CONST UINT8 *)(UINTN)1;
    UINTN        CertificateLength = 123;

    EXPECT_EQ (GetWinCertificates (Truncated.data (), Length, &Certificates, &CertificateLength), EFI_VOLUME_CORRUPTED)
    << "truncated length " << Length;
    EXPECT_EQ (Certificates, nullptr);
    EXPECT_EQ (CertificateLength, (UINTN)0);
  }
}

TEST_F (GetWinCertificatesTest, LocatesNonTrailingTableWithoutValidatingPlacement) {
  std::vector<UINT8>      Image = BuildPe32PlusImage (true);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size = 0x80;
  CONST UINT8  *Certificates = NULL;
  UINTN        Length        = 0;

  ASSERT_EQ (GetWinCertificates (Image.data (), Image.size (), &Certificates, &Length), EFI_SUCCESS);
  EXPECT_EQ (Certificates, Image.data () + kSecDirVa);
  EXPECT_EQ (Length, (UINTN)0x80);
}

static std::vector<UINT8>
BuildManySectionsImage (
  UINT16  Count,
  bool    Empty,
  bool    MixedOrder
  )
{
  const UINT32  HeadersSize = (UINT32)((kPeCoffOffset + sizeof (EFI_IMAGE_NT_HEADERS64) + Count * sizeof (EFI_IMAGE_SECTION_HEADER) + 0x1FF) & ~(UINTN)0x1FF);
  const UINT32  DataSize    = Empty ? 0x200 : Count * 8;

  std::vector<UINT8>  Image = BuildPe32PlusImage (false);

  Image.resize (HeadersSize + DataSize, 0);
  EFI_IMAGE_NT_HEADERS64    *Nt       = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);
  EFI_IMAGE_SECTION_HEADER  *Sections = (EFI_IMAGE_SECTION_HEADER *)(Image.data () + kPeCoffOffset + sizeof (*Nt));

  Nt->FileHeader.NumberOfSections  = Count;
  Nt->OptionalHeader.SizeOfHeaders = HeadersSize;
  Nt->OptionalHeader.SizeOfImage   = HeadersSize + DataSize + 0x1000;
  for (UINTN Index = 0; Index < Count; Index++) {
    Sections[Index].VirtualAddress = HeadersSize + (UINT32)Index * 8;
    const UINT32  Slot = MixedOrder ? ((UINT32)Index * 73 + 19) % Count : Count - (UINT32)Index - 1;

    Sections[Index].PointerToRawData = Empty ? (Count - (UINT32)Index) : HeadersSize + Slot * 8;
    Sections[Index].SizeOfRawData    = Empty ? 0 : 8;
  }

  for (UINTN Index = HeadersSize; Index < Image.size (); Index++) {
    Image[Index] = (UINT8)(Index * 31 + (Index >> 8));
  }

  return Image;
}

class ManySectionsTest : public ::testing::TestWithParam<bool> {
};

TEST_P (ManySectionsTest, HandlesReverseOrderedOrEmptySections) {
  std::vector<UINT8>  Image = BuildManySectionsImage (4000, GetParam (), false);

  ExpectAuthenticodeImage (Image.data (), Image.size (), ExpectedLinearPeImage (Image, kPeCoffOffset, Image.size ()));
}

INSTANTIATE_TEST_SUITE_P (EmptyAndNonempty, ManySectionsTest, ::testing::Bool ());

TEST (SectionSortingTest, HandlesMixedSectionOrder) {
  std::vector<UINT8>  Image = BuildManySectionsImage (127, false, true);

  ExpectAuthenticodeImage (Image.data (), Image.size (), ExpectedLinearPeImage (Image, kPeCoffOffset, Image.size ()));
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
