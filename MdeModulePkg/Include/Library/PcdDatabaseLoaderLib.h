/** @file
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Library interface to load the PCD Database from the Platform OEM Data FV.
**/

#ifndef _PCD_DATABASE_LOADER_LIB_H_
#define _PCD_DATABASE_LOADER_LIB_H_

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
  IN VOID  *Context
  );

#endif //_PCD_DATABASE_LOADER_LIB_H_
