/** @file
  CPU2 Protocol

  This code abstracts the DXE core from processor implementation details.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PROTOCOL_CPU2_H_
#define PROTOCOL_CPU2_H_

#define EFI_CPU2_PROTOCOL_GUID \
  { 0x55198405, 0x26c0, 0x4765, {0x8b, 0x7d, 0xbe, 0x1d, 0xf5, 0xf9, 0x97, 0x12 } }

typedef struct _EFI_CPU2_PROTOCOL EFI_CPU2_PROTOCOL;

/**
  This function enables CPU interrupts and then waits for an interrupt to arrive.

  @param  This             The EFI_CPU2_PROTOCOL instance.

  @retval EFI_SUCCESS           Interrupts are enabled on the processor.
  @retval EFI_DEVICE_ERROR      Interrupts could not be enabled on the processor.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_ENABLE_AND_WAIT_FOR_INTERRUPT)(
  IN EFI_CPU2_PROTOCOL              *This
  );

///
/// The EFI_CPU2_PROTOCOL is used to abstract processor-specific functions from the DXE
/// Foundation.
///
struct _EFI_CPU2_PROTOCOL {
  EFI_CPU_ENABLE_AND_WAIT_FOR_INTERRUPT    EnableAndForWaitInterrupt;
};

extern EFI_GUID  gEfiCpu2ProtocolGuid;

#endif
