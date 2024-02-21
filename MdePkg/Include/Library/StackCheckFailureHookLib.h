/** @file
  Library provides a hook called when a stack cookie check fails.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef STACK_COOKIE_FAILURE_HOOK_LIB_H_
#define STACK_COOKIE_FAILURE_HOOK_LIB_H_

#include <Uefi.h>

NO_STACK_COOKIE
VOID
EFIAPI
StackCheckFailureHook (
  VOID  *FailureAddress
  );

#endif
