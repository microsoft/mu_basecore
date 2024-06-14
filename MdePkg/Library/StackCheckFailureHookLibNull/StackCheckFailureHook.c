/** @file
  Library provides a hook called when a stack cookie check fails.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>

/**
  Initialize the security cookie.
**/
NO_STACK_COOKIE
VOID
EFIAPI
StackCheckFailureHook (
  VOID  *FailureAddress
  )
{
  return;
}
