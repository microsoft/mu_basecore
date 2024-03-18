/** @file
  The Variable Storage Support Protocol is specific for the EDK II implementation of UEFI variables.

  This protocol is produced by the UEFI variable driver. It contains services the UEFI variable driver provides
  to variable storage drivers.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef VARIABLE_STORAGE_SUPPORT_PROTOCOL_H_
#define VARIABLE_STORAGE_SUPPORT_PROTOCOL_H_

#define EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL_GUID \
  { \
    0x9c6e6668, 0x371d, 0x45f4, { 0x80, 0x48, 0xfd, 0xd1, 0x62, 0x2f, 0xd1, 0xf9 } \
  }

///
/// Revision
///
#define EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL_REVISION  1

typedef struct _EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL;

/**
  Notifies the UEFI variable driver that the variable storage driver's WriteServiceIsReady() function
  is now returning TRUE instead of FALSE.

  Variable storage drivers should call this function as soon as possible.

  The UEFI variable driver will delay producing the Variable Write Architectural Protocol
  (gEfiVariableWriteArchProtocolGuid) until all present variable storage protocols return TRUE
  from EDKII_VARIABLE_STORAGE_PROTOCOL.WriteServiceIsReady(). The UEFI variable driver will query
  the WriteServiceIsReady() status from each present variable storage protocol instance on each
  invocation of this function.

**/
typedef
VOID
(EFIAPI *EDKII_VARIABLE_STORAGE_SUPPORT_NOTIFY_WRITE_SERVICE_READY)(
  VOID
  );

///
/// Variable Storage Support Protocol
/// Interface functions for variable storage driver to access core variable driver functions in DXE/SMM phase.
///
struct _EDKII_VARIABLE_STORAGE_SUPPORT_PROTOCOL {
  EDKII_VARIABLE_STORAGE_SUPPORT_NOTIFY_WRITE_SERVICE_READY    NotifyWriteServiceReady;   ///< Notify variable writes are available in a variable driver
};

extern EFI_GUID  gEdkiiVariableStorageSupportProtocolGuid;

#endif
