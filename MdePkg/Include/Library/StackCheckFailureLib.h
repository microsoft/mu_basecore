/** @file
  Library provides the handler function for stack cookie check failures.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef STACK_COOKIE_FAILURE_HANDLER_LIB_H_
#define STACK_COOKIE_FAILURE_HANDLER_LIB_H_

#include <Uefi.h>

VOID
EFIAPI
StackCheckFailure (
  VOID
  );

#endif
