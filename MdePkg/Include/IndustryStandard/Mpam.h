/** @file
  ACPI for Memory System Resource Partitioning and Monitoring (MPAM) as
  specified in ARM spec DEN0065A

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MPAM_H__
#define __MPAM_H__

#include <IndustryStandard/Acpi.h>

///
/// PPTT Revision (as defined in ACPI 6.4 spec.)
///
#define EFI_ACPI_6_4_MEMORY_SYSTEM_RESOURCE_PARTITIONING_AND_MONITORING_TABLE_REVISION  (0x01)

///
/// MPAM Interrupt trigger types
///
#define MPAM_INTERRUPT_LEVEL_TRIGGERED  (0x0)
#define MPAM_INTERRUPT_EDGE_TRIGGERED   (0x1)

///
/// MPAM Interrupt wired/non-wired
///
#define MPAM_WIRED_INTERRUPT  (0x0)

///
/// MPAM MSC affinity
///
#define MPAM_PROCESSOR_AFFINITY            (0x0)
#define MPAM_PROCESSOR_CONTAINER_AFFINITY  (0x1)

///
/// MPAM MSC affinity validity
///
#define MPAM_AFFINITY_NOT_VALID  (0x0)
#define MPAM_AFFINITY_VALID      (0x1)

///
/// Location types
///
#define MPAM_PROCESSOR_CACHE  (0x0)
#define MPAM_MEMORY           (0x1)
#define MPAM_SMMU             (0x2)
#define MPAM_MEMORY_CACHE     (0x3)
#define MPAM_ACPI_DEVICE      (0x4)
#define MPAM_UNKNOWN          (0xff)

#pragma pack(1)

///
/// MPAM Interrupt Flags
///
typedef struct {
  UINT32    InterruptMode : 1;
  UINT32    InterruptType : 2;
  UINT32    AffinityType  : 1;
  UINT32    AffinityValid : 1;
  UINT32    Reserved      : 27;
} MPAM_MSC_INTERRUPT_FLAGS;

///
/// MPAM MSC node type
///
typedef struct {
  UINT16                      Length;
  UINT16                      Reserved;
  UINT32                      Identifier;
  UINT64                      BaseAddress;
  UINT32                      MmioSize;
  UINT32                      OverflowInterrupt;
  MPAM_MSC_INTERRUPT_FLAGS    OverflowInterruptFlags;
  UINT32                      Reserved1;
  UINT32                      OverflowInterruptAffinity;
  UINT32                      ErrorInterrupt;
  MPAM_MSC_INTERRUPT_FLAGS    ErrorInterruptFlags;
  UINT32                      Reserved2;
  UINT32                      ErrorInterruptAffinity;
  UINT32                      MaxNrdyUsec;
  UINT64                      PmLinkHID;
  UINT32                      PmLinkUID;
  UINT32                      NumMpamResources;
} MPAM_MSC_NODE;

///
/// MPAM MSC Locator field
///
typedef struct {
  UINT64    Descriptor1;
  UINT32    Descriptor2;
} MPAM_LOCATOR;

///
/// MPAM MSC Node resource type
///
typedef struct {
  UINT32          Identifier;
  UINT8           RisIndex;
  UINT16          Reserved1;
  UINT8           LocatorType;
  MPAM_LOCATOR    Locator;
  UINT32          NumDependencies;
} MPAM_MSC_RESOURCE;

///
/// MPAM Function dependency descriptor
///
typedef struct {
  UINT32    Producer;
  UINT32    Reserved;
} MPAM_FUNCTIONAL_DEPENDENCY_DESCRIPTOR;

#pragma pack()

#endif
