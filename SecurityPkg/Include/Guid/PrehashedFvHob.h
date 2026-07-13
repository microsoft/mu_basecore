/** @file
  HOB used to declare pre-hashed firmware volumes to Tpm2StartupLib
  (or any consumer that needs to know a FV's digests without hashing it).

  This is the phase-agnostic equivalent of
  gEdkiiPeiFirmwareVolumeInfoPrehashedFvPpiGuid. Producers include:
   - Tcg2Pei, which translates the PPI to this HOB early in its flow so
     the consolidated Tpm2StartupLib can find pre-hashed FVs without
     depending on PEI services.
   - PEI-less platforms (SEC / early DXE), which can BuildGuidHob directly
     when they have pre-computed FV digests (typically obtained from a
     prior firmware stage such as TF-A).

  One HOB is produced per pre-hashed FV. The HOB payload begins with a
  PREHASHED_FV_HOB header identifying the FV, immediately followed by
  Count HASH_INFO records; each HASH_INFO is followed inline by its
  digest bytes (HashSize bytes). This mirrors the PPI payload layout to
  keep any PPI-to-HOB translator trivial.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PREHASHED_FV_HOB_H_
#define PREHASHED_FV_HOB_H_

#include <Ppi/FirmwareVolumeInfoPrehashedFV.h>

extern EFI_GUID  gPrehashedFvHobGuid;

typedef struct {
  EFI_PHYSICAL_ADDRESS    FvBase;
  UINT64                  FvLength;
  UINT32                  Count;
  // HASH_INFO             HashInfo[]; each followed inline by HashSize bytes of digest
} PREHASHED_FV_HOB;

#endif
