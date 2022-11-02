/**@file ExceptionPersistenceLib.h

Library enables passing exception information between boots via the platform-specific early store.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _EX_PERSIST_LIB_H_
#define _EX_PERSIST_LIB_H_

typedef enum {
  ExceptionPersistNone,
  ExceptionPersistPageFault,
  ExceptionPersistI2C,
  ExceptionPersistOther,
  ExceptionPersistMax
} EXCEPTION_TYPE;

/**
  Check if an exception was hit on a previous boot.

  @param[out]  Exception          EXCEPTION_TYPE in persistent storage
                                  NOTE: Exception IS NOT updated if the function returns an error

  @retval EFI_SUCCESS             Exception contains the result of the check
  @retval EFI_INVALID_PARAMETER   Unable to validate persistent storage contents or Exception is NULL
  @retval EFI_UNSUPPORTED         Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR        Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistGetException (
  OUT   EXCEPTION_TYPE  *Exception
  );

/**
  Write input EXCEPTION_TYPE to platform-specific persistent storage.

  @param[in]  Exception   EXCEPTION_TYPE to set in persistent storage
                          NOTE: ExceptionPersistNone has the same effect as ExPersistClearExceptions()

  @retval EFI_SUCCESS             Value set
  @retval EFI_UNSUPPORTED         NULL implementation called
  @retval EFI_INVALID_PARAMETER   Platform-specific persistent storage contents are invalid or
                                  input EXCEPTION_TYPE is invalid
  @retval EFI_DEVICE_ERROR        Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistSetException (
  IN  EXCEPTION_TYPE  Exception
  );

/**
  Clears from the platform-specific persistent storage of all exception info.

  @retval EFI_SUCCESS       Value cleared
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage contents are invalid or NULL implementation called
  @retval EFI_DEVICE_ERROR  Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistClearExceptions (
  VOID
  );

/**
  Checks if the next page fault should be ignored and cleared. Using platform-specific persistent storage
  is a means for communicating to the exception handler that the page fault was intentional. Resuming execution
  requires faulting pages to have their attributes cleared so execution can continue.

  @param[out]  IgnoreNextPageFault  Boolean TRUE if next page fault should be ignored, FALSE otherwise.
                                    Persistent storage IS NOT updated if the function returns an error.

  @retval EFI_SUCCESS             IgnoreNextPageFault contains the result of the check
  @retval EFI_INVALID_PARAMETER   Unable to validate persistent storage contents
  @retval EFI_UNSUPPORTED         Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR        Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistGetIgnoreNextPageFault (
  OUT BOOLEAN  *IgnoreNextPageFault
  );

/**
  Updates the platform-specific persistent storage to indicate that the next page fault should be ignored and cleared.
  Using platform-specific persistent storage is a means for communicating to the exception handler that the
  page fault was intentional. Resuming execution requires faulting pages to have their attributes cleared so
  execution can continue.

  @retval EFI_SUCCESS       Value set
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR  Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistSetIgnoreNextPageFault (
  VOID
  );

/**
  Clears from the platform-specific persistent storage the value indicating that the
  next page fault should be ignored.

  @retval EFI_SUCCESS       Value cleared
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR  Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistClearIgnoreNextPageFault (
  VOID
  );

/**
  Clears all values from the platform-specific persistent storage.

  @retval EFI_SUCCESS       Successfully cleared the exception bytes
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR  Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistClearAll (
  VOID
  );

#endif
