/** @file -- Tcg2PreUefiEventLogLib.h
  This describes the interface that should be published by instances of the
  Tcg2PreUefiEventLogLib. This library can be used to publish TPM EventLog
  entries for measurements that may have been made prior to driver
  initialization.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

/**
  Create the EventLog entries.
**/
VOID
EFIAPI
CreateTcg2PreUefiEventLogEntries (
  VOID
  );
