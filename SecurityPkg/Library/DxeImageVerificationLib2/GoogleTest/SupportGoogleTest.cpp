/** @file
  Unit tests for GetImageSecurityDataDirectory in the
  DxeImageVerificationLib.
  These tests construct minimal-but-valid PE/COFF images in memory and
  exercise the real PeCoffLib through GetImageSecurityDataDirectory so
  that the security-data-directory capture path is covered end-to-end.
  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockBaseCryptLib.h>

#include <vector>
#include <cstring>

extern "C" {
  #include <Uefi.h>
  #include <IndustryStandard/PeImage.h>
  #include <Library/BaseMemoryLib.h>
  #include "../Support.h"
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

static bool
GetImageHashIndexForTest (
  const EFI_GUID  *Guid,
  UINTN           *Index
  )
{
  UINTN  I;

  if ((Guid == nullptr) || (Index == nullptr)) {
    return false;
  }

  for (I = 0; I < ARRAY_SIZE (mHashAlgorithms); I++) {
    if (CompareGuid (Guid, mHashAlgorithms[I].ImageHashGuid)) {
      *Index = I;
      return true;
    }
  }

  return false;
}

static bool
GetX509HashIndexForTest (
  const EFI_GUID  *Guid,
  UINTN           *Index
  )
{
  UINTN  I;

  if ((Guid == nullptr) || (Index == nullptr)) {
    return false;
  }

  for (I = 0; I < ARRAY_SIZE (mHashAlgorithms); I++) {
    if (CompareGuid (Guid, mHashAlgorithms[I].X509CertHashGuid)) {
      *Index = I;
      return true;
    }
  }

  return false;
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
static constexpr UINT32  kNt64Size      = sizeof (EFI_IMAGE_NT_HEADERS64);
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

class GetImageSecurityDataDirectoryTest : public ::testing::Test {
};

// ---------------------------------------------------------------------------
// Argument validation
// ---------------------------------------------------------------------------

TEST_F (GetImageSecurityDataDirectoryTest, NullFileBuffer_ReturnsInvalidParameter) {
  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  EXPECT_EQ (
    GetImageSecurityDataDirectory (NULL, 0x100, &SecDataDir),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (GetImageSecurityDataDirectoryTest, NullSecDataDir_ReturnsInvalidParameter) {
  std::vector<UINT8>  Image = BuildPe32PlusImage (false);

  EXPECT_EQ (
    GetImageSecurityDataDirectory (Image.data (), Image.size (), NULL),
    EFI_INVALID_PARAMETER
    );
}

TEST_F (GetImageSecurityDataDirectoryTest, NullFileBuffer_DoesNotTouchSecDataDir) {
  //
  // Caller-supplied SecDataDir must be untouched on the early
  // invalid-parameter return; only the success path is allowed to
  // overwrite it.
  //
  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  SetMem (&SecDataDir, sizeof (SecDataDir), 0xAA);

  EXPECT_EQ (
    GetImageSecurityDataDirectory (NULL, 0x100, &SecDataDir),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (SecDataDir.VirtualAddress, 0xAAAAAAAAu);
  EXPECT_EQ (SecDataDir.Size, 0xAAAAAAAAu);
}

// ---------------------------------------------------------------------------
// Malformed images
// ---------------------------------------------------------------------------

TEST_F (GetImageSecurityDataDirectoryTest, GarbageBuffer_ReturnsLoadError) {
  //
  // A buffer that is large enough to attempt header parsing but does
  // not actually carry valid DOS/NT signatures must be rejected.
  //
  std::vector<UINT8>        Buffer (kSizeOfImage, 0xAB);
  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  EXPECT_EQ (
    GetImageSecurityDataDirectory (Buffer.data (), Buffer.size (), &SecDataDir),
    EFI_LOAD_ERROR
    );
}

TEST_F (GetImageSecurityDataDirectoryTest, TinyBuffer_ReturnsLoadError) {
  //
  // A buffer too small to contain a DOS header must be rejected
  // without dereferencing any of the bytes.
  //
  UINT8                     TinyBuffer[4] = { 0 };
  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  EXPECT_EQ (
    GetImageSecurityDataDirectory (TinyBuffer, sizeof (TinyBuffer), &SecDataDir),
    EFI_LOAD_ERROR
    );
}

TEST_F (GetImageSecurityDataDirectoryTest, ZeroFileSize_ReturnsLoadError) {
  UINT8                     Byte = 0;
  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  EXPECT_EQ (
    GetImageSecurityDataDirectory (&Byte, 0, &SecDataDir),
    EFI_LOAD_ERROR
    );
}

TEST_F (GetImageSecurityDataDirectoryTest, ElfanewPastEof_ReturnsLoadError) {
  //
  // Valid DOS signature but e_lfanew points well past the end of the
  // buffer. The bounded reader must refuse to fetch the NT headers.
  //
  std::vector<UINT8>    Image = BuildPe32PlusImage (false);
  EFI_IMAGE_DOS_HEADER  *Dos  = (EFI_IMAGE_DOS_HEADER *)Image.data ();

  Dos->e_lfanew = (UINT32)(Image.size () + 0x1000);

  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  EXPECT_EQ (
    GetImageSecurityDataDirectory (Image.data (), Image.size (), &SecDataDir),
    EFI_LOAD_ERROR
    );
}

TEST_F (GetImageSecurityDataDirectoryTest, TruncatedAtNtHeaders_ReturnsLoadError) {
  //
  // Buffer is truncated to the DOS header only, so the read of the NT
  // headers at e_lfanew straddles EOF.
  //
  std::vector<UINT8>  Image = BuildPe32PlusImage (false);

  Image.resize (sizeof (EFI_IMAGE_DOS_HEADER));

  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  EXPECT_EQ (
    GetImageSecurityDataDirectory (Image.data (), Image.size (), &SecDataDir),
    EFI_LOAD_ERROR
    );
}

TEST_F (GetImageSecurityDataDirectoryTest, ValidDosBadNtSignature_ReturnsLoadError) {
  //
  // Valid DOS header but corrupted NT signature -- exercises the NT
  // signature validation path inside PeCoffLib.
  //
  std::vector<UINT8>      Image = BuildPe32PlusImage (false);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->Signature = 0xDEADBEEF;

  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  EXPECT_EQ (
    GetImageSecurityDataDirectory (Image.data (), Image.size (), &SecDataDir),
    EFI_LOAD_ERROR
    );
}

// ---------------------------------------------------------------------------
// Happy paths exercised through real PeCoffLib security-directory capture
// ---------------------------------------------------------------------------

TEST_F (GetImageSecurityDataDirectoryTest, ValidImageWithoutSecurityDir_ReturnsZeroedDir) {
  std::vector<UINT8>        Image = BuildPe32PlusImage (false);
  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  //
  // Pre-poison SecDataDir so we can prove the function actually wrote
  // back zeros (rather than leaving stale caller-supplied bytes).
  //
  SetMem (&SecDataDir, sizeof (SecDataDir), 0xCD);

  EXPECT_EQ (
    GetImageSecurityDataDirectory (Image.data (), Image.size (), &SecDataDir),
    EFI_SUCCESS
    );
  EXPECT_EQ (SecDataDir.VirtualAddress, 0u);
  EXPECT_EQ (SecDataDir.Size, 0u);
}

TEST_F (GetImageSecurityDataDirectoryTest, ValidImageWithSecurityDir_ReturnsCapturedDir) {
  std::vector<UINT8>        Image = BuildPe32PlusImage (true);
  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  EXPECT_EQ (
    GetImageSecurityDataDirectory (Image.data (), Image.size (), &SecDataDir),
    EFI_SUCCESS
    );
  EXPECT_EQ (SecDataDir.VirtualAddress, kSecDirVa);
  EXPECT_EQ (SecDataDir.Size, kSecDirSize);
}

TEST_F (GetImageSecurityDataDirectoryTest, OtherDataDirectoriesDoNotLeak) {
  //
  // Populate a non-SECURITY data directory entry (export table) with
  // a recognizable value to prove PeCoffLib only records the SECURITY
  // entry and ignores every other directory it walks.
  //
  std::vector<UINT8>      Image = BuildPe32PlusImage (true);
  EFI_IMAGE_NT_HEADERS64  *Nt   = (EFI_IMAGE_NT_HEADERS64 *)(Image.data () + kPeCoffOffset);

  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = 0xCAFEBABE;
  Nt->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].Size           = 0xBEEFu;

  EFI_IMAGE_DATA_DIRECTORY  SecDataDir;

  EXPECT_EQ (
    GetImageSecurityDataDirectory (Image.data (), Image.size (), &SecDataDir),
    EFI_SUCCESS
    );
  EXPECT_EQ (SecDataDir.VirtualAddress, kSecDirVa);
  EXPECT_EQ (SecDataDir.Size, kSecDirSize);
}

// ---------------------------------------------------------------------------
// GetHash
// ---------------------------------------------------------------------------

TEST (GetHashTest, NullParameters_ReturnsInvalidParameter) {
  DIGEST_CACHE  Cache;
  CONST UINT8   *Digest    = NULL;
  UINTN         DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (CONST VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_EQ (GetHash (NULL, &Cache, &Digest, &DigestSize), EFI_INVALID_PARAMETER);
  EXPECT_EQ (GetHash (&gEfiCertSha256Guid, NULL, &Digest, &DigestSize), EFI_INVALID_PARAMETER);
  EXPECT_EQ (GetHash (&gEfiCertSha256Guid, &Cache, NULL, &DigestSize), EFI_INVALID_PARAMETER);
  EXPECT_EQ (GetHash (&gEfiCertSha256Guid, &Cache, &Digest, NULL), EFI_INVALID_PARAMETER);
}

TEST (GetHashTest, CacheWithoutFileBuffer_ReturnsInvalidParameter) {
  DIGEST_CACHE  Cache;
  CONST UINT8   *Digest    = NULL;
  UINTN         DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));

  EXPECT_EQ (GetHash (&gEfiCertSha256Guid, &Cache, &Digest, &DigestSize), EFI_INVALID_PARAMETER);
}

TEST (GetHashTest, UnsupportedHashGuid_ReturnsUnsupported) {
  EFI_GUID      UnknownGuid = {
    0xA1B2C3D4,
    0x9999,
    0x8888,
    { 0x10,    0x20,0x30, 0x40, 0x50, 0x60, 0x70, 0x80 }
  };
  DIGEST_CACHE  Cache;
  CONST UINT8   *Digest    = NULL;
  UINTN         DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (CONST VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_EQ (GetHash (&UnknownGuid, &Cache, &Digest, &DigestSize), EFI_UNSUPPORTED);
}

TEST (GetHashTest, CacheHit_ReturnsExistingDigestBytes) {
  DIGEST_CACHE  Cache;
  CONST UINT8   *Digest    = NULL;
  UINTN         DigestSize = 0;
  UINTN         SlotIndex;
  CONST UINT8   ExpectedDigest[] = { 0x10, 0x20, 0x30, 0x40 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (CONST VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  ASSERT_TRUE (GetImageHashIndexForTest (&gEfiCertSha256Guid, &SlotIndex));
  CopyMem (Cache.Entries[SlotIndex].Bytes, ExpectedDigest, sizeof (ExpectedDigest));
  Cache.Entries[SlotIndex].BufferSize = sizeof (ExpectedDigest);

  EXPECT_EQ (GetHash (&gEfiCertSha256Guid, &Cache, &Digest, &DigestSize), EFI_SUCCESS);
  ASSERT_NE (Digest, (CONST UINT8 *)NULL);
  EXPECT_EQ (DigestSize, sizeof (ExpectedDigest));
  EXPECT_EQ (CompareMem (Digest, ExpectedDigest, sizeof (ExpectedDigest)), 0);
}

TEST (GetHashTest, CacheMiss_ComputesAndCachesDigest) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest    = NULL;
  UINTN             DigestSize = 0;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (CONST VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (
             IN  VOID            *FileBuffer,
             IN  UINTN           FileSize,
             IN  CONST EFI_GUID  *HashType,
             OUT UINT8           *OutDigest,
             OUT UINTN           *OutDigestSize
         ) -> EFI_STATUS {
    (VOID)FileBuffer;
    (VOID)FileSize;
    (VOID)HashType;
    OutDigest[0]   = 0x11;
    OutDigest[1]   = 0x22;
    OutDigest[2]   = 0x33;
    *OutDigestSize = 3;
    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_EQ (GetHash (&gEfiCertSha256Guid, &Cache, &Digest, &DigestSize), EFI_SUCCESS);
  ASSERT_NE (Digest, (CONST UINT8 *)NULL);
  EXPECT_EQ (DigestSize, (UINTN)3);
  EXPECT_EQ (Digest[0], 0x11);
  EXPECT_EQ (Digest[1], 0x22);
  EXPECT_EQ (Digest[2], 0x33);
}

TEST (GetHashTest, CacheMiss_HashFailure_ClearsSlotAndReturnsSecurityViolation) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest    = (CONST UINT8 *)(UINTN)1;
  UINTN             DigestSize = 9;
  UINTN             SlotIndex;

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Buffer     = (CONST VOID *)(UINTN)1;
  Cache.BufferSize = 1;

  ASSERT_TRUE (GetImageHashIndexForTest (&gEfiCertSha256Guid, &SlotIndex));

  EXPECT_CALL (BaseCryptLibMock, GetAuthenticodeHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (
             IN  VOID            *FileBuffer,
             IN  UINTN           FileSize,
             IN  CONST EFI_GUID  *HashType,
             OUT UINT8           *OutDigest,
             OUT UINTN           *OutDigestSize
         ) -> EFI_STATUS {
    (VOID)FileBuffer;
    (VOID)FileSize;
    (VOID)HashType;
    (VOID)OutDigest;
    *OutDigestSize = 64;
    return EFI_DEVICE_ERROR;
  }
         )
       );

  EXPECT_EQ (GetHash (&gEfiCertSha256Guid, &Cache, &Digest, &DigestSize), EFI_SECURITY_VIOLATION);
  EXPECT_EQ (Cache.Entries[SlotIndex].BufferSize, (UINTN)0);
}

TEST (GetHashTest, X509CacheMiss_ComputesAndCachesDigest) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest    = NULL;
  UINTN             DigestSize = 0;
  UINTN             SlotIndex;
  UINT8             Data[3] = { 0xA1, 0xB2, 0xC3 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = DigestCacheTypeX509;
  Cache.Buffer     = Data;
  Cache.BufferSize = sizeof (Data);

  ASSERT_TRUE (GetX509HashIndexForTest (&gEfiCertX509Sha256Guid, &SlotIndex));

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (
       Invoke (
         [] (
             IN  VOID            *CertBuffer,
             IN  UINTN           CertSize,
             IN  CONST EFI_GUID  *HashType,
             OUT UINT8           *OutDigest,
             OUT UINTN           *OutDigestSize
         ) -> EFI_STATUS {
    (VOID)CertBuffer;
    (VOID)CertSize;
    (VOID)HashType;
    SetMem (OutDigest, SHA256_DIGEST_SIZE, 0x5A);
    *OutDigestSize = SHA256_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  EXPECT_EQ (GetHash (&gEfiCertX509Sha256Guid, &Cache, &Digest, &DigestSize), EFI_SUCCESS);
  ASSERT_NE (Digest, (CONST UINT8 *)NULL);
  EXPECT_EQ (DigestSize, (UINTN)SHA256_DIGEST_SIZE);
  EXPECT_EQ (Cache.Entries[SlotIndex].BufferSize, (UINTN)SHA256_DIGEST_SIZE);
  EXPECT_EQ (Digest[0], (UINT8)0x5A);
}

TEST (GetHashTest, X509CacheMiss_HashFailure_ClearsSlotAndReturnsSecurityViolation) {
  MockBaseCryptLib  BaseCryptLibMock;
  DIGEST_CACHE      Cache;
  CONST UINT8       *Digest    = (CONST UINT8 *)(UINTN)1;
  UINTN             DigestSize = 17;
  UINTN             SlotIndex;
  UINT8             Data[3] = { 0xA1, 0xB2, 0xC3 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = DigestCacheTypeX509;
  Cache.Buffer     = Data;
  Cache.BufferSize = sizeof (Data);

  ASSERT_TRUE (GetX509HashIndexForTest (&gEfiCertX509Sha256Guid, &SlotIndex));

  EXPECT_CALL (BaseCryptLibMock, X509GetTbsCertHash (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  EXPECT_EQ (GetHash (&gEfiCertX509Sha256Guid, &Cache, &Digest, &DigestSize), EFI_SECURITY_VIOLATION);
  EXPECT_EQ (Cache.Entries[SlotIndex].BufferSize, (UINTN)0);
}

TEST (GetHashTest, X509CacheWithImageGuid_ReturnsUnsupported) {
  DIGEST_CACHE  Cache;
  CONST UINT8   *Digest    = NULL;
  UINTN         DigestSize = 0;
  UINT8         Data[1]    = { 0x11 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = DigestCacheTypeX509;
  Cache.Buffer     = Data;
  Cache.BufferSize = sizeof (Data);

  EXPECT_EQ (GetHash (&gEfiCertSha256Guid, &Cache, &Digest, &DigestSize), EFI_UNSUPPORTED);
}

TEST (GetHashTest, ImageCacheWithX509Guid_ReturnsUnsupported) {
  DIGEST_CACHE  Cache;
  CONST UINT8   *Digest    = NULL;
  UINTN         DigestSize = 0;
  UINT8         Data[1]    = { 0x22 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = DigestCacheTypeImage;
  Cache.Buffer     = Data;
  Cache.BufferSize = sizeof (Data);

  EXPECT_EQ (GetHash (&gEfiCertX509Sha256Guid, &Cache, &Digest, &DigestSize), EFI_UNSUPPORTED);
}

TEST (GetHashTest, InvalidCacheType_ReturnsUnsupported) {
  DIGEST_CACHE  Cache;
  CONST UINT8   *Digest    = NULL;
  UINTN         DigestSize = 0;
  UINT8         Data[1]    = { 0x33 };

  ZeroMem (&Cache, sizeof (Cache));
  Cache.Type       = (DIGEST_CACHE_TYPE)0xFF;
  Cache.Buffer     = Data;
  Cache.BufferSize = sizeof (Data);

  EXPECT_EQ (GetHash (&gEfiCertSha256Guid, &Cache, &Digest, &DigestSize), EFI_UNSUPPORTED);
}

// ---------------------------------------------------------------------------
// IsX509CertHashGuid
// ---------------------------------------------------------------------------

TEST (IsX509CertHashGuidTest, NullGuid_ReturnsFalse) {
  EXPECT_FALSE (IsX509CertHashGuid (NULL));
}

TEST (IsX509CertHashGuidTest, X509CertHashGuid_ReturnsTrue) {
  EXPECT_TRUE (IsX509CertHashGuid (&gEfiCertX509Sha256Guid));
}

TEST (IsX509CertHashGuidTest, NonX509CertHashGuid_ReturnsFalse) {
  // A GUID that is not present in the mHashAlgorithms X509CertHashGuid column.
  const EFI_GUID  OtherGuid = {
    0x11111111, 0x2222, 0x3333, { 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB }
  };

  EXPECT_FALSE (IsX509CertHashGuid (&OtherGuid));
}
