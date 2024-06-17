/** @file
  This PPI provides services to publish, update, and retrieve general policies in the PEI
  environment.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _POLICY_PPI_H_
#define _POLICY_PPI_H_

#include <PolicyInterface.h>

#define POLICY_PPI_GUID  {0xa8b33630, 0xa1ae, 0x4e2d, { 0x8d, 0x0f, 0x3d, 0xf3, 0xe5, 0x87, 0x08, 0xce } }

typedef struct _POLICY_INTERFACE POLICY_PPI;

extern EFI_GUID  gPeiPolicyPpiGuid;

#endif
