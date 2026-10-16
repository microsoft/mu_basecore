/** @file
  Produces the Random Number Generator protocol in MM.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/RngLib.h>
#include <Protocol/Rng.h>

STATIC
EFI_STATUS
EFIAPI
RngGetInfo (
  IN EFI_RNG_PROTOCOL    *This,
  IN OUT UINTN           *RNGAlgorithmListSize,
  OUT EFI_RNG_ALGORITHM  *RNGAlgorithmList
  );

STATIC
EFI_STATUS
EFIAPI
RngGetRNG (
  IN EFI_RNG_PROTOCOL   *This,
  IN EFI_RNG_ALGORITHM  *RNGAlgorithm OPTIONAL,
  IN UINTN              RNGValueLength,
  OUT UINT8             *RNGValue
  );

STATIC EFI_RNG_PROTOCOL  mRngProtocol = {
  RngGetInfo,
  RngGetRNG
};

STATIC
EFI_STATUS
EFIAPI
RngGetInfo (
  IN EFI_RNG_PROTOCOL    *This,
  IN OUT UINTN           *RNGAlgorithmListSize,
  OUT EFI_RNG_ALGORITHM  *RNGAlgorithmList
  )
{
  EFI_STATUS         Status;
  EFI_RNG_ALGORITHM  RngAlgorithm;

  if ((This == NULL) || (RNGAlgorithmListSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*RNGAlgorithmListSize < sizeof (RngAlgorithm)) {
    *RNGAlgorithmListSize = sizeof (RngAlgorithm);
    return EFI_BUFFER_TOO_SMALL;
  }

  if (RNGAlgorithmList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetRngGuid (&RngAlgorithm);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (RNGAlgorithmList, &RngAlgorithm, sizeof (RngAlgorithm));
  *RNGAlgorithmListSize = sizeof (RngAlgorithm);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
RngGetRNG (
  IN EFI_RNG_PROTOCOL   *This,
  IN EFI_RNG_ALGORITHM  *RNGAlgorithm OPTIONAL,
  IN UINTN              RNGValueLength,
  OUT UINT8             *RNGValue
  )
{
  EFI_STATUS         Status;
  EFI_RNG_ALGORITHM  RngLibAlgorithm;
  UINT64             RandomValue;
  UINTN              BytesToCopy;

  if ((This == NULL) || (RNGValueLength == 0) || (RNGValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetRngGuid (&RngLibAlgorithm);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((RNGAlgorithm != NULL) && !CompareGuid (RNGAlgorithm, &RngLibAlgorithm)) {
    return EFI_UNSUPPORTED;
  }

  while (RNGValueLength > 0) {
    if (!GetRandomNumber64 (&RandomValue)) {
      return EFI_NOT_READY;
    }

    BytesToCopy = MIN (RNGValueLength, sizeof (RandomValue));
    CopyMem (RNGValue, &RandomValue, BytesToCopy);
    RNGValue       += BytesToCopy;
    RNGValueLength -= BytesToCopy;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RngMmEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS         Status;
  EFI_HANDLE         Handle;
  EFI_RNG_ALGORITHM  RngAlgorithm;
  UINT64             RandomValue;

  Status = GetRngGuid (&RngAlgorithm);
  if (EFI_ERROR (Status) || !GetRandomNumber64 (&RandomValue)) {
    return EFI_UNSUPPORTED;
  }

  Handle = NULL;
  return gMmst->MmInstallProtocolInterface (
                  &Handle,
                  &gEfiRngProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mRngProtocol
                  );
}
