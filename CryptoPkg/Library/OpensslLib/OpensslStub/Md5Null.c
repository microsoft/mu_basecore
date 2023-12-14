/** @file
  Null implementation of MD5 functions called by BaseCryptLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
 * MD5 low level APIs are deprecated for public use, but still ok for
 * internal use.
 */

#include "openssl/include/internal/deprecated.h"

#include <openssl/md5.h>
#include "crypto/evp.h"
#include "openssl/crypto/evp/legacy_meth.h"

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

IMPLEMENT_LEGACY_EVP_MD_METH(md5, MD5)

static const EVP_MD md5_md = {
    NID_md5,
    NID_md5WithRSAEncryption,
    MD5_DIGEST_LENGTH,
    0,
    EVP_ORIG_GLOBAL,
    LEGACY_EVP_MD_METH_TABLE(init, update, final, NULL, MD5_CBLOCK)
};

const EVP_MD *EVP_md5(void)
{
    return NULL;
}

//taken from md5_sha1.h
static const EVP_MD md5_sha1_md = {
    NID_md5_sha1,
    NID_md5_sha1,
    MD5_DIGEST_LENGTH,
    0,
    EVP_ORIG_GLOBAL,
    LEGACY_EVP_MD_METH_TABLE(init, update, final, NULL, MD5_CBLOCK),
};

const EVP_MD *EVP_md5_sha1(void)
{
    return NULL;
}

// Used for s3_cbc.c
void MD5_Transform (MD5_CTX *c, const unsigned char *b) {
  return;
}

int MD5_Init(MD5_CTX *c) {
  return 1;
}
