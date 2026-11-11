/** @file
  Report the image verifier's cryptographic capabilities through ECIT.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DxeImageVerificationLib.h"
#include "Ecit.h"

#include <Guid/CryptoIndicatorTable.h>
#include <Guid/CryptoOpId.h>
#include <Library/CryptoIndicatorRegistrationLib.h>
#include <Library/EcitEncodingLib.h>
#include <Library/SafeIntLib.h>

STATIC CONST EFI_GUID  *mImageVerificationOps[] = {
  &gCryptoOpAuthenticodeVerifyGuid,
  &gCryptoOpAuthenticodeHashGuid
};

STATIC
BOOLEAN
IsSupportedImageDigest (
  IN CONST BASE_CRYPT_OP_CAPABILITY  *Capability
  )
{
  UINTN  Index;
  UINTN  OidSize;

  if (Capability->AlgorithmOid == NULL) {
    return FALSE;
  }

  for (Index = 0; Index < ARRAY_SIZE (mImageDigestAlgorithms); Index++) {
    OidSize = AsciiStrSize (mImageDigestAlgorithms[Index].Oid);
    if ((Capability->AlgorithmOidSize == OidSize) &&
        (CompareMem (Capability->AlgorithmOid, mImageDigestAlgorithms[Index].Oid, OidSize) == 0))
    {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
VOID
ReportImageVerificationAlgorithms (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     CapabilityIndex;
  UINTN                     TotalCount;
  UINTN                     AllocationSize;
  UINTN                     Offset;
  UINTN                     PayloadSize;
  BASE_CRYPT_OP_CAPABILITY  *OperationCapabilities[ARRAY_SIZE (mImageVerificationOps)] = { 0 };
  UINTN                     OperationCounts[ARRAY_SIZE (mImageVerificationOps)]        = { 0 };
  BASE_CRYPT_OP_CAPABILITY  *Capabilities;
  VOID                      *Payload;

  TotalCount   = 0;
  Capabilities = NULL;
  Payload      = NULL;
  for (Index = 0; Index < ARRAY_SIZE (mImageVerificationOps); Index++) {
    Status = GetCryptoOpCapability (
               mImageVerificationOps[Index],
               &OperationCapabilities[Index],
               &OperationCounts[Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "DxeImageVerificationLib: ECIT operation %g unavailable (%r).\n", mImageVerificationOps[Index], Status));
      goto Cleanup;
    }

    if ((OperationCounts[Index] != 0) && (OperationCapabilities[Index] == NULL)) {
      Status = EFI_COMPROMISED_DATA;
      goto Cleanup;
    }

    if (RETURN_ERROR (SafeUintnAdd (TotalCount, OperationCounts[Index], &TotalCount))) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Cleanup;
    }
  }

  if (TotalCount != 0) {
    if (RETURN_ERROR (SafeUintnMult (TotalCount, sizeof (*Capabilities), &AllocationSize))) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Cleanup;
    }

    Capabilities = AllocatePool (AllocationSize);
    if (Capabilities == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Cleanup;
    }

    Offset = 0;
    for (Index = 0; Index < ARRAY_SIZE (mImageVerificationOps); Index++) {
      for (CapabilityIndex = 0; CapabilityIndex < OperationCounts[Index]; CapabilityIndex++) {
        if ((mImageVerificationOps[Index] == &gCryptoOpAuthenticodeHashGuid) &&
            !IsSupportedImageDigest (&OperationCapabilities[Index][CapabilityIndex]))
        {
          continue;
        }

        Capabilities[Offset++] = OperationCapabilities[Index][CapabilityIndex];
      }
    }

    TotalCount = Offset;
  }

  Status = EcitEncodeOidSet (Capabilities, TotalCount, &Payload, &PayloadSize);
  if (!EFI_ERROR (Status)) {
    Status = EcitRegisterCryptoCapability (
               &gEfiEcitFeatureSbImageVerificationGuid,
               Payload,
               PayloadSize
               );
  }

Cleanup:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to report ECIT verification algorithms (%r).\n", Status));
  }

  if (Payload != NULL) {
    FreePool (Payload);
  }

  if (Capabilities != NULL) {
    FreePool (Capabilities);
  }

  for (Index = 0; Index < ARRAY_SIZE (mImageVerificationOps); Index++) {
    if (OperationCapabilities[Index] != NULL) {
      FreePool (OperationCapabilities[Index]);
    }
  }
}

STATIC
VOID
ReportSignatureListTypes (
  VOID
  )
{
  EFI_GUID  Types[ARRAY_SIZE (mImageHashSignatures) +
                  ARRAY_SIZE (mTbsHashSignatures) +
                  ARRAY_SIZE (mX509CertSignatures)];
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       Count;

  Count = 0;
  for (Index = 0; Index < ARRAY_SIZE (mX509CertSignatures); Index++) {
    CopyGuid (&Types[Count++], mX509CertSignatures[Index].SignatureType);
  }

  for (Index = 0; Index < ARRAY_SIZE (mImageHashSignatures); Index++) {
    CopyGuid (&Types[Count++], mImageHashSignatures[Index].SignatureType);
  }

  for (Index = 0; Index < ARRAY_SIZE (mTbsHashSignatures); Index++) {
    CopyGuid (&Types[Count++], mTbsHashSignatures[Index].SignatureType);
  }

  Status = EcitRegisterCryptoCapability (
             &gEfiEcitFeatureSbImageAuthorizationGuid,
             Types,
             sizeof (Types)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to report ECIT authorization types (%r).\n", Status));
  }

  Status = EcitRegisterCryptoCapability (
             &gEfiEcitFeatureSbImageRevocationGuid,
             Types,
             sizeof (Types)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "DxeImageVerificationLib: failed to report ECIT revocation types (%r).\n", Status));
  }
}

VOID
EFIAPI
ReportImageVerificationCapabilities (
  VOID
  )
{
  ReportImageVerificationAlgorithms ();
  ReportSignatureListTypes ();
}
