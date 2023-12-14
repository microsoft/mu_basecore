/** @file
  UEFI Openssl cipher provider implementation.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <openssl/aes.h>
#include "prov/implementations.h"
#include "prov/providercommon.h"
#include "prov/ciphercommon.h"


typedef struct prov_aes_ctx_st {
    PROV_CIPHER_CTX base;      /* Must be first */
    union {
        OSSL_UNION_ALIGN;
        AES_KEY ks;
    } ks;

    /* Platform specific data */
    union {
        int dummy;
#if defined(OPENSSL_CPUID_OBJ) && defined(__s390__)
        struct {
            union {
                OSSL_UNION_ALIGN;
                /*-
                 * KM-AES parameter block - begin
                 * (see z/Architecture Principles of Operation >= SA22-7832-06)
                 */
                struct {
                    unsigned char k[32];
                } km;
                /* KM-AES parameter block - end */
                /*-
                 * KMO-AES/KMF-AES parameter block - begin
                 * (see z/Architecture Principles of Operation >= SA22-7832-08)
                 */
                struct {
                    unsigned char cv[16];
                    unsigned char k[32];
                } kmo_kmf;
                /* KMO-AES/KMF-AES parameter block - end */
            } param;
            unsigned int fc;
            int res;
        } s390x;
#endif /* defined(OPENSSL_CPUID_OBJ) && defined(__s390__) */
    } plat;

} PROV_AES_CTX;

static OSSL_FUNC_cipher_freectx_fn aes_freectx;
static OSSL_FUNC_cipher_dupctx_fn aes_dupctx;

static void aes_freectx(void *vctx)
{
    PROV_AES_CTX *ctx = (PROV_AES_CTX *)vctx;

    ossl_cipher_generic_reset_ctx((PROV_CIPHER_CTX *)vctx);
    OPENSSL_clear_free(ctx,  sizeof(*ctx));
}

static void *aes_dupctx(void *ctx)
{
    PROV_AES_CTX *in = (PROV_AES_CTX *)ctx;
    PROV_AES_CTX *ret;

    if (!ossl_prov_is_running())
        return NULL;

    ret = OPENSSL_malloc(sizeof(*ret));
    if (ret == NULL) {
        ERR_raise(ERR_LIB_PROV, ERR_R_MALLOC_FAILURE);
        return NULL;
    }
    in->base.hw->copyctx(&ret->base, &in->base);

    return ret;
}

const PROV_CIPHER_HW *ossl_prov_cipher_hw_aes_cbc(size_t keybits);

/* ossl_aes256cbc_functions */
IMPLEMENT_generic_cipher(aes, AES, cbc, CBC, 0, 256, 128, 128, block)
/* ossl_aes192cbc_functions */
IMPLEMENT_generic_cipher(aes, AES, cbc, CBC, 0, 192, 128, 128, block)
/* ossl_aes128cbc_functions */
IMPLEMENT_generic_cipher(aes, AES, cbc, CBC, 0, 128, 128, 128, block)
