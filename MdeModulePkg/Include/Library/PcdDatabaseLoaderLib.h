/** @file
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  Library interface to load the PCD Database from the Platform OEM Data FV.
**/

#pragma once

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
