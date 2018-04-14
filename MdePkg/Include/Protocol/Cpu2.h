/** @file
  CPU2 Protocol

  This code abstracts the DXE core from processor implementation details.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PROTOCOL_CPU2_H__
#define __PROTOCOL_CPU2_H__

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
