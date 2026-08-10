/** @file
  ECIT (EFI Crypto Indicator Table) crypto-operation identifier GUIDs.

  These GUIDs name the crypto operations understood by
  GetCryptoOpCapability() (declared in <Library/BaseCryptLib.h>). A caller
  passes one of these GUIDs to ask the linked crypto binary which
  algorithms it will actually accept for that operation; the answer is a
  CSV-encoded, NUL-terminated ASCII string of dotted-decimal algorithm
  OIDs (unordered set).

  The GUIDs live in CryptoPkg (not a backend package) so that
  backend-agnostic consumers -- the OneCrypto protocol forwarder, an ECIT
  collector, host tests, or either the OpenSSL or MbedTLS BaseCryptLib
  implementation -- can reference them without depending on a specific
  crypto backend. Storage is provided by AutoGen for every module that
  lists them in its INF [Guids] block; the values are registered in
  CryptoPkg.dec.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CRYPTO_OP_ID_H_
#define CRYPTO_OP_ID_H_

//
// Operation-ID GUIDs. Stable once committed.
//
extern EFI_GUID  gCryptoOpCmsVerifyGuid;
extern EFI_GUID  gCryptoOpAuthenticodeVerifyGuid;

#endif // CRYPTO_OP_ID_H_
