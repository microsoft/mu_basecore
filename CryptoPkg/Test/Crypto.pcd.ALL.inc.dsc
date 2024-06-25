# List of Crypto PCDs with all functionality enabled.  This should only be used for testing.

# HMACSHA256
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceHmacSha256New|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceHmacSha256Free|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceHmacSha256SetKey|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceHmacSha256Duplicate|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceHmacSha256Update|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceHmacSha256Final|TRUE
# PKCS
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServicePkcs5HashPassword|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServicePkcs1v2Encrypt|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServicePkcs7GetSigners|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServicePkcs7FreeSigners|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServicePkcs7GetCertificatesList|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServicePkcs7Sign|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServicePkcs7Verify|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceVerifyEKUsInPkcs7Signature|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServicePkcs7GetAttachedContent|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceAuthenticodeVerify|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceImageTimestampVerify|TRUE
# DH
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceDhNew|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceDhFree|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceDhGenerateParameter|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceDhSetParameter|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceDhGenerateKey|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceDhComputeKey|TRUE
# RANDOM
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRandomSeed|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRandomBytes|TRUE
# RSA
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaNew|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaFree|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaSetKey|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaGetKey|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaGenerateKey|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaCheckKey|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaPkcs1Sign|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaPkcs1Verify|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaPssSign|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaPssVerify|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaGetPrivateKeyFromPem|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceRsaGetPublicKeyFromX509|TRUE
# SHA1
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha1GetContextSize|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha1Init|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha1Duplicate|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha1Update|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha1Final|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha1HashAll|TRUE
# SHA256
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha256GetContextSize|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha256Init|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha256Duplicate|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha256Update|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha256Final|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha256HashAll|TRUE
# SHA384
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha384GetContextSize|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha384Init|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha384Duplicate|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha384Update|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha384Final|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha384HashAll|TRUE
# SHA512
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha512GetContextSize|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha512Init|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha512Duplicate|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha512Update|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha512Final|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSha512HashAll|TRUE
# PARALLELHASH256
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceParallelHash256HashAll|TRUE
# X509
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509GetSubjectName|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509GetCommonName|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509GetOrganizationName|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509VerifyCert|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509ConstructCertificate|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509ConstructCertificateStackV|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509ConstructCertificateStack|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509Free|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509StackFree|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceX509GetTBSCert|TRUE
# TDES
# AES
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceAesGetContextSize|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceAesInit|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceAesCbcEncrypt|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceAesCbcDecrypt|TRUE
# ARC4
# SM3
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSm3GetContextSize|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSm3Init|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSm3Duplicate|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSm3Update|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSm3Final|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceSm3HashAll|TRUE
# HKDF
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceHkdfSha256ExtractAndExpand|TRUE
# TLS
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsInitialize|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsCtxFree|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsCtxNew|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsFree|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsNew|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsInHandshake|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsDoHandshake|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsHandleAlert|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsCloseNotify|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsCtrlTrafficOut|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsCtrlTrafficIn|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsRead|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsWrite|TRUE
# TLSSET
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetVersion|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetConnectionEnd|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetCipherList|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetCompressionMethod|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetVerify|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetVerifyHost|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetSessionId|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetCaCertificate|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetHostPublicCert|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetHostPrivateKey|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsSetCertRevocationList|TRUE
# TLSGET
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetVersion|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetConnectionEnd|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetCurrentCipher|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetCurrentCompressionId|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetVerify|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetSessionId|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetClientRandom|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetServerRandom|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetKeyMaterial|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetCaCertificate|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetHostPublicCert|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetHostPrivateKey|TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceTlsGetCertRevocationList|TRUE
# AUTOGEN ENDS
# ****************************************************************************
