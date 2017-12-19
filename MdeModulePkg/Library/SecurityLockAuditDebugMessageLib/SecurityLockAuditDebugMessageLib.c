/** @file
  This library implements the necessary functions
  to log hardware and software security locks for post-processing

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h> // MU_CHANGE
#include <Library/SecurityLockAuditLib.h>
#include <Library/DebugLib.h>

// Used to look up lock name from LOCK_TYPE enum
CHAR8  *mLockName[] = {
  "SOFTWARE_LOCK",
  "HARDWARE_LOCK"
};

/**
  Function for security Lock event logging and reporting

  @param[in] Module                   GUID of calling module
  @param[in] Function                 Name of calling function
  @param[in] LockEventText            Debug message explaining what is locked
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
  UINTN  LockTypeIndex = (UINTN)LockType;
  // Get count of strings in LOCK_NAME
  UINTN  LockNameCount = sizeof (mLockName)/sizeof (mLockName[0]);

  if (LockTypeIndex < LockNameCount) {
    DEBUG ((DEBUG_ERROR, "SecurityLock::LockType: %a, Module: %g, Function: %a, Output: %a\n", mLockName[LockTypeIndex], Module, Function, LockEventText));
  } else {
    DEBUG ((DEBUG_ERROR, "SecurityLock::LockType: %d, Module: %g, Function: %a, Output: %a\n", LockType, Module, Function, LockEventText));
  }
}
