/** @file
  PEI instance of CryptoIndicatorRegistrationLib.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Pi/PiHob.h>
#include <Guid/CryptoIndicatorTable.h>
#include <Guid/EcitCapabilityHob.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/CryptoIndicatorRegistrationLib.h>
#include <Library/HobLib.h>

#define MAX_GUID_HOB_LENGTH  0xFFF8U

/**
  Store a caller-supplied capability payload in a GUIDed HOB for the DXE ECIT
  collector.

  @param[in] FeatureId    ECIT feature GUID this capability describes.
  @param[in] Payload      Feature capability payload. May be NULL only when
                          PayloadSize is zero.
  @param[in] PayloadSize  Size of Payload in bytes.

  @retval EFI_SUCCESS            The capability HOB was created.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid or the record exceeds
                                 the HOB or ECIT format limit.
  @retval EFI_OUT_OF_RESOURCES   The capability HOB could not be allocated.
**/
EFI_STATUS
EFIAPI
EcitRegisterCryptoCapability (
  IN CONST EFI_GUID  *FeatureId,
  IN CONST VOID      *Payload        OPTIONAL,
  IN UINTN           PayloadSize
  )
{
  EDKII_ECIT_CAPABILITY_HOB  *CapabilityHob;
  UINTN                      HobDataSize;

  if ((FeatureId == NULL) ||
      IsZeroGuid (FeatureId) ||
      ((Payload == NULL) && (PayloadSize != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((PayloadSize > (MAX_UINT16 - sizeof (EFI_CRYPTO_INDICATOR_ENTRY))) ||
      (PayloadSize > (MAX_GUID_HOB_LENGTH - sizeof (EFI_HOB_GUID_TYPE) - sizeof (EDKII_ECIT_CAPABILITY_HOB))))
  {
    return EFI_INVALID_PARAMETER;
  }

  HobDataSize   = ALIGN_VALUE (sizeof (EDKII_ECIT_CAPABILITY_HOB) + PayloadSize, 8);
  CapabilityHob = BuildGuidHob (&gEdkiiEcitCapabilityHobGuid, HobDataSize);
  if (CapabilityHob == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (CapabilityHob, HobDataSize);
  CapabilityHob->Revision      = EDKII_ECIT_CAPABILITY_HOB_REVISION;
  CapabilityHob->HeaderSize    = (UINT16)sizeof (EDKII_ECIT_CAPABILITY_HOB);
  CapabilityHob->EntryDataSize = (UINT32)PayloadSize;
  CopyGuid (&CapabilityHob->FeatureIdentifier, FeatureId);
  if (PayloadSize != 0) {
    CopyMem (
      CapabilityHob->EntryData,
      Payload,
      PayloadSize
      );
  }

  DEBUG ((
    DEBUG_INFO,
    "ECIT(PEI): recorded feature %g (%u byte payload)\n",
    FeatureId,
    (UINT32)PayloadSize
    ));

  return EFI_SUCCESS;
}
