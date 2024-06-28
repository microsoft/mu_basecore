/** @file
  Provides an interface for recovering from page faults without rebooting.

  NOTE: This API should not be installed if the platform does not want to allow
        page faults to be cleared. For most cases, this API should only be installed
        on platforms in manufacturing mode to test the function of memory protections.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MEMORY_PROTECTION_NONSTOP_MODE_H_
#define MEMORY_PROTECTION_NONSTOP_MODE_H_

#define MEMORY_PROTECTION_NONSTOP_MODE_PROTOCOL_GUID \
  { \
    {0x896306BA, 0x1722, 0x414D, {0x93, 0xCF, 0x14, 0x24, 0x4F, 0x5E, 0x31, 0x5D } \
  }

typedef struct _MEMORY_PROTECTION_NONSTOP_MODE_PROTOCOL MEMORY_PROTECTION_NONSTOP_MODE_PROTOCOL;

/**
  Marks the faulting pages as present, R/W, and executable so execution can resume.

  @param[in] ExceptionType  Exception type.
  @param[in] SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

  @retval EFI_SUCCESS              Page attributes were cleared.
  @retval RETURN_ACCESS_DENIED     The attributes for the page could not be modified.
  @retval RETURN_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                   the memory resource range.
**/
typedef
EFI_STATUS
(EFIAPI *CLEAR_PAGE_FAULT)(
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  );

/**
  Restores the page table attributes of a page whose attributes were cleared via a call to
  CLEAR_PAGE_FAULT.

**/
typedef
EFI_STATUS
(EFIAPI *RESET_PAGE_ATTRIBUTES)(
  VOID
  );

///
/// EFI Nonstop Protocol provides services to clear a page fault
/// by reseting the faulting page's attributes and to restore
/// the previously faulting page to its original attributes.
///
struct _MEMORY_PROTECTION_NONSTOP_MODE_PROTOCOL {
  CLEAR_PAGE_FAULT         ClearPageFault;
  RESET_PAGE_ATTRIBUTES    ResetPageAttributes;
};

extern EFI_GUID  gMemoryProtectionNonstopModeProtocolGuid;

#endif
