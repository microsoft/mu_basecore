/** @file
  Mock implementation of PeiServicesLib
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PEI_SERVICES_LIB_H_
#define MOCK_PEI_SERVICES_LIB_H_

#include <Uefi.h>

typedef struct _PPI_STATUS {
  VOID          *Ppi;
  EFI_STATUS    Status;
} PPI_STATUS;

#endif // MOCK_PEI_SERVICES_LIB_H_