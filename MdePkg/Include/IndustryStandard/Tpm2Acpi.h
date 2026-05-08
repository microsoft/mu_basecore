/** @file
  TPM2 ACPI table definition.

Copyright (c) 2013 - 2019, Intel Corporation. All rights reserved. <BR>
Copyright (c) 2021, Ampere Computing LLC. All rights reserved. <BR>
Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TPM2_ACPI_H_
#define _TPM2_ACPI_H_

#include <IndustryStandard/Acpi.h>

#pragma pack (1)

#define EFI_TPM2_ACPI_TABLE_REVISION_3  3
#define EFI_TPM2_ACPI_TABLE_REVISION_4  4
#define EFI_TPM2_ACPI_TABLE_REVISION_5  5
#define EFI_TPM2_ACPI_TABLE_REVISION    EFI_TPM2_ACPI_TABLE_REVISION_5

#define EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_4  12
#define EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_5  16
#define EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE             EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_5

// MU_CHANGE - [BEGIN]

// Common fields shared across all TPM2 ACPI table revisions.
// Flags field is replaced in version 4 and above:
//    BIT0~15:  PlatformClass      This field is only valid for version 4 and above
//    BIT16~31: Reserved
//
#define EFI_TPM2_ACPI_TABLE_COMMON_FIELDS \
  EFI_ACPI_DESCRIPTION_HEADER    Header; \
  UINT32                         Flags; \
  UINT64                         AddressOfControlArea; \
  UINT32                         StartMethod;

typedef struct {
  EFI_TPM2_ACPI_TABLE_COMMON_FIELDS
} EFI_TPM2_ACPI_TABLE;

// MU_CHANGE - [END]

#define EFI_TPM2_ACPI_TABLE_START_METHOD_ACPI                                         2
#define EFI_TPM2_ACPI_TABLE_START_METHOD_TIS                                          6
#define EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE            7
#define EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_ACPI  8
#define EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_SMC   11
#define EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_FFA   15

typedef struct {
  UINT32    Reserved;
  UINT32    Error;
  UINT32    Cancel;
  UINT32    Start;
  UINT64    InterruptControl;
  UINT32    CommandSize;
  UINT64    Command;
  UINT32    ResponseSize;
  UINT64    Response;
} EFI_TPM2_ACPI_CONTROL_AREA;

//
// Start Method Specific Parameters for ARM SMC Start Method (11)
// Refer to Table 9: Start Method Specific Parameters for ARM SMC
//
typedef struct {
  UINT32    Interrupt;
  UINT8     Flags;
  UINT8     OperationFlags;
  UINT8     Attributes; // MU_CHANGE
  UINT8     Reserved;   // MU_CHANGE
  UINT32    SmcFunctionId;
} EFI_TPM2_ACPI_START_METHOD_SPECIFIC_PARAMETERS_ARM_SMC;

//
// Start Method Specific Parameters for ARM FFA Start Method (15)
// Reference: TCG ACPI Specification revision 1.4
// Refer to Table 11: Start Method Specific Parameters for Arm FF-A
//
typedef struct {
  UINT8     Flags;
  UINT8     Attributes;
  UINT16    PartitionId;
  UINT8     Reserved[8];
} EFI_TPM2_ACPI_START_METHOD_SPECIFIC_PARAMETERS_ARM_FFA;

// MU_CHANGE - [BEGIN]

typedef struct {
  EFI_TPM2_ACPI_TABLE_COMMON_FIELDS

  // StartMethodSpecificParameters is variable in size and LAML/LASA are
  // optional fields. It is the user's responsibility to access the
  // Header.Length field to determine what is accessible in the table.
  union {
    UINT8                                                     PlatformSpecificParameters[EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_4];
    EFI_TPM2_ACPI_START_METHOD_SPECIFIC_PARAMETERS_ARM_SMC    SmcParameters;
  } StartMethodSpecificParameters;

  UINT32    Laml; // Optional
  UINT64    Lasa; // Optional
} EFI_TPM2_ACPI_TABLE_V4;

typedef struct {
  EFI_TPM2_ACPI_TABLE_COMMON_FIELDS

  // StartMethodSpecificParameters is variable in size and LAML/LASA are
  // optional fields. It is the user's responsibility to access the
  // Header.Length field to determine what is accessible in the table.
  union {
    UINT8                                                     PlatformSpecificParameters[EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_5];
    EFI_TPM2_ACPI_START_METHOD_SPECIFIC_PARAMETERS_ARM_SMC    SmcParameters;
    EFI_TPM2_ACPI_START_METHOD_SPECIFIC_PARAMETERS_ARM_FFA    FfaParameters;
  } StartMethodSpecificParameters;

  UINT32    Laml; // Optional
  UINT64    Lasa; // Optional
} EFI_TPM2_ACPI_TABLE_V5;

typedef struct {
  EFI_TPM2_ACPI_TABLE_COMMON_FIELDS
  UINT8     PlatformSpecificParameters[EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE];
  UINT32    Laml; // Optional
  UINT64    Lasa; // Optional
} EFI_TPM2_ACPI_TABLE_TEMPLATE;

// MU_CHANGE - [END]

#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_FLAG_NOTIFICATION_SUPPORT  BIT0

#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_MEM_TYPE_MASK           0x3
#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_MEM_TYPE_SHIFT          0x0
#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_MEM_TYPE_NOT_CACHABLE   0x0
#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_MEM_TYPE_WRITE_COMBINE  0x1
#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_MEM_TYPE_WRITE_THROUGH  0x2
#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_MEM_TYPE_WRITE_BACK     0x3

#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_CRB_REGION_SIZE_MASK   0x3
#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_CRB_REGION_SIZE_SHIFT  0x2
#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_CRB_REGION_SIZE_4KB    0x0
#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_CRB_REGION_SIZE_16KB   0x1
#define EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_CRB_REGION_SIZE_64KB   0x2

#pragma pack ()

#endif
