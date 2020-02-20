/** @file

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

void
__CxxFrameHandler3 (void)
{
  CpuDeadLoop();
}