/**

File: MemoryTypeInformationChangeLib.h
Used to report when memory type information changes from previous boot.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

MU_CHANGE new file

*/

#ifndef __MEMORY_TYPE_INFORMATION_CHANGE_LIB__
#define __MEMORY_TYPE_INFORMATION_CHANGE_LIB__

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
  );

#endif //__MEMORY_TYPE_INFORMATION_CHANGE_LIB__
