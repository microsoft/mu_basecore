/** @file
  SMM GetVariable Implementation to retrieve the AdvLogger memory.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADV_LOGGER_ACCESS_LIB_H__
#define __ADV_LOGGER_ACCESS_LIB_H__

/**

  This code accesses the AdvLogger private storage.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize is external input.
  This function will do basic validation, before parse the data.

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       The buffer to return the contents of the variable. May be NULL
                                    with a zero DataSize in order to determine the size buffer needed.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Found the specified data.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
AdvLoggerAccessGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  );

/**
    AdvLoggerInit - Obtain the address of the logger info block.

    @param          NONE

    @return         NONE
**/
VOID
AdvLoggerAccessInit (
  VOID
  );

/**
    AdvLoggerAccessAtRuntime - Exit Boot Services.

    @param          NONE

    @return         NONE
**/
VOID
AdvLoggerAccessAtRuntime (
  VOID
  );

/**
    AdvLoggerAccessGonevirtual - Virtual Address Change.

    @param          NONE

    @return         NONE
**/
VOID
AdvLoggerAccessGoneVirtual (
  VOID
  );

#endif // __ADV_LOGGER_ACCESS_LIB_H__
