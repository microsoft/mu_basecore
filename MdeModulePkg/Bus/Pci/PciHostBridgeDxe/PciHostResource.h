/** @file

  The Header file of the Pci Host Bridge Driver.

Copyright (c) 1999 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PCI_HOST_RESOURCE_H_
#define _PCI_HOST_RESOURCE_H_

#include <PiDxe.h>

#define PCI_RESOURCE_LESS  0xFFFFFFFFFFFFFFFEULL

typedef enum {
  TypeIo,  // MU_CHANGE
  TypeMem32,
  TypePMem32,
  TypeMem64,
  TypePMem64,
  TypeBus,
  TypeMax
} PCI_RESOURCE_TYPE;

// MU_CHANGE begin
// Only x86_64 has IO resource
#if defined(__x86_64__)
#define PCI_RESOURCE_TYPE_ENUM_START TypeIo
#else
#define PCI_RESOURCE_TYPE_ENUM_START TypeMem32
#endif
// MU_CHANGE end

typedef enum {
  ResNone,
  ResSubmitted,
  ResAllocated,
  ResStatusMax
} RES_STATUS;

typedef struct {
  PCI_RESOURCE_TYPE    Type;
  //
  // Base is a host address
  //
  UINT64               Base;
  UINT64               Length;
  UINT64               Alignment;
  RES_STATUS           Status;
} PCI_RES_NODE;

#endif
