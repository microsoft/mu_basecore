/** @file
  GUIDed HOB used to pass ECIT capability records from PEI to DXE.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>

#define EDKII_ECIT_CAPABILITY_HOB_REVISION  1

typedef struct {
  UINT16      Revision;
  UINT16      HeaderSize;
  UINT32      EntryDataSize;
  EFI_GUID    FeatureIdentifier;
  UINT8       EntryData[0];
} EDKII_ECIT_CAPABILITY_HOB;

extern EFI_GUID  gEdkiiEcitCapabilityHobGuid;
