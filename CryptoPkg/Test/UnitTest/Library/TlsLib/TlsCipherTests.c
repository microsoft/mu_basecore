/** @file
  TLS Cipher Suite Enumeration and Configuration Tests.

  Enumerate supported cipher suites, signature algorithms, and EC curves
  through the TLS API. Produces a structured report of TLS capabilities
  so CI logs capture what the built crypto binary supports.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestTlsLib.h"

//
// TLS EC Named Curve identifiers (from IndustryStandard/Tls1.h)
//
#define TLS_EC_NAMED_CURVE_SECP256R1  23
#define TLS_EC_NAMED_CURVE_SECP384R1  24
#define TLS_EC_NAMED_CURVE_SECP521R1  25
#define TLS_EC_NAMED_CURVE_X25519     29
#define TLS_EC_NAMED_CURVE_X448       30

//
// PQC Hybrid Key Exchange Group identifiers
// https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-supported-groups
//
#define TLS_SUPPORTED_GROUP_SECP256R1MLKEM768   0x11EB
#define TLS_SUPPORTED_GROUP_X25519MLKEM768      0x11EC
#define TLS_SUPPORTED_GROUP_SECP384R1MLKEM1024  0x11ED

//
// IANA TLS Cipher Suite IDs (network byte order UINT16)
// https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml
//
#define TLS_RSA_WITH_AES_128_CBC_SHA           0x002F
#define TLS_RSA_WITH_AES_256_CBC_SHA           0x0035
#define TLS_RSA_WITH_AES_128_CBC_SHA256        0x003C
#define TLS_RSA_WITH_AES_256_CBC_SHA256        0x003D
#define TLS_DHE_RSA_WITH_AES_128_CBC_SHA256    0x0067
#define TLS_DHE_RSA_WITH_AES_256_CBC_SHA256    0x006B
#define TLS_RSA_WITH_AES_128_GCM_SHA256        0x009C
#define TLS_RSA_WITH_AES_256_GCM_SHA384        0x009D
#define TLS_DHE_RSA_WITH_AES_128_GCM_SHA256    0x009E
#define TLS_DHE_RSA_WITH_AES_256_GCM_SHA384    0x009F
#define TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256  0xC027
#define TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384  0xC028
#define TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256  0xC02F
#define TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384  0xC030

//
// TLS 1.3 Cipher Suite IDs (AEAD-only, key exchange is via supported groups)
// https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml
//
#define TLS_AES_128_GCM_SHA256        0x1301
#define TLS_AES_256_GCM_SHA384        0x1302
#define TLS_CHACHA20_POLY1305_SHA256  0x1303
#define TLS_AES_128_CCM_SHA256        0x1304

typedef struct {
  UINT16         CipherId;
  CONST CHAR8    *Name;
} TLS_CIPHER_INFO;

STATIC TLS_CIPHER_INFO  mCipherList[] = {
  { TLS_RSA_WITH_AES_128_CBC_SHA,          "TLS_RSA_WITH_AES_128_CBC_SHA"          },
  { TLS_RSA_WITH_AES_256_CBC_SHA,          "TLS_RSA_WITH_AES_256_CBC_SHA"          },
  { TLS_RSA_WITH_AES_128_CBC_SHA256,       "TLS_RSA_WITH_AES_128_CBC_SHA256"       },
  { TLS_RSA_WITH_AES_256_CBC_SHA256,       "TLS_RSA_WITH_AES_256_CBC_SHA256"       },
  { TLS_DHE_RSA_WITH_AES_128_CBC_SHA256,   "TLS_DHE_RSA_WITH_AES_128_CBC_SHA256"   },
  { TLS_DHE_RSA_WITH_AES_256_CBC_SHA256,   "TLS_DHE_RSA_WITH_AES_256_CBC_SHA256"   },
  { TLS_RSA_WITH_AES_128_GCM_SHA256,       "TLS_RSA_WITH_AES_128_GCM_SHA256"       },
  { TLS_RSA_WITH_AES_256_GCM_SHA384,       "TLS_RSA_WITH_AES_256_GCM_SHA384"       },
  { TLS_DHE_RSA_WITH_AES_128_GCM_SHA256,   "TLS_DHE_RSA_WITH_AES_128_GCM_SHA256"   },
  { TLS_DHE_RSA_WITH_AES_256_GCM_SHA384,   "TLS_DHE_RSA_WITH_AES_256_GCM_SHA384"   },
  { TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256, "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256" },
  { TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384, "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384" },
  { TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256" },
  { TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384, "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384" },
};

/**
  Helper: Create a TLS 1.2 client context + connection for cipher testing.

  @param[out] TlsCtx  Receives the created TLS context.
  @param[out] Tls      Receives the created TLS connection.

  @retval EFI_SUCCESS  Context and connection created successfully.
**/
STATIC
EFI_STATUS
CreateTls12ClientConnection (
  OUT VOID  **TlsCtx,
  OUT VOID  **Tls
  )
{
  EFI_STATUS  Status;

  *TlsCtx = TlsCtxNew (3, 3);
  if (*TlsCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Tls = TlsNew (*TlsCtx);
  if (*Tls == NULL) {
    TlsCtxFree (*TlsCtx);
    *TlsCtx = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  Status = TlsSetConnectionEnd (*Tls, FALSE);
  if (EFI_ERROR (Status)) {
    TlsFree (*Tls);
    TlsCtxFree (*TlsCtx);
    *Tls    = NULL;
    *TlsCtx = NULL;
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Enumerate all cipher suites and report which are supported.

  This test iterates over the IANA cipher suite list and attempts to set
  each one individually via TlsSetCipherList(). It logs the result for
  every cipher, producing the algorithm report.

  The test passes overall if at least one GCM cipher suite is supported.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsCipherSuiteEnumeration (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       SupportedCount;
  UINTN       GcmSupportedCount;
  UINT16      CipherId;

  TlsInitialize ();

  SupportedCount    = 0;
  GcmSupportedCount = 0;

  DEBUG ((DEBUG_INFO, "\n[TLS Cipher Suite Report]\n"));

  for (Index = 0; Index < ARRAY_SIZE (mCipherList); Index++) {
    //
    // Create a fresh connection for each cipher test to avoid state leakage.
    //
    Status = CreateTls12ClientConnection (&TlsCtx, &Tls);
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("Failed to create TLS connection for cipher test\n");
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    CipherId = mCipherList[Index].CipherId;
    Status   = TlsSetCipherList (Tls, &CipherId, 1);

    if (!EFI_ERROR (Status)) {
      DEBUG (
        (
         DEBUG_INFO,
         "  %-45a (0x%04x) .. SUPPORTED\n",
         mCipherList[Index].Name,
         CipherId
        )
        );
      SupportedCount++;

      //
      // Track GCM ciphers specifically
      //
      if ((CipherId == TLS_RSA_WITH_AES_128_GCM_SHA256) ||
          (CipherId == TLS_RSA_WITH_AES_256_GCM_SHA384) ||
          (CipherId == TLS_DHE_RSA_WITH_AES_128_GCM_SHA256) ||
          (CipherId == TLS_DHE_RSA_WITH_AES_256_GCM_SHA384) ||
          (CipherId == TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256) ||
          (CipherId == TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384))
      {
        GcmSupportedCount++;
      }
    } else {
      DEBUG (
        (
         DEBUG_INFO,
         "  %-45a (0x%04x) .. UNSUPPORTED\n",
         mCipherList[Index].Name,
         CipherId
        )
        );
    }

    TlsFree (Tls);
    TlsCtxFree (TlsCtx);
  }

  DEBUG ((DEBUG_INFO, "\n  Total supported: %u / %u\n", SupportedCount, ARRAY_SIZE (mCipherList)));
  DEBUG ((DEBUG_INFO, "  GCM ciphers supported: %u\n\n", GcmSupportedCount));

  //
  // At least one GCM cipher must be available for the test to pass.
  //
  UT_ASSERT_TRUE (GcmSupportedCount > 0);

  return UNIT_TEST_PASSED;
}

/**
  Test setting multiple cipher suites in a single call.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetCipherListMultiple (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINT16      CipherIds[3];

  TlsInitialize ();

  Status = CreateTls12ClientConnection (&TlsCtx, &Tls);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Try setting multiple ciphers at once.
  //
  CipherIds[0] = TLS_RSA_WITH_AES_256_GCM_SHA384;
  CipherIds[1] = TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256;
  CipherIds[2] = TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384;

  Status = TlsSetCipherList (Tls, CipherIds, 3);
  //
  // At least one of these should be accepted.
  //
  UT_ASSERT_NOT_EFI_ERROR (Status);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test that setting a bogus cipher ID returns an error.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsSetCipherListInvalid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINT16      BogusCipher;

  TlsInitialize ();

  Status = CreateTls12ClientConnection (&TlsCtx, &Tls);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  BogusCipher = 0xFFFF;
  Status      = TlsSetCipherList (Tls, &BogusCipher, 1);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_UNSUPPORTED);

  TlsFree (Tls);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test TLS version support enumeration.

  Reports which TLS versions can be used to create contexts.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsVersionSupport (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID   *TlsCtx;
  UINTN  SupportedVersions;

  typedef struct {
    UINT8          MajorVer;
    UINT8          MinorVer;
    CONST CHAR8    *Name;
  } TLS_VERSION_INFO;

  TLS_VERSION_INFO  Versions[] = {
    { 3, 1, "TLS 1.0" },
    { 3, 2, "TLS 1.1" },
    { 3, 3, "TLS 1.2" },
  };

  TlsInitialize ();

  DEBUG ((DEBUG_INFO, "\n[TLS Version Support Report]\n"));

  SupportedVersions = 0;
  for (UINTN Index = 0; Index < ARRAY_SIZE (Versions); Index++) {
    TlsCtx = TlsCtxNew (Versions[Index].MajorVer, Versions[Index].MinorVer);
    if (TlsCtx != NULL) {
      DEBUG ((DEBUG_INFO, "  %-10a .............. SUPPORTED\n", Versions[Index].Name));
      SupportedVersions++;
      TlsCtxFree (TlsCtx);
    } else {
      DEBUG ((DEBUG_INFO, "  %-10a .............. UNSUPPORTED\n", Versions[Index].Name));
    }
  }

  DEBUG ((DEBUG_INFO, "\n"));

  //
  // At minimum TLS 1.2 must be supported.
  //
  TlsCtx = TlsCtxNew (3, 3);
  UT_ASSERT_NOT_NULL (TlsCtx);
  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test EC curve support through TlsSetEcCurve.

  TlsSetEcCurve expects a UINT32 TLS_EC_NAMED_CURVE enum value
  and DataSize == sizeof(UINT32).
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsEcCurveSupport (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINTN       SupportedCurves;
  UINT32      CurveId;

  typedef struct {
    UINT32         CurveId;
    CONST CHAR8    *Name;
  } EC_CURVE_INFO;

  EC_CURVE_INFO  Curves[] = {
    { TLS_EC_NAMED_CURVE_SECP256R1, "P-256 (secp256r1)" },
    { TLS_EC_NAMED_CURVE_SECP384R1, "P-384 (secp384r1)" },
    { TLS_EC_NAMED_CURVE_SECP521R1, "P-521 (secp521r1)" },
    { TLS_EC_NAMED_CURVE_X25519,    "X25519"            },
    { TLS_EC_NAMED_CURVE_X448,      "X448"              },
  };

  TlsInitialize ();

  DEBUG ((DEBUG_INFO, "\n[TLS EC Curve Support Report]\n"));

  SupportedCurves = 0;
  for (UINTN Index = 0; Index < ARRAY_SIZE (Curves); Index++) {
    Status = CreateTls12ClientConnection (&TlsCtx, &Tls);
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("Failed to create TLS connection for EC curve test\n");
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    CurveId = Curves[Index].CurveId;
    Status  = TlsSetEcCurve (Tls, (UINT8 *)&CurveId, sizeof (CurveId));
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "  %-20a ......... SUPPORTED\n", Curves[Index].Name));
      SupportedCurves++;
    } else {
      DEBUG ((DEBUG_INFO, "  %-20a ......... UNSUPPORTED\n", Curves[Index].Name));
    }

    TlsFree (Tls);
    TlsCtxFree (TlsCtx);
  }

  DEBUG ((DEBUG_INFO, "\n"));

  //
  // Report results. secp256r1 is known to be unsupported in some configs.
  //
  DEBUG ((DEBUG_INFO, "  EC curves supported: %u / %u\n\n", SupportedCurves, ARRAY_SIZE (Curves)));

  //
  // At least one curve should be supported.
  //
  UT_ASSERT_TRUE (SupportedCurves > 0);

  return UNIT_TEST_PASSED;
}

/**
  Test TLS 1.3 support and cipher suite availability.

  TLS 1.3 is required for PQC key exchange. This test checks whether
  TLS 1.3 contexts can be created and which TLS 1.3 AEAD cipher suites
  are available.
**/
UNIT_TEST_STATUS
EFIAPI
TestTls13CipherSuiteSupport (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       SupportedCount;
  UINT16      CipherId;

  typedef struct {
    UINT16         CipherId;
    CONST CHAR8    *Name;
  } TLS13_CIPHER_INFO;

  TLS13_CIPHER_INFO  Tls13Ciphers[] = {
    { TLS_AES_128_GCM_SHA256,       "TLS_AES_128_GCM_SHA256"       },
    { TLS_AES_256_GCM_SHA384,       "TLS_AES_256_GCM_SHA384"       },
    { TLS_CHACHA20_POLY1305_SHA256, "TLS_CHACHA20_POLY1305_SHA256" },
    { TLS_AES_128_CCM_SHA256,       "TLS_AES_128_CCM_SHA256"       },
  };

  TlsInitialize ();

  //
  // First check if TLS 1.3 context can be created.
  // TLS 1.3 = Major 3, Minor 4
  //
  TlsCtx = TlsCtxNew (3, 4);

  DEBUG ((DEBUG_INFO, "\n[TLS 1.3 Support Report]\n"));

  if (TlsCtx == NULL) {
    DEBUG ((DEBUG_INFO, "  TLS 1.3 context creation .. UNSUPPORTED\n"));
    DEBUG ((DEBUG_INFO, "  (PQC key exchange requires TLS 1.3)\n\n"));
    //
    // TLS 1.3 not supported is a valid state for now — test documents it.
    //
    return UNIT_TEST_PASSED;
  }

  DEBUG ((DEBUG_INFO, "  TLS 1.3 context creation .. SUPPORTED\n\n"));

  DEBUG ((DEBUG_INFO, "  [TLS 1.3 Cipher Suites]\n"));

  SupportedCount = 0;
  for (Index = 0; Index < ARRAY_SIZE (Tls13Ciphers); Index++) {
    Tls = TlsNew (TlsCtx);
    if (Tls == NULL) {
      UT_LOG_ERROR ("Failed to create TLS 1.3 connection\n");
      TlsCtxFree (TlsCtx);
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    Status = TlsSetConnectionEnd (Tls, FALSE);
    if (EFI_ERROR (Status)) {
      TlsFree (Tls);
      continue;
    }

    CipherId = Tls13Ciphers[Index].CipherId;
    Status   = TlsSetCipherList (Tls, &CipherId, 1);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "  %-35a (0x%04x) .. SUPPORTED\n", Tls13Ciphers[Index].Name, CipherId));
      SupportedCount++;
    } else {
      DEBUG ((DEBUG_INFO, "  %-35a (0x%04x) .. UNSUPPORTED\n", Tls13Ciphers[Index].Name, CipherId));
    }

    TlsFree (Tls);
  }

  DEBUG ((DEBUG_INFO, "\n  TLS 1.3 ciphers supported: %u / %u\n\n", SupportedCount, ARRAY_SIZE (Tls13Ciphers)));

  TlsCtxFree (TlsCtx);

  return UNIT_TEST_PASSED;
}

/**
  Test PQC hybrid key exchange group support via TlsSetEcCurve.

  ML-KEM (Kyber) hybrid key exchange groups are configured as TLS
  supported groups. This test probes whether the TLS library accepts
  PQC hybrid group identifiers through the TlsSetEcCurve API.

  Currently, TlsSetEcCurve only handles traditional EC curves via a
  hardcoded switch. PQC groups will return EFI_UNSUPPORTED until the
  API is extended. This test documents the current state and will
  automatically detect when PQC support is added.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsPqcKeyExchangeGroupSupport (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINTN       SupportedGroups;
  UINT32      GroupId;

  typedef struct {
    UINT32         GroupId;
    CONST CHAR8    *Name;
  } PQC_GROUP_INFO;

  PQC_GROUP_INFO  PqcGroups[] = {
    { TLS_SUPPORTED_GROUP_X25519MLKEM768,     "X25519MLKEM768"     },
    { TLS_SUPPORTED_GROUP_SECP256R1MLKEM768,  "SecP256r1MLKEM768"  },
    { TLS_SUPPORTED_GROUP_SECP384R1MLKEM1024, "SecP384r1MLKEM1024" },
  };

  TlsInitialize ();

  DEBUG ((DEBUG_INFO, "\n[PQC Hybrid Key Exchange Group Report]\n"));

  SupportedGroups = 0;
  for (UINTN Index = 0; Index < ARRAY_SIZE (PqcGroups); Index++) {
    Status = CreateTls12ClientConnection (&TlsCtx, &Tls);
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("Failed to create TLS connection for PQC group test\n");
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    GroupId = PqcGroups[Index].GroupId;
    Status  = TlsSetEcCurve (Tls, (UINT8 *)&GroupId, sizeof (GroupId));
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "  %-25a .... SUPPORTED\n", PqcGroups[Index].Name));
      SupportedGroups++;
    } else {
      DEBUG ((DEBUG_INFO, "  %-25a .... UNSUPPORTED\n", PqcGroups[Index].Name));
    }

    TlsFree (Tls);
    TlsCtxFree (TlsCtx);
  }

  DEBUG ((DEBUG_INFO, "\n  PQC groups supported: %u / %u\n", SupportedGroups, ARRAY_SIZE (PqcGroups)));

  if (SupportedGroups == 0) {
    DEBUG ((DEBUG_INFO, "  (PQC hybrid key exchange not yet wired into TlsSetEcCurve)\n"));
  }

  DEBUG ((DEBUG_INFO, "\n"));

  //
  // PQC support is aspirational — test passes regardless to document status.
  // When PQC is enabled, change this assertion to require support.
  //
  return UNIT_TEST_PASSED;
}

//
// DFCI / Intune required cipher suites.
// These 4 ECDHE-RSA ciphers are mandatory for Surface firmware TLS connectivity.
// See: https://github.com/microsoft/mu_feature_dfci/blob/main/DfciPkg/UnitTests/DfciTests/RefreshServer/Src/server.py
//
STATIC TLS_CIPHER_INFO  mDfciRequiredCiphers[] = {
  { TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384, "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384" },
  { TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256" },
  { TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384, "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384" },
  { TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256, "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256" },
};

/**
  Hard-fail test for DFCI/Intune required cipher suites.

  These 4 ECDHE-RSA ciphers must be supported. If any is missing, the firmware
  cannot establish TLS connections to Intune/DFCI endpoints.
**/
UNIT_TEST_STATUS
EFIAPI
TestTlsDfciRequiredCiphers (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  VOID        *TlsCtx;
  VOID        *Tls;
  EFI_STATUS  Status;
  UINTN       Index;
  UINT16      CipherId;

  TlsInitialize ();

  DEBUG ((DEBUG_INFO, "\n[DFCI Required Cipher Suites]\n"));

  for (Index = 0; Index < ARRAY_SIZE (mDfciRequiredCiphers); Index++) {
    Status = CreateTls12ClientConnection (&TlsCtx, &Tls);
    if (EFI_ERROR (Status)) {
      UT_LOG_ERROR ("Failed to create TLS connection for DFCI cipher test\n");
      return UNIT_TEST_ERROR_TEST_FAILED;
    }

    CipherId = mDfciRequiredCiphers[Index].CipherId;
    Status   = TlsSetCipherList (Tls, &CipherId, 1);

    DEBUG (
      (
       DEBUG_INFO,
       "  %-45a (0x%04X) .. %a\n",
       mDfciRequiredCiphers[Index].Name,
       CipherId,
       EFI_ERROR (Status) ? "MISSING" : "OK"
      )
      );

    TlsFree (Tls);
    TlsCtxFree (TlsCtx);

    //
    // Hard failure — this cipher is required for DFCI/Intune connectivity.
    //
    UT_ASSERT_NOT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_INFO, "\n  All %u DFCI required ciphers present.\n\n", ARRAY_SIZE (mDfciRequiredCiphers)));

  return UNIT_TEST_PASSED;
}

TEST_DESC  mTlsCipherTest[] = {
  //
  // Description--------------------------------------Class-------------------------------------Func---------------------------------PreReq--CleanUp--Context
  //
  { "TLS cipher suite enumeration",               "CryptoPkg.TlsLib.Cipher", TestTlsCipherSuiteEnumeration,     NULL, NULL, NULL },
  { "DFCI required cipher suites",                "CryptoPkg.TlsLib.Cipher", TestTlsDfciRequiredCiphers,        NULL, NULL, NULL },
  { "TLS set multiple cipher suites",             "CryptoPkg.TlsLib.Cipher", TestTlsSetCipherListMultiple,      NULL, NULL, NULL },
  { "TLS set invalid cipher returns UNSUPPORTED", "CryptoPkg.TlsLib.Cipher", TestTlsSetCipherListInvalid,       NULL, NULL, NULL },
  { "TLS version support enumeration",            "CryptoPkg.TlsLib.Cipher", TestTlsVersionSupport,             NULL, NULL, NULL },
  { "TLS EC curve support enumeration",           "CryptoPkg.TlsLib.Cipher", TestTlsEcCurveSupport,             NULL, NULL, NULL },
  { "TLS 1.3 cipher suite support",               "CryptoPkg.TlsLib.Cipher", TestTls13CipherSuiteSupport,       NULL, NULL, NULL },
  { "PQC hybrid key exchange group support",      "CryptoPkg.TlsLib.Cipher", TestTlsPqcKeyExchangeGroupSupport, NULL, NULL, NULL },
};

UINTN  mTlsCipherTestNum = ARRAY_SIZE (mTlsCipherTest);
