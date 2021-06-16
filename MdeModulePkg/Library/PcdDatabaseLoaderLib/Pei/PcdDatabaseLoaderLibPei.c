/** @file -- PcdDatabaseLoaderLibPei.c

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  PcdBridge support driver.

**/

#include <Base.h>

#include <Library/DebugLib.h>
#include <Library/PcdDatabaseLoaderLib.h>
#include <Library/PeiServicesLib.h>

/**
Function to load the PcdDatabase.

@param[in]  Context  - For PEI, this is a file handle of the Pcd driver
                       For DXE, this parameter is ignored, and should be NULL

@retval       Pointer to Pei Pcd Database binary
@retval       NULL if no PcdDatabase is found

**/
VOID *
EFIAPI
PcdDatabaseLoaderLoad (
  VOID  *FileHandle
  )
{
  VOID        *PcdDb;
  EFI_STATUS  Status;

  Status = PeiServicesFfsFindSectionData (EFI_SECTION_RAW, FileHandle, &PcdDb);
  ASSERT_EFI_ERROR (Status);

  return PcdDb;
}
