/** @file -- Tcg2PreUefiEventLogLib.h
  This describes the interface that should be published by instances of the
  Tcg2PreUefiEventLogLib. This library can be used to publish TPM EventLog
  entries for measurements that may have been made prior to driver
  initialization.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TCG_2_PRE_UEFI_EVENT_LOG_LIB_H_
#define TCG_2_PRE_UEFI_EVENT_LOG_LIB_H_

/**
  Create the EventLog entries.
**/
VOID
EFIAPI
CreateTcg2PreUefiEventLogEntries (
  VOID
  );

#endif // TCG_2_PRE_UEFI_EVENT_LOG_LIB_H_
