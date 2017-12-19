/** @file

  Null library for security lock logging that does nothing but meet compile requirements

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/SecurityLockAuditLib.h>

/**
  Null function for security Lock event logging and reporting

  @param[in] Module                   GUID of calling module
  @param[in] Function                 Name of calling function
  @param[in] LockEventText            Event text explaining what is locked
  @param[in] LockType                 Enumerated lock type for differentiation

**/
VOID
EFIAPI
SecurityLockReportEvent (
  IN GUID         *Module,
  IN CONST CHAR8  *Function,
  IN CONST CHAR8  *LockEventText,
  IN LOCK_TYPE    LockType
  )
{
}
