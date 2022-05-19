/** @file -- RngLibHostTestLfsr.c
A minimal implementation of RngLib that supports host based testing
with a simple LFSR:
https://en.wikipedia.org/wiki/Linear-feedback_shift_register

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/RngLib.h>

STATIC
UINT16
LfsrXorshift16 (
  VOID
  )
{
  static UINT16  Lfsr = 0xACE1;         /* Any nonzero start state will work. */

  // 7,9,13 triplet from http://www.retroprogramming.com/2017/07/xorshift-pseudorandom-numbers-in-z80.html
  Lfsr ^= Lfsr >> 7;
  Lfsr ^= Lfsr << 9;
  Lfsr ^= Lfsr >> 13;

  return Lfsr;
}

/**
  Generates a 16-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 16-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber16 (
  OUT     UINT16  *Rand
  )
{
  *Rand = LfsrXorshift16 ();
  return TRUE;
}

/**
  Generates a 32-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 32-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber32 (
  OUT     UINT32  *Rand
  )
{
  UINT16  *Marker;

  Marker = (UINT16 *)Rand;
  GetRandomNumber16 (&Marker[0]);
  GetRandomNumber16 (&Marker[1]);
  return TRUE;
}

/**
  Generates a 64-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 64-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber64 (
  OUT     UINT64  *Rand
  )
{
  UINT32  *Marker;

  Marker = (UINT32 *)Rand;
  GetRandomNumber32 (&Marker[0]);
  GetRandomNumber32 (&Marker[1]);
  return TRUE;
}

/**
  Generates a 128-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 128-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber128 (
  OUT     UINT64  *Rand
  )
{
  UINT64  *Marker;

  Marker = (UINT64 *)Rand;
  GetRandomNumber64 (&Marker[0]);
  GetRandomNumber64 (&Marker[1]);
  return TRUE;
}
