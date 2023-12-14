/*
 * Copyright 2017-2021 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright 2017 Ribose Inc. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

// Copied from sm3_legacy.c

#include "crypto/evp.h"
#include "openssl/crypto/evp/legacy_meth.h"
#include "internal/sm3.h"

static int init(EVP_MD_CTX *ctx)
{
    return 1;
}

static int update(EVP_MD_CTX *ctx, const void *data, size_t count)
{
    return 1;
}

static int final(EVP_MD_CTX *ctx, unsigned char *md)
{
    return 1;
}

IMPLEMENT_LEGACY_EVP_MD_METH_LC(sm3_int, ossl_sm3)

static const EVP_MD sm3_md = {
    NID_sm3,
    NID_sm3WithRSAEncryption,
    SM3_DIGEST_LENGTH,
    0,
    EVP_ORIG_GLOBAL,
    LEGACY_EVP_MD_METH_TABLE(init, update, final, NULL,
                             SM3_CBLOCK),
};

const EVP_MD *EVP_sm3(void)
{
    return NULL;
}
