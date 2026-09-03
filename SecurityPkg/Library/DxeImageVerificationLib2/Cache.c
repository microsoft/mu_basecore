/** @file
  Dynamic digest cache for DxeImageVerificationLib.

  Memoizes the digest of a bound buffer under each requested hash-algorithm GUID, allocating one
  entry per algorithm on demand (no fixed-size table). The actual hashing is delegated to
  BaseCryptLib's HashAllByGuid ().

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Support.h"

//
// One memoized digest: the algorithm GUID it was computed under and the digest bytes. Nodes are
// singly linked so the cache grows one entry per distinct algorithm requested for its buffer.
//
struct DIGEST_CACHE_ENTRY {
  DIGEST_CACHE_ENTRY    *Next;
  CONST EFI_GUID        *Algorithm;
  UINT8                 Digest[MAX_DIGEST_SIZE];
  UINTN                 DigestSize;
};

/**
  Get or compute the cached digest of the cache's buffer under a hash algorithm.

  On a cache hit the memoized digest is returned; on a miss the buffer is hashed with HashAllByGuid ()
  and the result is memoized in a newly allocated entry (keyed by HashAlgorithm) before being
  returned. The digest bytes remain valid until FreeDigestCache ().

  @param[in]      HashAlgorithm  Protocol/Hash.h algorithm GUID (EFI_HASH_ALGORITHM_*_GUID).
  @param[in,out]  Cache          Caller-owned cache bound to a buffer via Cache->Buffer /
                                 Cache->BufferSize.
  @param[out]     Digest         On success, a pointer to the cached digest bytes, valid until
                                 FreeDigestCache ().
  @param[out]     DigestSize     On success, the digest length in bytes.

  @retval EFI_SUCCESS            Digest / DigestSize describe a valid cached digest.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_OUT_OF_RESOURCES   A cache entry could not be allocated.
  @retval other                  A failure computing the digest, propagated from HashAllByGuid ().
**/
EFI_STATUS
GetHash (
  IN     CONST EFI_GUID  *HashAlgorithm,
  IN OUT DIGEST_CACHE    *Cache,
  OUT    CONST UINT8     **Digest,
  OUT    UINTN           *DigestSize
  )
{
  EFI_STATUS          Status;
  DIGEST_CACHE_ENTRY  *Entry;

  if ((HashAlgorithm == NULL) || (Cache == NULL) || (Cache->Buffer == NULL) ||
      (Digest == NULL) || (DigestSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Return a memoized digest if this algorithm has already been computed for the bound buffer.
  //
  for (Entry = Cache->Entries; Entry != NULL; Entry = Entry->Next) {
    if (CompareGuid (Entry->Algorithm, HashAlgorithm)) {
      *Digest     = Entry->Digest;
      *DigestSize = Entry->DigestSize;
      return EFI_SUCCESS;
    }
  }

  //
  // Miss: hash the buffer into a fresh entry. The entry is linked only on success, so a failed hash
  // leaves the cache unchanged.
  //
  Entry = AllocatePool (sizeof (*Entry));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HashAllByGuid (HashAlgorithm, Cache->Buffer, Cache->BufferSize, Entry->Digest, &Entry->DigestSize);
  if (EFI_ERROR (Status)) {
    FreePool (Entry);
    return Status;
  }

  Entry->Algorithm = HashAlgorithm;
  Entry->Next      = Cache->Entries;
  Cache->Entries   = Entry;

  *Digest     = Entry->Digest;
  *DigestSize = Entry->DigestSize;
  return EFI_SUCCESS;
}

/**
  Release the memoized digest entries owned by a DIGEST_CACHE.

  Frees every entry GetHash () allocated and resets the cache to empty. Buffer / BufferSize are left
  intact (the buffer is caller-owned). Safe to call on a zero-initialized or already-freed cache.

  @param[in,out]  Cache  Cache whose memoized entries are released, or NULL.
**/
VOID
FreeDigestCache (
  IN OUT DIGEST_CACHE  *Cache
  )
{
  DIGEST_CACHE_ENTRY  *Entry;
  DIGEST_CACHE_ENTRY  *Next;

  if (Cache == NULL) {
    return;
  }

  for (Entry = Cache->Entries; Entry != NULL; Entry = Next) {
    Next = Entry->Next;
    FreePool (Entry);
  }

  Cache->Entries = NULL;
}
