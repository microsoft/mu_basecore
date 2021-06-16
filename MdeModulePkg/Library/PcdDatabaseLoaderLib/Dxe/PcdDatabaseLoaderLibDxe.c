/** @file -- PcdDatabaseLoaderLibDxe.c

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Pcd Database Loader.

**/

#include <Base.h>

#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/PcdDatabaseLoaderLib.h>

//
// The DXE Pcd Database is expected to be in a Hob created by the the PEI Pcd Database loader.
// This is done as these databases are located in the FLASH_REGION_SCPC_PRODUCTDATA FV, and this
// FV is not loaded on the certified path.
//

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
  VOID        *DxePcdDatabaseBinary;
  UINTN       DxePcdDatabaseBinarySize;
  EFI_STATUS  Status;

  //
  // Search the External Pcd database from one section of current FFS,
  // and read it to memory
  //
  Status = GetSectionFromFfs (
             EFI_SECTION_RAW,
             0,
             (VOID **)&DxePcdDatabaseBinary,
             &DxePcdDatabaseBinarySize
             );

  ASSERT_EFI_ERROR (Status);

  return DxePcdDatabaseBinary;
}
