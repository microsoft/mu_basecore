/** @file
  Unit tests for GetWinCertificates in AuthenticodeLib.

  These tests construct minimal-but-valid PE/COFF images in memory and
  exercise the real PeCoffLib through GetWinCertificates so that the
  embedded WIN_CERTIFICATE table lookup path is covered end-to-end.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

#include <vector>

extern "C" {
  #include <Uefi.h>
  #include <IndustryStandard/PeImage.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/AuthenticodeLib.h>
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
  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

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
  std::vector<UINT8>     Image = BuildPe32PlusImage (false);
  CONST WIN_CERTIFICATE  *WinCertificates;

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
  CONST WIN_CERTIFICATE  *WinCertificates;
  CONST WIN_CERTIFICATE  *ExpectedWinCertificates;
  UINTN                  Length;
  UINTN                  ExpectedLength;

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
  std::vector<UINT8>     Buffer (kSizeOfImage, 0xAB);
  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

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
  UINT8                  TinyBuffer[4] = { 0 };
  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

  EXPECT_EQ (
    GetWinCertificates (TinyBuffer, sizeof (TinyBuffer), &WinCertificates, &Length),
    EFI_VOLUME_CORRUPTED
    );
}

TEST_F (GetWinCertificatesTest, ZeroFileSize_ReturnsVolumeCorrupted) {
  UINT8                  Byte = 0;
  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

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

  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

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

  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

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

  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, &Length),
    EFI_VOLUME_CORRUPTED
    );
}

// ---------------------------------------------------------------------------
// Happy paths exercised through real PeCoffLib security-directory capture
// ---------------------------------------------------------------------------

TEST_F (GetWinCertificatesTest, ValidImageWithoutSecurityDir_ReturnsNoCertificates) {
  std::vector<UINT8>     Image = BuildPe32PlusImage (false);
  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

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
  EXPECT_EQ (WinCertificates, (CONST WIN_CERTIFICATE *)NULL);
  EXPECT_EQ (Length, (UINTN)0);
}

TEST_F (GetWinCertificatesTest, ValidImageWithSecurityDir_ReturnsCertificates) {
  std::vector<UINT8>     Image = BuildPe32PlusImage (true);
  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, &Length),
    EFI_SUCCESS
    );
  EXPECT_EQ ((CONST UINT8 *)WinCertificates, (CONST UINT8 *)(Image.data () + kSecDirVa));
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

  CONST WIN_CERTIFICATE  *WinCertificates;
  UINTN                  Length;

  EXPECT_EQ (
    GetWinCertificates (Image.data (), Image.size (), &WinCertificates, &Length),
    EFI_SUCCESS
    );
  EXPECT_EQ ((CONST UINT8 *)WinCertificates, (CONST UINT8 *)(Image.data () + kSecDirVa));
  EXPECT_EQ (Length, (UINTN)kSecDirSize);
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
