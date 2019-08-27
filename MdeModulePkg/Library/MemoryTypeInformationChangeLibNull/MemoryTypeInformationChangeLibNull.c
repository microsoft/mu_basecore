/** @file -- MemoryTypeInformationChangeLibNull.c
 Null library, returns success.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

MU_CHANGE NEW FILE
**/

#include <Library/BaseLib.h>
#include <Library/MemoryTypeInformationChangeLib.h>

/**
  Report the change in memory type allocations.

  @param[in] Type                   EFI memory type defined in UEFI specification
  @param[in] PreviousNumberOfPages  Number of pages retrieved from
                                    gEfiMemoryTypeInformationGuid HOB
  @param[in] NextNumberOfPages      Number of pages calculated to be used on next boot

  @retval    Status                 Status of the report
**/
RETURN_STATUS
ReportMemoryTypeInformationChange (
  IN UINT32  Type,
  IN UINT32  PreviousNumberOfPages,
  IN UINT32  NextNumberOfPages
  )
{
  return RETURN_SUCCESS;
}
