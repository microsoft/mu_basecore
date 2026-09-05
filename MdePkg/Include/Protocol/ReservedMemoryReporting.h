/** @file
  Defines the Reserved-Memory Reporting (RMEM) registration protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RESERVED_MEMORY_REPORTING_PROTOCOL_H_
#define RESERVED_MEMORY_REPORTING_PROTOCOL_H_

#include <IndustryStandard/ReservedMemoryReportingTable.h>

#define EDKII_RMEM_REGISTRATION_PROTOCOL_GUID \
  { 0x0cb661d3, 0x3c81, 0x4074, { 0xa6, 0xa8, 0xc9, 0x75, 0x9c, 0xc5, 0x12, 0x07 } }

#define EDKII_RMEM_REGISTRATION_PROTOCOL_REVISION  1

typedef struct _EDKII_RMEM_REGISTRATION_PROTOCOL EDKII_RMEM_REGISTRATION_PROTOCOL;

/**
  Registers a reserved physical-memory range for inclusion in the RMEM ACPI
  table.

  The protocol copies the label before returning. The caller retains ownership
  of the label buffer.

  @param[in] This      A pointer to the EDKII_RMEM_REGISTRATION_PROTOCOL
                       instance.
  @param[in] Base      The physical address of the first byte in the range.
  @param[in] Size      The size of the range in bytes.
  @param[in] Category  The purpose category assigned to the range.
  @param[in] Label     An optional null-terminated ASCII diagnostic label.

  @retval EFI_SUCCESS           The range was registered.
  @retval EFI_ALREADY_STARTED   An identical range is already registered.
  @retval EFI_INVALID_PARAMETER A parameter or category value is invalid.
  @retval EFI_BAD_BUFFER_SIZE   The label exceeds RMEM_LABEL_MAX_LEN.
  @retval EFI_ACCESS_DENIED     Registration is finalized or the range overlaps
                                an existing entry.
  @retval EFI_OUT_OF_RESOURCES  The registration capacity has been reached.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_RMEM_ADD_RESERVED_RANGE)(
  IN EDKII_RMEM_REGISTRATION_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS              Base,
  IN UINT64                            Size,
  IN RMEM_CATEGORY                     Category,
  IN CONST CHAR8                       *Label OPTIONAL
  );

///
/// Provides the interface used by DXE producers to register reserved-memory
/// ranges with the RMEM ACPI table publisher.
///
struct _EDKII_RMEM_REGISTRATION_PROTOCOL {
  UINT32                           Revision;
  EDKII_RMEM_ADD_RESERVED_RANGE    AddReservedRange;
};

extern EFI_GUID  gEdkiiRmemRegistrationProtocolGuid;

#endif
