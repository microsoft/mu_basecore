/** @file
  UEFI Openssl provider implementation.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CIPHER_PROV_H__
#define __CIPHER_PROV_H__

#include "prov/ciphercommon.h"

const PROV_CIPHER_HW *ossl_prov_cipher_hw_aes_cbc(size_t keybits);

#endif
