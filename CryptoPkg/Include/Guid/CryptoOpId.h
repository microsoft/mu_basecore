/** @file
  Crypto-operation identifiers queried with GetCryptoOpCapability().

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

/// Provider signature algorithm OIDs for PKCS#7/CMS verification.
extern EFI_GUID  gCryptoOpCmsVerifyGuid;
/// Provider fixed-output digest algorithm OIDs for PKCS#7/CMS content.
extern EFI_GUID  gCryptoOpCmsContentDigestGuid;
/// Provider signature algorithm OIDs for Authenticode verification.
extern EFI_GUID  gCryptoOpAuthenticodeVerifyGuid;
/// Available image-hash digest algorithm OIDs for Authenticode.
extern EFI_GUID  gCryptoOpAuthenticodeHashGuid;
