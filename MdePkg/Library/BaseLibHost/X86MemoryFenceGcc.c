/** @file
  GCC inline implementation of BaseLib processor specific functions.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


/**
  Used to serialize load and store operations.

  All loads and stores that proceed calls to this function are guaranteed to be
  globally visible when this function returns.

**/
VOID
EFIAPI
MemoryFence (
  VOID
  )
{
  // This is a little bit of overkill and it is more about the compiler that it is
  // actually processor synchronization. This is like the _ReadWriteBarrier
  // Microsoft specific intrinsic
  __asm__ __volatile__ ("":::"memory");
}

