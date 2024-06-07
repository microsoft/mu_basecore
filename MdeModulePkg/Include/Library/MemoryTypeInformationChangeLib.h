/** @file MemoryTypeInformationChangeLib.h

Used to report when memory type information changes from previous boot.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

MU_CHANGE new file

*/

#pragma once

/**
  Report the change in memory type allocations.

  @param[in] Type                   EFI memory type defined in UEFI specification
  @param[in] PreviousNumberOfPages  Number of pages retrieved from
                                    gEfiMemoryTypeInformationGuid HOB
  @param[in] NextNumberOfPages      Number of pages calculated to be used on next boot

  @retval    Status                 Status of the report
**/
RETURN_STATUS
EFIAPI
ReportMemoryTypeInformationChange (
  IN UINT32  Type,
  IN UINT32  PreviousNumberOfPages,
  IN UINT32  NextNumberOfPages
  );
