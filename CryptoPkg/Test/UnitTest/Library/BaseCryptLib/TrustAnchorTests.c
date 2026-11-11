/** @file
  End-to-end unit tests for GetTrustAnchorX509FromAuthData() and
  FreeTrustAnchorX509Cache().

  The tests build a minimal PKCS#7 SignedData blob around one (or more)
  known X.509 certificates, compute the expected TBSCertificate digest
  with the public BaseCryptLib hash primitives, and assert that the
  function returns the matching certificate. Both the bare SignedData
  encoding and the ContentInfo wrapper are exercised, all four supported
  digest sizes are covered, and the optional cache is verified across
  back-to-back calls.

  The tests depend only on public BaseCryptLib API, so they run
  identically against the OpenSSL and MbedTLS BaseCryptLib instances.

Copyright (C) Microsoft Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"

//
// Self-signed test certificate generated for this unit test:
//   Subject : CN=Edk2 BaseCryptLib TrustAnchor Test, O=Edk2
//   Algo    : ECDSA P-256 / SHA-256
//   Validity: 2026-06-10 .. 2126-05-17
//   Size    : 468 bytes (DER)
// The cert is self-signed and not part of any chain of trust; it is
// only used as a stable, deterministic blob to feed into
// GetTrustAnchorX509FromAuthData() and X509GetTBSCert(). Regenerate
// with:
//   openssl ecparam -name prime256v1 -genkey -noout -out anchor.key
//   openssl req -new -x509 -key anchor.key -days 36500 -sha256
//     -subj '/CN=Edk2 BaseCryptLib TrustAnchor Test/O=Edk2'
//     -out anchor.pem
//   openssl x509 -in anchor.pem -outform DER -out anchor.der
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  mTrustAnchorTestCert[] = {
  0x30, 0x82, 0x01, 0xd0, 0x30, 0x82, 0x01, 0x75, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x14, 0x02, // 0...0..u........
  0x69, 0xba, 0x47, 0x29, 0x15, 0xe0, 0x37, 0x67, 0xaf, 0x93, 0x02, 0x42, 0x82, 0xb3, 0x8e, 0xba, // i.G)..7g...B....
  0xe4, 0xe6, 0xf5, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x30, // ...0...*.H.=...0
  0x3c, 0x31, 0x2b, 0x30, 0x29, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x22, 0x45, 0x64, 0x6b, 0x32, // <1+0)..U..."Edk2
  0x20, 0x42, 0x61, 0x73, 0x65, 0x43, 0x72, 0x79, 0x70, 0x74, 0x4c, 0x69, 0x62, 0x20, 0x54, 0x72, //  BaseCryptLib Tr
  0x75, 0x73, 0x74, 0x41, 0x6e, 0x63, 0x68, 0x6f, 0x72, 0x20, 0x54, 0x65, 0x73, 0x74, 0x31, 0x0d, // ustAnchor Test1.
  0x30, 0x0b, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x04, 0x45, 0x64, 0x6b, 0x32, 0x30, 0x20, 0x17, // 0...U....Edk20 .
  0x0d, 0x32, 0x36, 0x30, 0x36, 0x31, 0x30, 0x32, 0x32, 0x35, 0x36, 0x35, 0x32, 0x5a, 0x18, 0x0f, // .260610225652Z..
  0x32, 0x31, 0x32, 0x36, 0x30, 0x35, 0x31, 0x37, 0x32, 0x32, 0x35, 0x36, 0x35, 0x32, 0x5a, 0x30, // 21260517225652Z0
  0x3c, 0x31, 0x2b, 0x30, 0x29, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x22, 0x45, 0x64, 0x6b, 0x32, // <1+0)..U..."Edk2
  0x20, 0x42, 0x61, 0x73, 0x65, 0x43, 0x72, 0x79, 0x70, 0x74, 0x4c, 0x69, 0x62, 0x20, 0x54, 0x72, //  BaseCryptLib Tr
  0x75, 0x73, 0x74, 0x41, 0x6e, 0x63, 0x68, 0x6f, 0x72, 0x20, 0x54, 0x65, 0x73, 0x74, 0x31, 0x0d, // ustAnchor Test1.
  0x30, 0x0b, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x04, 0x45, 0x64, 0x6b, 0x32, 0x30, 0x59, 0x30, // 0...U....Edk20Y0
  0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, // ...*.H.=....*.H.
  0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0xe9, 0xe0, 0x54, 0x8f, 0x06, 0x5e, 0x77, 0x3c, // =....B....T..^w<
  0x18, 0x57, 0xe9, 0x24, 0xd0, 0xac, 0xb6, 0xaa, 0x6b, 0x90, 0x31, 0xac, 0xbc, 0x6d, 0x49, 0xc8, // .W.$....k.1..mI.
  0x5a, 0xe7, 0x3d, 0x37, 0x68, 0x08, 0x17, 0xcd, 0xde, 0xa1, 0xbb, 0x1f, 0x37, 0x63, 0xd1, 0x74, // Z.=7h.......7c.t
  0xe4, 0x7b, 0x5f, 0x41, 0x92, 0x79, 0x93, 0x6b, 0xe6, 0xdb, 0x4d, 0x60, 0xf2, 0x09, 0xe3, 0xe5, // .{_A.y.k..M`....
  0x45, 0xf9, 0xf9, 0xd6, 0x24, 0x77, 0x29, 0x3d, 0xa3, 0x53, 0x30, 0x51, 0x30, 0x1d, 0x06, 0x03, // E...$w)=.S0Q0...
  0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0xb4, 0x77, 0xef, 0xe6, 0xe2, 0x4f, 0x97, 0xd0, 0x12, // U.......w...O...
  0xc8, 0xa6, 0xb0, 0xec, 0x95, 0xdb, 0xc4, 0xa5, 0x3b, 0x76, 0xf1, 0x30, 0x1f, 0x06, 0x03, 0x55, // ........;v.0...U
  0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0xb4, 0x77, 0xef, 0xe6, 0xe2, 0x4f, 0x97, 0xd0, // .#..0....w...O..
  0x12, 0xc8, 0xa6, 0xb0, 0xec, 0x95, 0xdb, 0xc4, 0xa5, 0x3b, 0x76, 0xf1, 0x30, 0x0f, 0x06, 0x03, // .........;v.0...
  0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0a, 0x06, // U.......0....0..
  0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x49, 0x00, 0x30, 0x46, 0x02, 0x21, // .*.H.=....I.0F.!
  0x00, 0xaa, 0x4b, 0x52, 0x77, 0xec, 0xd8, 0x28, 0x2d, 0x0d, 0x3b, 0x15, 0xec, 0xdc, 0xde, 0x10, // ..KRw..(-.;.....
  0xdb, 0x22, 0xdc, 0x61, 0x2b, 0xcc, 0x96, 0x12, 0xe7, 0x15, 0xdb, 0x20, 0x0f, 0x42, 0xc9, 0xf6, // .".a+...... .B..
  0x4a, 0x02, 0x21, 0x00, 0xcd, 0xbf, 0xb8, 0x04, 0xc8, 0x62, 0x5d, 0xf8, 0xe9, 0xae, 0x2b, 0x81, // J.!......b]...+.
  0x8b, 0xf6, 0xcb, 0x31, 0x80, 0xea, 0xbc, 0x59, 0x71, 0xb5, 0xba, 0x20, 0xff, 0xb8, 0x3e, 0x79, // ...1...Yq.. ..>y
  0x7a, 0x67, 0x85, 0x7e,                                                                         // zg.~
};

//
// PKCS#7 signedData OID: 1.2.840.113549.1.7.2
//
STATIC CONST UINT8  mSignedDataOid[] = {
  0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02
};

//
// PKCS#7 data OID: 1.2.840.113549.1.7.1 (used as the encapContentInfo
// content type).
//
STATIC CONST UINT8  mDataOidTLV[] = {
  0x30, 0x0B,                                                      // SEQUENCE (encapContentInfo, body=11)
  0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x01 //   contentType OID = data
};

//
// Constant TLVs that don't depend on the cert.
//
STATIC CONST UINT8  mVersion1[] = { 0x02, 0x01, 0x01 };     // INTEGER 1
STATIC CONST UINT8  mEmptySet[] = { 0x31, 0x00 };           // SET {}

/**
  Encode a DER definite-length field. Writes between 1 and 5 bytes to
  Out and returns the number written.

  @param[in]  Length  Length to encode.
  @param[out] Out     Receives 1..5 bytes.

  @return Number of bytes written.
**/
STATIC
UINTN
DerEncodeLength (
  IN  UINTN  Length,
  OUT UINT8  *Out
  )
{
  if (Length < 0x80) {
    Out[0] = (UINT8)Length;
    return 1;
  }

  if (Length <= 0xFF) {
    Out[0] = 0x81;
    Out[1] = (UINT8)Length;
    return 2;
  }

  if (Length <= 0xFFFF) {
    Out[0] = 0x82;
    Out[1] = (UINT8)((Length >> 8) & 0xFF);
    Out[2] = (UINT8)(Length & 0xFF);
    return 3;
  }

  Out[0] = 0x84;
  Out[1] = (UINT8)((Length >> 24) & 0xFF);
  Out[2] = (UINT8)((Length >> 16) & 0xFF);
  Out[3] = (UINT8)((Length >> 8)  & 0xFF);
  Out[4] = (UINT8)(Length & 0xFF);
  return 5;
}

/**
  Build a minimal PKCS#7 SignedData blob containing the supplied
  certificates.

    SignedData ::= SEQUENCE {
      version           INTEGER 1,
      digestAlgorithms  SET (empty),
      encapContentInfo  SEQUENCE { contentType OID = data },
      certificates [0]  IMPLICIT SET OF Certificate,
      signerInfos       SET (empty)
    }

  When WrapInContentInfo is TRUE, the SignedData is further wrapped in:

    ContentInfo ::= SEQUENCE {
      contentType  OID = signedData,
      content [0]  EXPLICIT SignedData
    }

  @param[in]  Certs              Array of certificate buffer pointers.
  @param[in]  CertSizes          Array of certificate sizes (parallel
                                 to Certs).
  @param[in]  CertCount          Number of certificates.
  @param[in]  WrapInContentInfo  TRUE to add the ContentInfo wrapper.
  @param[out] OutBuf             Receives the allocated buffer.
  @param[out] OutSize            Receives the buffer size.

  @retval EFI_SUCCESS  Buffer was constructed.
**/
STATIC
EFI_STATUS
BuildSignedData (
  IN  CONST UINT8  *CONST  *Certs,
  IN  CONST UINTN          *CertSizes,
  IN  UINTN                CertCount,
  IN  BOOLEAN              WrapInContentInfo,
  OUT UINT8                **OutBuf,
  OUT UINTN                *OutSize
  )
{
  UINTN  Index;
  UINTN  CertsTotal;
  UINTN  CertsTlvSize;     // certificates [0] IMPLICIT TLV size
  UINTN  SdContentSize;    // SignedData inner content size
  UINTN  SdTlvSize;        // SignedData SEQUENCE TLV size
  UINTN  TotalSize;
  UINT8  *Buf;
  UINT8  *Cursor;
  UINT8  LenScratch[5];
  UINTN  LenBytes;

  CertsTotal = 0;
  for (Index = 0; Index < CertCount; Index++) {
    CertsTotal += CertSizes[Index];
  }

  //
  // The certificates [0] IMPLICIT field replaces the SET-OF tag with
  // [0] (0xA0). The contents are simply the concatenated DER-encoded
  // Certificate TLVs.
  //
  CertsTlvSize = 1 + DerEncodeLength (CertsTotal, LenScratch) + CertsTotal;

  SdContentSize = sizeof (mVersion1) + sizeof (mEmptySet) +
                  sizeof (mDataOidTLV) + CertsTlvSize + sizeof (mEmptySet);

  SdTlvSize = 1 + DerEncodeLength (SdContentSize, LenScratch) + SdContentSize;

  if (WrapInContentInfo) {
    //
    // ContentInfo: SEQUENCE { OID, [0] EXPLICIT SignedData }
    //
    UINTN  ExplicitContentSize = SdTlvSize;
    UINTN  ExplicitTlvSize     = 1 + DerEncodeLength (ExplicitContentSize, LenScratch) + ExplicitContentSize;
    UINTN  CiContentSize       = sizeof (mSignedDataOid) + ExplicitTlvSize;
    UINTN  CiTlvSize           = 1 + DerEncodeLength (CiContentSize, LenScratch) + CiContentSize;

    TotalSize = CiTlvSize;
    Buf       = AllocatePool (TotalSize);
    if (Buf == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Cursor = Buf;
    //
    // Outer ContentInfo SEQUENCE
    //
    *Cursor++ = 0x30;
    LenBytes  = DerEncodeLength (CiContentSize, Cursor);
    Cursor   += LenBytes;
    //
    // contentType OID
    //
    CopyMem (Cursor, mSignedDataOid, sizeof (mSignedDataOid));
    Cursor += sizeof (mSignedDataOid);
    //
    // [0] EXPLICIT
    //
    *Cursor++ = 0xA0;
    LenBytes  = DerEncodeLength (ExplicitContentSize, Cursor);
    Cursor   += LenBytes;
  } else {
    TotalSize = SdTlvSize;
    Buf       = AllocatePool (TotalSize);
    if (Buf == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Cursor = Buf;
  }

  //
  // SignedData SEQUENCE
  //
  *Cursor++ = 0x30;
  LenBytes  = DerEncodeLength (SdContentSize, Cursor);
  Cursor   += LenBytes;

  //
  //   version
  //
  CopyMem (Cursor, mVersion1, sizeof (mVersion1));
  Cursor += sizeof (mVersion1);
  //
  //   digestAlgorithms (empty SET)
  //
  CopyMem (Cursor, mEmptySet, sizeof (mEmptySet));
  Cursor += sizeof (mEmptySet);
  //
  //   encapContentInfo
  //
  CopyMem (Cursor, mDataOidTLV, sizeof (mDataOidTLV));
  Cursor += sizeof (mDataOidTLV);
  //
  //   certificates [0] IMPLICIT
  //
  *Cursor++ = 0xA0;
  LenBytes  = DerEncodeLength (CertsTotal, Cursor);
  Cursor   += LenBytes;
  for (Index = 0; Index < CertCount; Index++) {
    CopyMem (Cursor, Certs[Index], CertSizes[Index]);
    Cursor += CertSizes[Index];
  }

  //
  //   signerInfos (empty SET)
  //
  CopyMem (Cursor, mEmptySet, sizeof (mEmptySet));
  Cursor += sizeof (mEmptySet);

  ASSERT ((UINTN)(Cursor - Buf) == TotalSize);
  *OutBuf  = Buf;
  *OutSize = TotalSize;
  return EFI_SUCCESS;
}

/**
  Compute the TBSCertificate digest of mTrustAnchorTestCert under a
  given algorithm, using only public BaseCryptLib API.

  @param[in]   HashSize    20, 32, 48, or 64.
  @param[out]  Digest      Caller-allocated, at least HashSize bytes.

  @retval TRUE   Digest computed.
  @retval FALSE  X509GetTBSCert / hash failed.
**/
STATIC
BOOLEAN
ComputeExpectedTbsHash (
  IN  UINTN  HashSize,
  OUT UINT8  *Digest
  )
{
  UINT8    *TbsCert;
  UINTN    TbsCertSize;
  BOOLEAN  Ok;

  TbsCert     = NULL;
  TbsCertSize = 0;
  if (!X509GetTBSCert (mTrustAnchorTestCert, sizeof (mTrustAnchorTestCert), &TbsCert, &TbsCertSize)) {
    return FALSE;
  }

  switch (HashSize) {
    case SHA1_DIGEST_SIZE:
      Ok = Sha1HashAll (TbsCert, TbsCertSize, Digest);
      break;
    case SHA256_DIGEST_SIZE:
      Ok = Sha256HashAll (TbsCert, TbsCertSize, Digest);
      break;
    case SHA384_DIGEST_SIZE:
      Ok = Sha384HashAll (TbsCert, TbsCertSize, Digest);
      break;
    case SHA512_DIGEST_SIZE:
      Ok = Sha512HashAll (TbsCert, TbsCertSize, Digest);
      break;
    default:
      Ok = FALSE;
      break;
  }

  //
  // X509GetTBSCert may return a pointer into the cert (no allocation)
  // or an allocated buffer depending on the backend. Free defensively
  // when it lies outside the source cert.
  //
  if ((TbsCert != NULL) &&
      ((TbsCert < mTrustAnchorTestCert) ||
       (TbsCert >= mTrustAnchorTestCert + sizeof (mTrustAnchorTestCert))))
  {
    FreePool (TbsCert);
  }

  return Ok;
}

/**
  Common positive-path test: build SignedData, compute the expected
  TBS hash with the given algorithm, ask GetTrustAnchorX509FromAuthData
  to find the cert, and verify byte-equality with the original.
**/
STATIC
UNIT_TEST_STATUS
RunFindCertCase (
  IN UINTN    HashSize,
  IN BOOLEAN  WrapInContentInfo
  )
{
  EFI_STATUS   Status;
  UINT8        *AuthData;
  UINTN        AuthDataSize;
  UINT8        Expected[SHA512_DIGEST_SIZE];
  UINT8        *FoundCert;
  UINTN        FoundCertSize;
  CONST UINT8  *Cert    = mTrustAnchorTestCert;
  UINTN        CertSize = sizeof (mTrustAnchorTestCert);

  AuthData     = NULL;
  AuthDataSize = 0;
  FoundCert    = NULL;

  //
  // Compute hash first so an X509GetTBSCert failure (e.g. against the
  // Null BaseCryptLib) doesn't leak the SignedData allocation.
  //
  UT_ASSERT_TRUE (ComputeExpectedTbsHash (HashSize, Expected));

  Status = BuildSignedData (&Cert, &CertSize, 1, WrapInContentInfo, &AuthData, &AuthDataSize);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = GetTrustAnchorX509FromAuthData (
             NULL,
             Expected,
             HashSize,
             AuthData,
             AuthDataSize,
             &FoundCert,
             &FoundCertSize
             );
  if (EFI_ERROR (Status)) {
    FreePool (AuthData);
    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  UT_ASSERT_NOT_NULL (FoundCert);
  UT_ASSERT_EQUAL (FoundCertSize, sizeof (mTrustAnchorTestCert));
  UT_ASSERT_MEM_EQUAL (FoundCert, mTrustAnchorTestCert, sizeof (mTrustAnchorTestCert));

  FreePool (FoundCert);
  FreePool (AuthData);
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorFindSha256Bare (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunFindCertCase (SHA256_DIGEST_SIZE, FALSE);
}

UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorFindSha256Wrapped (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunFindCertCase (SHA256_DIGEST_SIZE, TRUE);
}

UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorFindSha1 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunFindCertCase (SHA1_DIGEST_SIZE, FALSE);
}

UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorFindSha384 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunFindCertCase (SHA384_DIGEST_SIZE, FALSE);
}

UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorFindSha512 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunFindCertCase (SHA512_DIGEST_SIZE, FALSE);
}

/**
  Hash mismatch must produce EFI_NOT_FOUND, not a false positive.
**/
UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorHashMismatch (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS   Status;
  UINT8        *AuthData;
  UINTN        AuthDataSize;
  UINT8        BadHash[SHA256_DIGEST_SIZE];
  UINT8        *FoundCert;
  UINTN        FoundCertSize;
  CONST UINT8  *Cert    = mTrustAnchorTestCert;
  UINTN        CertSize = sizeof (mTrustAnchorTestCert);

  AuthData     = NULL;
  AuthDataSize = 0;
  FoundCert    = NULL;
  SetMem (BadHash, sizeof (BadHash), 0xAB);

  Status = BuildSignedData (&Cert, &CertSize, 1, FALSE, &AuthData, &AuthDataSize);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = GetTrustAnchorX509FromAuthData (
             NULL,
             BadHash,
             sizeof (BadHash),
             AuthData,
             AuthDataSize,
             &FoundCert,
             &FoundCertSize
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);
  UT_ASSERT_TRUE (FoundCert == NULL);

  FreePool (AuthData);
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorBadParameters (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Hash[SHA256_DIGEST_SIZE] = { 0 };
  UINT8       AuthData[]               = { 0x30, 0x00 };
  UINT8       *FoundCert               = NULL;
  UINTN       FoundCertSize            = 0;

  //
  // NULL hash
  //
  Status = GetTrustAnchorX509FromAuthData (NULL, NULL, sizeof (Hash), AuthData, sizeof (AuthData), &FoundCert, &FoundCertSize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // Zero hash size
  //
  Status = GetTrustAnchorX509FromAuthData (NULL, Hash, 0, AuthData, sizeof (AuthData), &FoundCert, &FoundCertSize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // NULL AuthData
  //
  Status = GetTrustAnchorX509FromAuthData (NULL, Hash, sizeof (Hash), NULL, sizeof (AuthData), &FoundCert, &FoundCertSize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // Zero AuthDataSize
  //
  Status = GetTrustAnchorX509FromAuthData (NULL, Hash, sizeof (Hash), AuthData, 0, &FoundCert, &FoundCertSize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // NULL output pointer
  //
  Status = GetTrustAnchorX509FromAuthData (NULL, Hash, sizeof (Hash), AuthData, sizeof (AuthData), NULL, &FoundCertSize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  Status = GetTrustAnchorX509FromAuthData (NULL, Hash, sizeof (Hash), AuthData, sizeof (AuthData), &FoundCert, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // Unsupported hash size
  //
  Status = GetTrustAnchorX509FromAuthData (NULL, Hash, 33, AuthData, sizeof (AuthData), &FoundCert, &FoundCertSize);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

/**
  Truncated AuthData (an empty SEQUENCE) must produce a non-success
  return without dereferencing past the input.
**/
UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorTruncatedAuthData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Hash[SHA256_DIGEST_SIZE] = { 0 };
  //
  // SEQUENCE { } -- nothing inside, will fail when LocateCertificatesField
  // tries to read version.
  //
  UINT8  AuthData[]    = { 0x30, 0x00 };
  UINT8  *FoundCert    = NULL;
  UINTN  FoundCertSize = 0;

  Status = GetTrustAnchorX509FromAuthData (
             NULL,
             Hash,
             sizeof (Hash),
             AuthData,
             sizeof (AuthData),
             &FoundCert,
             &FoundCertSize
             );
  UT_ASSERT_TRUE (EFI_ERROR (Status));
  UT_ASSERT_TRUE (FoundCert == NULL);

  return UNIT_TEST_PASSED;
}

/**
  Cache reuse: two back-to-back calls with the same handle must succeed
  without leaking, and FreeTrustAnchorX509Cache() must accept the
  populated handle.
**/
UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorCacheReuse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS   Status;
  UINT8        *AuthData;
  UINTN        AuthDataSize;
  UINT8        Expected[SHA256_DIGEST_SIZE];
  UINT8        *FoundCert1;
  UINT8        *FoundCert2;
  UINTN        FoundCertSize1;
  UINTN        FoundCertSize2;
  VOID         *CacheHandle = NULL;
  CONST UINT8  *Cert        = mTrustAnchorTestCert;
  UINTN        CertSize     = sizeof (mTrustAnchorTestCert);

  AuthData     = NULL;
  AuthDataSize = 0;
  FoundCert1   = NULL;
  FoundCert2   = NULL;

  UT_ASSERT_TRUE (ComputeExpectedTbsHash (SHA256_DIGEST_SIZE, Expected));

  Status = BuildSignedData (&Cert, &CertSize, 1, FALSE, &AuthData, &AuthDataSize);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // First call: cache is allocated by the function.
  //
  Status = GetTrustAnchorX509FromAuthData (
             &CacheHandle,
             Expected,
             SHA256_DIGEST_SIZE,
             AuthData,
             AuthDataSize,
             &FoundCert1,
             &FoundCertSize1
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (CacheHandle);
  UT_ASSERT_NOT_NULL (FoundCert1);

  //
  // Second call: same handle, must hit the cache and still succeed.
  //
  Status = GetTrustAnchorX509FromAuthData (
             &CacheHandle,
             Expected,
             SHA256_DIGEST_SIZE,
             AuthData,
             AuthDataSize,
             &FoundCert2,
             &FoundCertSize2
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (FoundCert2);
  UT_ASSERT_EQUAL (FoundCertSize2, FoundCertSize1);
  UT_ASSERT_MEM_EQUAL (FoundCert2, FoundCert1, FoundCertSize1);

  FreeTrustAnchorX509Cache (CacheHandle);
  FreePool (FoundCert1);
  FreePool (FoundCert2);
  FreePool (AuthData);
  return UNIT_TEST_PASSED;
}

/**
  FreeTrustAnchorX509Cache(NULL) must be a no-op (mirrors FreePool style).
**/
UNIT_TEST_STATUS
EFIAPI
TestTrustAnchorFreeNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  FreeTrustAnchorX509Cache (NULL);
  return UNIT_TEST_PASSED;
}

TEST_DESC  mTrustAnchorTest[] = {
  //
  // -----Description--------------------------------------Class----------------------------Function---------------------Pre--Post-Context
  //
  { "Find by SHA256 (bare SignedData)",        "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorFindSha256Bare,    NULL, NULL, NULL },
  { "Find by SHA256 (ContentInfo wrapper)",    "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorFindSha256Wrapped, NULL, NULL, NULL },
  { "Find by SHA1",                            "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorFindSha1,          NULL, NULL, NULL },
  { "Find by SHA384",                          "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorFindSha384,        NULL, NULL, NULL },
  { "Find by SHA512",                          "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorFindSha512,        NULL, NULL, NULL },
  { "Hash mismatch returns EFI_NOT_FOUND",     "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorHashMismatch,      NULL, NULL, NULL },
  { "Bad parameters return INVALID_PARAMETER", "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorBadParameters,     NULL, NULL, NULL },
  { "Truncated AuthData rejected",             "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorTruncatedAuthData, NULL, NULL, NULL },
  { "Cache reuse across calls",                "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorCacheReuse,        NULL, NULL, NULL },
  { "FreeTrustAnchorX509Cache(NULL) is safe",  "CryptoPkg.BaseCryptLib.TrustAnchor", TestTrustAnchorFreeNull,          NULL, NULL, NULL },
};

UINTN  mTrustAnchorTestNum = ARRAY_SIZE (mTrustAnchorTest);
