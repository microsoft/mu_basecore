/** @file
  Tests for extended Authenticode verification and verified chain output.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"

extern UINT8        AuthenticodeWithSha256[];
extern UINT8        TestRootCert2[];
extern UINT8        PeSha256Hash[];
extern CONST UINTN  AuthenticodeWithSha256Size;
extern CONST UINTN  TestRootCert2Size;

/**
  Correctness test for AuthenticodeVerifyEx(): it must verify the image AND
  hand back the *cryptographically-verified* signer chain in one pass. We
  assert the returned EFI_CERT_STACK is well-formed and **terminates
  at the supplied trust anchor's subject**. Also verifies that an invalid
  image hash does not return a chain.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyAuthenticodeVerifyEx (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS   Status;
  UINT8        *Chain;
  UINTN        ChainSize;
  UINTN        Offset;
  UINTN        Index;
  UINT8        Count;
  UINT32       Len;
  CONST UINT8  *FirstCert;
  UINT32       FirstLen;
  CONST UINT8  *LastCert;
  UINT32       LastLen;
  UINT8        *Chain2;
  UINTN        ChainSize2;
  UINT8        BadHash[SHA256_DIGEST_SIZE];
  UINT8        SubjFirst[512];
  UINTN        SubjFirstLen;
  UINT8        SubjLast[512];
  UINTN        SubjLastLen;
  UINT8        SubjRoot[512];
  UINTN        SubjRootLen;

  //
  // Exercise the SHA-256 sample, which chains to TestRootCert2.
  //
  //
  // 1. The chain output is optional - NULL out-params still verify.
  //
  Status = AuthenticodeVerifyEx (
             AuthenticodeWithSha256,
             AuthenticodeWithSha256Size,
             TestRootCert2,
             TestRootCert2Size,
             PeSha256Hash,
             SHA256_DIGEST_SIZE,
             NULL,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  //
  // 2. Verify AND return the chain in a single pass.
  //
  Chain     = NULL;
  ChainSize = 0;
  Status    = AuthenticodeVerifyEx (
                AuthenticodeWithSha256,
                AuthenticodeWithSha256Size,
                TestRootCert2,
                TestRootCert2Size,
                PeSha256Hash,
                SHA256_DIGEST_SIZE,
                &Chain,
                &ChainSize
                );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_NOT_NULL (Chain);
  UT_ASSERT_TRUE (ChainSize >= 1);

  //
  // Walk the EFI_CERT_STACK (UINT8 count; { UINT32 len; DER } x count).
  // Confirm it is well-formed, then compare the last entry's subject to
  // the supplied trust anchor.
  //
  Count = Chain[0];
  UT_ASSERT_TRUE (Count >= 2);

  Offset    = 1;
  FirstCert = NULL;
  FirstLen  = 0;
  LastCert  = NULL;
  LastLen   = 0;
  for (Index = 0; Index < Count; Index++) {
    UT_ASSERT_TRUE (Offset + sizeof (UINT32) <= ChainSize);
    Len     = ReadUnaligned32 ((CONST UINT32 *)(Chain + Offset));
    Offset += sizeof (UINT32);
    UT_ASSERT_TRUE (Offset + Len <= ChainSize);
    if (Index == 0) {
      FirstCert = Chain + Offset;
      FirstLen  = Len;
    }

    LastCert = Chain + Offset;
    LastLen  = Len;
    Offset  += Len;
  }

  UT_ASSERT_EQUAL (Offset, ChainSize);

  //
  // The last entry must be our trust anchor. Compare by subject
  // Distinguished Name, which is stable across OpenSSL's DER re-encoding of
  // the outer certificate (the raw bytes can differ by a length octet).
  //
  SubjLastLen = sizeof (SubjLast);
  SubjRootLen = sizeof (SubjRoot);
  UT_ASSERT_TRUE (X509GetSubjectName (LastCert, LastLen, SubjLast, &SubjLastLen));
  UT_ASSERT_TRUE (X509GetSubjectName (TestRootCert2, TestRootCert2Size, SubjRoot, &SubjRootLen));
  UT_ASSERT_EQUAL (SubjLastLen, SubjRootLen);
  UT_ASSERT_TRUE (CompareMem (SubjLast, SubjRoot, SubjLastLen) == 0);

  //
  // The FIRST entry is the signer (leaf): it must parse as a valid cert and,
  // for a real chain, its subject differs from the trust anchor's - so the
  // stack is ordered signer(0)..anchor(N-1), not degenerate.
  //
  SubjFirstLen = sizeof (SubjFirst);
  UT_ASSERT_TRUE (X509GetSubjectName (FirstCert, FirstLen, SubjFirst, &SubjFirstLen));
  UT_ASSERT_TRUE (
    (SubjFirstLen != SubjRootLen) ||
    (CompareMem (SubjFirst, SubjRoot, SubjFirstLen) != 0)
    );

  //
  // 2b. Repeatability / no cross-call state bleed: a second call with the
  //     same inputs must independently return a byte-identical chain. This
  //     is the functional proof that the per-call capture context (attached
  //     via X509_STORE ex_data) carries no state between verifications.
  //
  Chain2     = NULL;
  ChainSize2 = 0;
  Status     = AuthenticodeVerifyEx (
                 AuthenticodeWithSha256,
                 AuthenticodeWithSha256Size,
                 TestRootCert2,
                 TestRootCert2Size,
                 PeSha256Hash,
                 SHA256_DIGEST_SIZE,
                 &Chain2,
                 &ChainSize2
                 );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_NOT_NULL (Chain2);
  UT_ASSERT_EQUAL (ChainSize2, ChainSize);
  UT_ASSERT_TRUE (CompareMem (Chain2, Chain, ChainSize) == 0);
  FreePool (Chain2);

  FreePool (Chain);

  //
  // 3. Invalid parameters are rejected, and no chain is produced.
  //
  Chain  = NULL;
  Status = AuthenticodeVerifyEx (
             NULL,
             AuthenticodeWithSha256Size,
             TestRootCert2,
             TestRootCert2Size,
             PeSha256Hash,
             SHA256_DIGEST_SIZE,
             &Chain,
             &ChainSize
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
  UT_ASSERT_TRUE (Chain == NULL);

  //
  // 4. A corrupted image hash is rejected (EFI_SECURITY_VIOLATION) and the
  //    chain is NEVER returned for an invalid signature.
  //
  CopyMem (BadHash, PeSha256Hash, SHA256_DIGEST_SIZE);
  BadHash[0] ^= 0xFF;
  Chain       = NULL;
  Status      = AuthenticodeVerifyEx (
                  AuthenticodeWithSha256,
                  AuthenticodeWithSha256Size,
                  TestRootCert2,
                  TestRootCert2Size,
                  BadHash,
                  SHA256_DIGEST_SIZE,
                  &Chain,
                  &ChainSize
                  );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SECURITY_VIOLATION);
  UT_ASSERT_TRUE (Chain == NULL);

  //
  // 5. The two chain out-params are independently OPTIONAL. Supplying only
  //    one of the pair must still verify (EFI_SUCCESS) and write no chain -
  //    and must not leak the internally captured chain.
  //
  Chain  = NULL;
  Status = AuthenticodeVerifyEx (
             AuthenticodeWithSha256,
             AuthenticodeWithSha256Size,
             TestRootCert2,
             TestRootCert2Size,
             PeSha256Hash,
             SHA256_DIGEST_SIZE,
             &Chain,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_TRUE (Chain == NULL);

  ChainSize = 0;
  Status    = AuthenticodeVerifyEx (
                AuthenticodeWithSha256,
                AuthenticodeWithSha256Size,
                TestRootCert2,
                TestRootCert2Size,
                PeSha256Hash,
                SHA256_DIGEST_SIZE,
                NULL,
                &ChainSize
                );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_EQUAL (ChainSize, 0);

  return UNIT_TEST_PASSED;
}

/**
  Verifies the "signer is the trust anchor" case for AuthenticodeVerifyEx().

  The returned chain is the trust path: the image signer up to and including
  the trust anchor. When the certificate that signed the image is itself
  supplied as the trust anchor, that path is exactly one certificate (the
  signer/anchor) - the chain is NOT extended past the anchor with the other
  certificates embedded in the message. This test obtains the signer (leaf)
  from a normal verification, re-verifies with it as the trust anchor, and
  asserts a single-certificate chain equal to the signer.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyAuthenticodeVerifyExSignerIsAnchor (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       *Chain;
  UINTN       ChainSize;
  UINT8       *SignerCert;
  UINT32      SignerLen;
  UINTN       Offset;
  UINT32      Len;
  UINT8       SubjChain[512];
  UINTN       SubjChainLen;
  UINT8       SubjSigner[512];
  UINTN       SubjSignerLen;

  //
  // Only exercise the SHA-256 sample (chains to TestRootCert2).
  //
  //
  // 1. Normal verification to obtain the signer (leaf) certificate, which is
  //    entry 0 of the returned chain. Copy it out before freeing the chain.
  //
  Chain     = NULL;
  ChainSize = 0;
  Status    = AuthenticodeVerifyEx (
                AuthenticodeWithSha256,
                AuthenticodeWithSha256Size,
                TestRootCert2,
                TestRootCert2Size,
                PeSha256Hash,
                SHA256_DIGEST_SIZE,
                &Chain,
                &ChainSize
                );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_NOT_NULL (Chain);
  UT_ASSERT_TRUE (Chain[0] >= 1);
  UT_ASSERT_TRUE (1 + sizeof (UINT32) <= ChainSize);

  SignerLen = ReadUnaligned32 ((CONST UINT32 *)(Chain + 1));
  UT_ASSERT_TRUE ((UINTN)(1 + sizeof (UINT32) + SignerLen) <= ChainSize);
  SignerCert = AllocatePool (SignerLen);
  UT_ASSERT_NOT_NULL (SignerCert);
  CopyMem (SignerCert, Chain + 1 + sizeof (UINT32), SignerLen);
  FreePool (Chain);

  //
  // 2. Re-verify with the signer's own certificate as the trust anchor
  //    (signer == anchor). The trust path is a single certificate.
  //
  Chain     = NULL;
  ChainSize = 0;
  Status    = AuthenticodeVerifyEx (
                AuthenticodeWithSha256,
                AuthenticodeWithSha256Size,
                SignerCert,
                SignerLen,
                PeSha256Hash,
                SHA256_DIGEST_SIZE,
                &Chain,
                &ChainSize
                );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);
  UT_ASSERT_NOT_NULL (Chain);

  //
  // Exactly one certificate: the signer, which is also the anchor. The chain
  // is not extended past the anchor with the message's other certificates.
  //
  UT_ASSERT_EQUAL (Chain[0], 1);
  Offset  = 1;
  Len     = ReadUnaligned32 ((CONST UINT32 *)(Chain + Offset));
  Offset += sizeof (UINT32);
  UT_ASSERT_TRUE (Offset + Len <= ChainSize);
  UT_ASSERT_EQUAL (Offset + Len, ChainSize);

  //
  // The single certificate is the signer we supplied as the anchor.
  //
  SubjChainLen  = sizeof (SubjChain);
  SubjSignerLen = sizeof (SubjSigner);
  UT_ASSERT_TRUE (X509GetSubjectName (Chain + Offset, Len, SubjChain, &SubjChainLen));
  UT_ASSERT_TRUE (X509GetSubjectName (SignerCert, SignerLen, SubjSigner, &SubjSignerLen));
  UT_ASSERT_EQUAL (SubjChainLen, SubjSignerLen);
  UT_ASSERT_TRUE (CompareMem (SubjChain, SubjSigner, SubjChainLen) == 0);

  FreePool (Chain);
  FreePool (SignerCert);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mAuthenticodeVerifyExTest[] = {
  { "Verify and return the signer chain",       "CryptoPkg.BaseCryptLib.AuthenticodeVerifyEx", TestVerifyAuthenticodeVerifyEx,               NULL, NULL, NULL },
  { "Stop at the supplied signer trust anchor", "CryptoPkg.BaseCryptLib.AuthenticodeVerifyEx", TestVerifyAuthenticodeVerifyExSignerIsAnchor, NULL, NULL, NULL }
};

UINTN  mAuthenticodeVerifyExTestNum = ARRAY_SIZE (mAuthenticodeVerifyExTest);
