/**

This library implements the necessary functions
to log hardware and software security locks for post-processing

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SECURITY_LOCK_LIB_H__
#define __SECURITY_LOCK_LIB_H__

#define SECURITY_LOCK_REPORT_EVENT(LockMessage, LockType) \
      SecurityLockReportEvent(&gEfiCallerIdGuid,__FUNCTION__,LockMessage,LockType); \


/**

Enum to hold the various lock types for use in post-processing

**/
typedef enum {
  SOFTWARE_LOCK = 0,
  HARDWARE_LOCK,
} LOCK_TYPE;

/**
  Function for security Lock event logging and reporting

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
  );

#endif
