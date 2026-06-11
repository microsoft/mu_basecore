/** @file
  Unit tests for GetAuthenticodeHashAlgorithm().

  The tests build a minimal but well-formed PKCS#7 SignedData blob whose
  encapContentInfo carries an SpcIndirectDataContent
  (OID 1.3.6.1.4.1.311.2.1.4) with a chosen digestAlgorithm OID, then
  assert that GetAuthenticodeHashAlgorithm() recovers the matching
  signature-type GUID. All four supported digest algorithms are covered,
  along with bad-parameter, malformed-input, wrong-content-type, and
  unsupported-algorithm cases.

  The tests depend only on public BaseCryptLib API, so they run
  identically against the OpenSSL and MbedTLS BaseCryptLib instances.

Copyright (C) Microsoft Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"
#include <Guid/ImageAuthentication.h>

//
// Digest size (in bytes) of the dummy messageDigest OCTET STRING. The
// value is irrelevant to GetAuthenticodeHashAlgorithm(); only the
// digestAlgorithm OID is examined.
//
#define AUTH_ALG_TEST_DIGEST_SIZE  32

//
// OID value bytes (no tag/length) for the structures we build.
//
STATIC CONST UINT8  mAlgPkcs7SignedDataOid[] = {
  0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02  // 1.2.840.113549.1.7.2
};
STATIC CONST UINT8  mAlgPkcs7DataOid[] = {
  0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x01  // 1.2.840.113549.1.7.1
};
STATIC CONST UINT8  mAlgSpcIndirectDataOid[] = {
  0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x01, 0x04  // 1.3.6.1.4.1.311.2.1.4
};
STATIC CONST UINT8  mAlgOidSha1[] = {
  0x2B, 0x0E, 0x03, 0x02, 0x1A                          // 1.3.14.3.2.26
};
STATIC CONST UINT8  mAlgOidSha256[] = {
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01  // 2.16.840.1.101.3.4.2.1
};
STATIC CONST UINT8  mAlgOidSha384[] = {
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02  // 2.16.840.1.101.3.4.2.2
};
STATIC CONST UINT8  mAlgOidSha512[] = {
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03  // 2.16.840.1.101.3.4.2.3
};
STATIC CONST UINT8  mAlgOidMd5[] = {
  0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05        // 1.2.840.113549.2.5
};

//
// Expected signature-type GUIDs, materialized locally so the test does
// not depend on the gEfiCert*Guid link symbols.
//
STATIC CONST EFI_GUID  mAlgExpectSha1Guid   = EFI_CERT_SHA1_GUID;
STATIC CONST EFI_GUID  mAlgExpectSha256Guid = EFI_CERT_SHA256_GUID;
STATIC CONST EFI_GUID  mAlgExpectSha384Guid = EFI_CERT_SHA384_GUID;
STATIC CONST EFI_GUID  mAlgExpectSha512Guid = EFI_CERT_SHA512_GUID;

//
// ASN.1 DER tag bytes.
//
#define ALG_ASN1_TAG_OCTET_STRING  0x04
#define ALG_ASN1_TAG_OID           0x06
#define ALG_ASN1_TAG_SEQUENCE      0x30
#define ALG_ASN1_TAG_SET           0x31
#define ALG_ASN1_TAG_CTX_CONS_0    0xA0

/**
  Encode a DER definite-length field. Writes 1..3 bytes to Out and
  returns the number written. Only lengths up to 0xFFFF are needed by
  these tests.

  @param[in]  Length  Length to encode.
  @param[out] Out     Receives 1..3 bytes.

  @return Number of bytes written.
**/
STATIC
UINTN
AlgDerEncodeLength (
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

  Out[0] = 0x82;
  Out[1] = (UINT8)((Length >> 8) & 0xFF);
  Out[2] = (UINT8)(Length & 0xFF);
  return 3;
}

/**
  Allocate a new buffer holding Tag || DER-length || Content.

  @param[in]  Tag         The ASN.1 tag byte.
  @param[in]  Content     Pointer to the value bytes (may be NULL when
                          ContentLen is 0).
  @param[in]  ContentLen  Number of value bytes.
  @param[out] OutLen      Receives the size of the returned buffer.

  @return Newly allocated TLV buffer, or NULL on allocation failure. The
          caller is responsible for freeing it.
**/
STATIC
UINT8 *
AlgTlv (
  IN  UINT8        Tag,
  IN  CONST UINT8  *Content,
  IN  UINTN        ContentLen,
  OUT UINTN        *OutLen
  )
{
  UINT8  LenScratch[3];
  UINTN  LenBytes;
  UINTN  Total;
  UINT8  *Buf;

  LenBytes = AlgDerEncodeLength (ContentLen, LenScratch);
  Total    = 1 + LenBytes + ContentLen;
  Buf      = AllocatePool (Total);
  if (Buf == NULL) {
    *OutLen = 0;
    return NULL;
  }

  Buf[0] = Tag;
  CopyMem (Buf + 1, LenScratch, LenBytes);
  if (ContentLen != 0) {
    CopyMem (Buf + 1 + LenBytes, Content, ContentLen);
  }

  *OutLen = Total;
  return Buf;
}

/**
  Allocate a new buffer holding the concatenation of two buffers.

  @param[in]  A       First buffer (may be NULL when ALen is 0).
  @param[in]  ALen    Size of the first buffer.
  @param[in]  B       Second buffer (may be NULL when BLen is 0).
  @param[in]  BLen    Size of the second buffer.
  @param[out] OutLen  Receives the size of the returned buffer.

  @return Newly allocated buffer, or NULL on allocation failure. The
          caller is responsible for freeing it.
**/
STATIC
UINT8 *
AlgCat (
  IN  CONST UINT8  *A,
  IN  UINTN        ALen,
  IN  CONST UINT8  *B,
  IN  UINTN        BLen,
  OUT UINTN        *OutLen
  )
{
  UINT8  *Buf;

  Buf = AllocatePool (ALen + BLen);
  if (Buf == NULL) {
    *OutLen = 0;
    return NULL;
  }

  if (ALen != 0) {
    CopyMem (Buf, A, ALen);
  }

  if (BLen != 0) {
    CopyMem (Buf + ALen, B, BLen);
  }

  *OutLen = ALen + BLen;
  return Buf;
}

/**
  Build a minimal Authenticode PKCS#7 SignedData blob.

    ContentInfo ::= SEQUENCE {
      contentType  OID  = OuterOid,
      content [0]  EXPLICIT SignedData }
    SignedData ::= SEQUENCE {
      version           INTEGER 1,
      digestAlgorithms  SET (empty),
      encapContentInfo  SEQUENCE {
        contentType  OID  = EncapOid,
        content [0]  EXPLICIT SpcIndirectDataContent } }
    SpcIndirectDataContent ::= SEQUENCE {
      data           SEQUENCE (empty),
      messageDigest  DigestInfo ::= SEQUENCE {
        digestAlgorithm  AlgorithmIdentifier ::= SEQUENCE { algorithm OID = AlgOid },
        digest           OCTET STRING (AUTH_ALG_TEST_DIGEST_SIZE zero bytes) } }

  Pass mAlgPkcs7SignedDataOid / mAlgSpcIndirectDataOid for a valid blob,
  or substitute other OIDs to exercise the rejection paths.

  @param[in]  OuterOid     contentType OID value bytes for ContentInfo.
  @param[in]  OuterOidLen  Length of OuterOid.
  @param[in]  EncapOid     contentType OID value bytes for encapContentInfo.
  @param[in]  EncapOidLen  Length of EncapOid.
  @param[in]  AlgOid       digestAlgorithm OID value bytes.
  @param[in]  AlgOidLen    Length of AlgOid.
  @param[out] OutBuf       Receives the allocated blob.
  @param[out] OutSize      Receives the blob size.

  @retval EFI_SUCCESS           Blob was constructed.
  @retval EFI_OUT_OF_RESOURCES  Allocation failed.
**/
STATIC
EFI_STATUS
BuildAuthenticodeBlob (
  IN  CONST UINT8  *OuterOid,
  IN  UINTN        OuterOidLen,
  IN  CONST UINT8  *EncapOid,
  IN  UINTN        EncapOidLen,
  IN  CONST UINT8  *AlgOid,
  IN  UINTN        AlgOidLen,
  OUT UINT8        **OutBuf,
  OUT UINTN        *OutSize
  )
{
  EFI_STATUS  Status;
  UINT8       Zero[AUTH_ALG_TEST_DIGEST_SIZE];
  UINT8       *AlgOidTlv;
  UINT8       *DigestAlg;
  UINT8       *DigestStr;
  UINT8       *DigestInfoBody;
  UINT8       *DigestInfo;
  UINT8       *Data;
  UINT8       *SpcBody;
  UINT8       *SpcIndirect;
  UINT8       *SpcExplicit;
  UINT8       *EncapOidTlv;
  UINT8       *EncapBody;
  UINT8       *Encap;
  UINT8       *Version;
  UINT8       *DigestAlgsSet;
  UINT8       *SdBody0;
  UINT8       *SdBody;
  UINT8       *SignedData;
  UINT8       *SdExplicit;
  UINT8       *OuterOidTlv;
  UINT8       *CiBody;
  UINT8       *ContentInfo;
  UINTN       AlgOidTlvLen;
  UINTN       DigestAlgLen;
  UINTN       DigestStrLen;
  UINTN       DigestInfoBodyLen;
  UINTN       DigestInfoLen;
  UINTN       DataLen;
  UINTN       SpcBodyLen;
  UINTN       SpcIndirectLen;
  UINTN       SpcExplicitLen;
  UINTN       EncapOidTlvLen;
  UINTN       EncapBodyLen;
  UINTN       EncapLen;
  UINTN       VersionLen;
  UINTN       DigestAlgsSetLen;
  UINTN       SdBody0Len;
  UINTN       SdBodyLen;
  UINTN       SignedDataLen;
  UINTN       SdExplicitLen;
  UINTN       OuterOidTlvLen;
  UINTN       CiBodyLen;
  UINTN       ContentInfoLen;

  Status         = EFI_OUT_OF_RESOURCES;
  AlgOidTlv      = NULL;
  DigestAlg      = NULL;
  DigestStr      = NULL;
  DigestInfoBody = NULL;
  DigestInfo     = NULL;
  Data           = NULL;
  SpcBody        = NULL;
  SpcIndirect    = NULL;
  SpcExplicit    = NULL;
  EncapOidTlv    = NULL;
  EncapBody      = NULL;
  Encap          = NULL;
  Version        = NULL;
  DigestAlgsSet  = NULL;
  SdBody0        = NULL;
  SdBody         = NULL;
  SignedData     = NULL;
  SdExplicit     = NULL;
  OuterOidTlv    = NULL;
  CiBody         = NULL;
  ContentInfo    = NULL;

  ZeroMem (Zero, sizeof (Zero));

  //
  //   digestAlgorithm AlgorithmIdentifier ::= SEQUENCE { algorithm OID }
  //
  AlgOidTlv = AlgTlv (ALG_ASN1_TAG_OID, AlgOid, AlgOidLen, &AlgOidTlvLen);
  if (AlgOidTlv == NULL) {
    goto Done;
  }

  DigestAlg = AlgTlv (ALG_ASN1_TAG_SEQUENCE, AlgOidTlv, AlgOidTlvLen, &DigestAlgLen);
  if (DigestAlg == NULL) {
    goto Done;
  }

  //
  //   digest OCTET STRING
  //
  DigestStr = AlgTlv (ALG_ASN1_TAG_OCTET_STRING, Zero, sizeof (Zero), &DigestStrLen);
  if (DigestStr == NULL) {
    goto Done;
  }

  //
  //   messageDigest DigestInfo ::= SEQUENCE { digestAlgorithm, digest }
  //
  DigestInfoBody = AlgCat (DigestAlg, DigestAlgLen, DigestStr, DigestStrLen, &DigestInfoBodyLen);
  if (DigestInfoBody == NULL) {
    goto Done;
  }

  DigestInfo = AlgTlv (ALG_ASN1_TAG_SEQUENCE, DigestInfoBody, DigestInfoBodyLen, &DigestInfoLen);
  if (DigestInfo == NULL) {
    goto Done;
  }

  //
  //   data SpcAttributeTypeAndOptionalValue SEQUENCE (empty, skipped)
  //
  Data = AlgTlv (ALG_ASN1_TAG_SEQUENCE, NULL, 0, &DataLen);
  if (Data == NULL) {
    goto Done;
  }

  //
  //   SpcIndirectDataContent ::= SEQUENCE { data, messageDigest }
  //
  SpcBody = AlgCat (Data, DataLen, DigestInfo, DigestInfoLen, &SpcBodyLen);
  if (SpcBody == NULL) {
    goto Done;
  }

  SpcIndirect = AlgTlv (ALG_ASN1_TAG_SEQUENCE, SpcBody, SpcBodyLen, &SpcIndirectLen);
  if (SpcIndirect == NULL) {
    goto Done;
  }

  //
  //   content [0] EXPLICIT -> SpcIndirectDataContent
  //
  SpcExplicit = AlgTlv (ALG_ASN1_TAG_CTX_CONS_0, SpcIndirect, SpcIndirectLen, &SpcExplicitLen);
  if (SpcExplicit == NULL) {
    goto Done;
  }

  //
  //   encapContentInfo ::= SEQUENCE { contentType OID, content [0] EXPLICIT }
  //
  EncapOidTlv = AlgTlv (ALG_ASN1_TAG_OID, EncapOid, EncapOidLen, &EncapOidTlvLen);
  if (EncapOidTlv == NULL) {
    goto Done;
  }

  EncapBody = AlgCat (EncapOidTlv, EncapOidTlvLen, SpcExplicit, SpcExplicitLen, &EncapBodyLen);
  if (EncapBody == NULL) {
    goto Done;
  }

  Encap = AlgTlv (ALG_ASN1_TAG_SEQUENCE, EncapBody, EncapBodyLen, &EncapLen);
  if (Encap == NULL) {
    goto Done;
  }

  //
  // SignedData ::= SEQUENCE { version, digestAlgorithms, encapContentInfo }
  //
  Version = AlgTlv (0x02, (CONST UINT8 *)"\x01", 1, &VersionLen);  // INTEGER 1
  if (Version == NULL) {
    goto Done;
  }

  DigestAlgsSet = AlgTlv (ALG_ASN1_TAG_SET, NULL, 0, &DigestAlgsSetLen);  // empty SET
  if (DigestAlgsSet == NULL) {
    goto Done;
  }

  SdBody0 = AlgCat (Version, VersionLen, DigestAlgsSet, DigestAlgsSetLen, &SdBody0Len);
  if (SdBody0 == NULL) {
    goto Done;
  }

  SdBody = AlgCat (SdBody0, SdBody0Len, Encap, EncapLen, &SdBodyLen);
  if (SdBody == NULL) {
    goto Done;
  }

  SignedData = AlgTlv (ALG_ASN1_TAG_SEQUENCE, SdBody, SdBodyLen, &SignedDataLen);
  if (SignedData == NULL) {
    goto Done;
  }

  //
  // content [0] EXPLICIT -> SignedData
  //
  SdExplicit = AlgTlv (ALG_ASN1_TAG_CTX_CONS_0, SignedData, SignedDataLen, &SdExplicitLen);
  if (SdExplicit == NULL) {
    goto Done;
  }

  //
  // ContentInfo ::= SEQUENCE { contentType OID, content [0] EXPLICIT }
  //
  OuterOidTlv = AlgTlv (ALG_ASN1_TAG_OID, OuterOid, OuterOidLen, &OuterOidTlvLen);
  if (OuterOidTlv == NULL) {
    goto Done;
  }

  CiBody = AlgCat (OuterOidTlv, OuterOidTlvLen, SdExplicit, SdExplicitLen, &CiBodyLen);
  if (CiBody == NULL) {
    goto Done;
  }

  ContentInfo = AlgTlv (ALG_ASN1_TAG_SEQUENCE, CiBody, CiBodyLen, &ContentInfoLen);
  if (ContentInfo == NULL) {
    goto Done;
  }

  *OutBuf     = ContentInfo;
  *OutSize    = ContentInfoLen;
  ContentInfo = NULL;     // ownership transferred to caller
  Status      = EFI_SUCCESS;

Done:
  if (AlgOidTlv != NULL) {
    FreePool (AlgOidTlv);
  }

  if (DigestAlg != NULL) {
    FreePool (DigestAlg);
  }

  if (DigestStr != NULL) {
    FreePool (DigestStr);
  }

  if (DigestInfoBody != NULL) {
    FreePool (DigestInfoBody);
  }

  if (DigestInfo != NULL) {
    FreePool (DigestInfo);
  }

  if (Data != NULL) {
    FreePool (Data);
  }

  if (SpcBody != NULL) {
    FreePool (SpcBody);
  }

  if (SpcIndirect != NULL) {
    FreePool (SpcIndirect);
  }

  if (SpcExplicit != NULL) {
    FreePool (SpcExplicit);
  }

  if (EncapOidTlv != NULL) {
    FreePool (EncapOidTlv);
  }

  if (EncapBody != NULL) {
    FreePool (EncapBody);
  }

  if (Encap != NULL) {
    FreePool (Encap);
  }

  if (Version != NULL) {
    FreePool (Version);
  }

  if (DigestAlgsSet != NULL) {
    FreePool (DigestAlgsSet);
  }

  if (SdBody0 != NULL) {
    FreePool (SdBody0);
  }

  if (SdBody != NULL) {
    FreePool (SdBody);
  }

  if (SignedData != NULL) {
    FreePool (SignedData);
  }

  if (SdExplicit != NULL) {
    FreePool (SdExplicit);
  }

  if (OuterOidTlv != NULL) {
    FreePool (OuterOidTlv);
  }

  if (CiBody != NULL) {
    FreePool (CiBody);
  }

  if (ContentInfo != NULL) {
    FreePool (ContentInfo);
  }

  return Status;
}

/**
  Build a valid Authenticode blob for AlgOid and assert that
  GetAuthenticodeHashAlgorithm() returns ExpectGuid.

  @param[in]  AlgOid      digestAlgorithm OID value bytes.
  @param[in]  AlgOidLen   Length of AlgOid.
  @param[in]  ExpectGuid  Expected signature-type GUID.

  @retval UNIT_TEST_PASSED on success (asserts otherwise).
**/
STATIC
UNIT_TEST_STATUS
RunPositiveAlgCase (
  IN  CONST UINT8     *AlgOid,
  IN  UINTN           AlgOidLen,
  IN  CONST EFI_GUID  *ExpectGuid
  )
{
  EFI_STATUS  Status;
  UINT8       *Blob;
  UINTN       BlobSize;
  EFI_GUID    HashType;

  Blob     = NULL;
  BlobSize = 0;

  Status = BuildAuthenticodeBlob (
             mAlgPkcs7SignedDataOid,
             sizeof (mAlgPkcs7SignedDataOid),
             mAlgSpcIndirectDataOid,
             sizeof (mAlgSpcIndirectDataOid),
             AlgOid,
             AlgOidLen,
             &Blob,
             &BlobSize
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_NOT_NULL (Blob);

  ZeroMem (&HashType, sizeof (HashType));
  Status = GetAuthenticodeHashAlgorithm (Blob, BlobSize, &HashType);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_TRUE (CompareGuid (&HashType, ExpectGuid));

  FreePool (Blob);
  return UNIT_TEST_PASSED;
}

/**
  SHA-1 digest algorithm resolves to gEfiCertSha1Guid.
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthAlgSha1 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPositiveAlgCase (mAlgOidSha1, sizeof (mAlgOidSha1), &mAlgExpectSha1Guid);
}

/**
  SHA-256 digest algorithm resolves to gEfiCertSha256Guid.
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthAlgSha256 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPositiveAlgCase (mAlgOidSha256, sizeof (mAlgOidSha256), &mAlgExpectSha256Guid);
}

/**
  SHA-384 digest algorithm resolves to gEfiCertSha384Guid.
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthAlgSha384 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPositiveAlgCase (mAlgOidSha384, sizeof (mAlgOidSha384), &mAlgExpectSha384Guid);
}

/**
  SHA-512 digest algorithm resolves to gEfiCertSha512Guid.
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthAlgSha512 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPositiveAlgCase (mAlgOidSha512, sizeof (mAlgOidSha512), &mAlgExpectSha512Guid);
}

/**
  NULL pointers and a zero size all yield EFI_INVALID_PARAMETER.
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthAlgBadParameters (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       *Blob;
  UINTN       BlobSize;
  EFI_GUID    HashType;

  Blob     = NULL;
  BlobSize = 0;

  Status = BuildAuthenticodeBlob (
             mAlgPkcs7SignedDataOid,
             sizeof (mAlgPkcs7SignedDataOid),
             mAlgSpcIndirectDataOid,
             sizeof (mAlgSpcIndirectDataOid),
             mAlgOidSha256,
             sizeof (mAlgOidSha256),
             &Blob,
             &BlobSize
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // NULL AuthData.
  //
  Status = GetAuthenticodeHashAlgorithm (NULL, BlobSize, &HashType);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // Zero AuthDataSize.
  //
  Status = GetAuthenticodeHashAlgorithm (Blob, 0, &HashType);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // NULL HashType output.
  //
  Status = GetAuthenticodeHashAlgorithm (Blob, BlobSize, NULL);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  FreePool (Blob);
  return UNIT_TEST_PASSED;
}

/**
  A blob truncated to fewer bytes than the encoded structure must be
  rejected with EFI_INVALID_PARAMETER rather than read out of bounds.
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthAlgTruncated (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       *Blob;
  UINTN       BlobSize;
  EFI_GUID    HashType;
  UINTN       Index;

  Blob     = NULL;
  BlobSize = 0;

  Status = BuildAuthenticodeBlob (
             mAlgPkcs7SignedDataOid,
             sizeof (mAlgPkcs7SignedDataOid),
             mAlgSpcIndirectDataOid,
             sizeof (mAlgSpcIndirectDataOid),
             mAlgOidSha256,
             sizeof (mAlgOidSha256),
             &Blob,
             &BlobSize
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_TRUE (BlobSize > 1);

  //
  // Every strict prefix of the blob is malformed and must be rejected.
  //
  for (Index = 1; Index < BlobSize; Index++) {
    Status = GetAuthenticodeHashAlgorithm (Blob, Index, &HashType);
    UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);
  }

  FreePool (Blob);
  return UNIT_TEST_PASSED;
}

/**
  A SignedData whose outer contentType is not pkcs7-signedData is
  rejected with EFI_INVALID_PARAMETER.
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthAlgWrongOuterContentType (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       *Blob;
  UINTN       BlobSize;
  EFI_GUID    HashType;

  Blob     = NULL;
  BlobSize = 0;

  Status = BuildAuthenticodeBlob (
             mAlgPkcs7DataOid,
             sizeof (mAlgPkcs7DataOid),
             mAlgSpcIndirectDataOid,
             sizeof (mAlgSpcIndirectDataOid),
             mAlgOidSha256,
             sizeof (mAlgOidSha256),
             &Blob,
             &BlobSize
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = GetAuthenticodeHashAlgorithm (Blob, BlobSize, &HashType);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  FreePool (Blob);
  return UNIT_TEST_PASSED;
}

/**
  An encapContentInfo whose contentType is not SpcIndirectData is
  rejected with EFI_INVALID_PARAMETER.
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthAlgWrongEncapContentType (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       *Blob;
  UINTN       BlobSize;
  EFI_GUID    HashType;

  Blob     = NULL;
  BlobSize = 0;

  Status = BuildAuthenticodeBlob (
             mAlgPkcs7SignedDataOid,
             sizeof (mAlgPkcs7SignedDataOid),
             mAlgPkcs7DataOid,
             sizeof (mAlgPkcs7DataOid),
             mAlgOidSha256,
             sizeof (mAlgOidSha256),
             &Blob,
             &BlobSize
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = GetAuthenticodeHashAlgorithm (Blob, BlobSize, &HashType);
  UT_ASSERT_EQUAL (Status, EFI_INVALID_PARAMETER);

  FreePool (Blob);
  return UNIT_TEST_PASSED;
}

/**
  A well-formed blob carrying an unrecognized digest algorithm (MD5) is
  rejected with EFI_UNSUPPORTED.
**/
UNIT_TEST_STATUS
EFIAPI
TestAuthAlgUnsupportedDigest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       *Blob;
  UINTN       BlobSize;
  EFI_GUID    HashType;

  Blob     = NULL;
  BlobSize = 0;

  Status = BuildAuthenticodeBlob (
             mAlgPkcs7SignedDataOid,
             sizeof (mAlgPkcs7SignedDataOid),
             mAlgSpcIndirectDataOid,
             sizeof (mAlgSpcIndirectDataOid),
             mAlgOidMd5,
             sizeof (mAlgOidMd5),
             &Blob,
             &BlobSize
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = GetAuthenticodeHashAlgorithm (Blob, BlobSize, &HashType);
  UT_ASSERT_EQUAL (Status, EFI_UNSUPPORTED);

  FreePool (Blob);
  return UNIT_TEST_PASSED;
}

TEST_DESC  mAuthenticodeHashAlgorithmTest[] = {
  //
  // -----Description-------------------------------------Class--------------------------------------------Function-------------------------Pre--Post-Context
  //
  { "Identify SHA1 digest algorithm",          "CryptoPkg.BaseCryptLib.AuthenticodeHashAlgorithm", TestAuthAlgSha1,                  NULL, NULL, NULL },
  { "Identify SHA256 digest algorithm",        "CryptoPkg.BaseCryptLib.AuthenticodeHashAlgorithm", TestAuthAlgSha256,                NULL, NULL, NULL },
  { "Identify SHA384 digest algorithm",        "CryptoPkg.BaseCryptLib.AuthenticodeHashAlgorithm", TestAuthAlgSha384,                NULL, NULL, NULL },
  { "Identify SHA512 digest algorithm",        "CryptoPkg.BaseCryptLib.AuthenticodeHashAlgorithm", TestAuthAlgSha512,                NULL, NULL, NULL },
  { "Bad parameters return INVALID_PARAMETER", "CryptoPkg.BaseCryptLib.AuthenticodeHashAlgorithm", TestAuthAlgBadParameters,         NULL, NULL, NULL },
  { "Truncated AuthData rejected",             "CryptoPkg.BaseCryptLib.AuthenticodeHashAlgorithm", TestAuthAlgTruncated,             NULL, NULL, NULL },
  { "Wrong outer contentType rejected",        "CryptoPkg.BaseCryptLib.AuthenticodeHashAlgorithm", TestAuthAlgWrongOuterContentType, NULL, NULL, NULL },
  { "Wrong encap contentType rejected",        "CryptoPkg.BaseCryptLib.AuthenticodeHashAlgorithm", TestAuthAlgWrongEncapContentType, NULL, NULL, NULL },
  { "Unsupported digest returns UNSUPPORTED",  "CryptoPkg.BaseCryptLib.AuthenticodeHashAlgorithm", TestAuthAlgUnsupportedDigest,     NULL, NULL, NULL },
};

UINTN  mAuthenticodeHashAlgorithmTestNum = ARRAY_SIZE (mAuthenticodeHashAlgorithmTest);
