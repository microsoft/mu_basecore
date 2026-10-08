/** @file
  ECIT (EFI Crypto Indicator Table) crypto-operation identifier GUIDs.

  Identifiers for ECIT crypto operations queried with GetCryptoOpCapability().
  Each capability is an unordered array of algorithm OID records.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CRYPTO_OP_ID_H_
#define CRYPTO_OP_ID_H_

//
// Operation-ID GUIDs. Stable once committed.
//
extern EFI_GUID  gCryptoOpCmsVerifyGuid;
extern EFI_GUID  gCryptoOpCmsContentDigestGuid;
extern EFI_GUID  gCryptoOpAuthenticodeVerifyGuid;
extern EFI_GUID  gCryptoOpAuthenticodeHashGuid;

#endif // CRYPTO_OP_ID_H_
