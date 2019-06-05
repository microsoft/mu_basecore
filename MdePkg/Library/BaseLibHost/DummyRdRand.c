/** @file

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Generates a 16-bit random number through RDRAND instruction.

  @param[out]  Rand     Buffer pointer to store the random result.

  @retval TRUE          RDRAND call was successful.
  @retval FALSE         Failed attempts to call RDRAND.

 **/
BOOLEAN
EFIAPI
InternalX86RdRand16 (
  OUT     UINT16                    *Rand
  )
{
  return TRUE;
}

/**
  Generates a 32-bit random number through RDRAND instruction.

  @param[out]  Rand     Buffer pointer to store the random result.

  @retval TRUE          RDRAND call was successful.
  @retval FALSE         Failed attempts to call RDRAND.

**/
BOOLEAN
EFIAPI
InternalX86RdRand32 (
  OUT     UINT32                    *Rand
  )
{
  return TRUE;
}

/**
  Generates a 64-bit random number through RDRAND instruction.


  @param[out]  Rand     Buffer pointer to store the random result.

  @retval TRUE          RDRAND call was successful.
  @retval FALSE         Failed attempts to call RDRAND.

**/
BOOLEAN
EFIAPI
InternalX86RdRand64  (
  OUT     UINT64                    *Rand
  )
{
  return TRUE;
}
