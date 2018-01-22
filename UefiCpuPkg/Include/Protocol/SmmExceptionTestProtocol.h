/** @file -- SmmExceptionTestProtocol.h
A simple protocol to enable SMM exception handling being placed in test mode.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _SMM_EXCEPTION_TEST_PROTOCOL_H_
#define _SMM_EXCEPTION_TEST_PROTOCOL_H_

// B76383A1-0E70-4A3F-86B4-C6134C8E5723
#define SMM_EXCEPTION_TEST_PROTOCOL_GUID \
  { \
    0xb76383a1, 0x0e70, 0x4a3f, { 0x86, 0xb4, 0xc6, 0x13, 0x4c, 0x8e, 0x57, 0x23 } \
  }

extern EFI_GUID  gSmmExceptionTestProtocolGuid;

/**
  Enable exception handling test mode.

  NOTE: This should only work on debug builds, otherwise return EFI_UNSUPPORTED.

  @retval EFI_SUCCESS            Test mode enabled.
  @retval EFI_UNSUPPORTED        Test mode could not be enabled.
**/
typedef
EFI_STATUS
(EFIAPI *SMM_ENABLE_EXCEPTION_TEST_MODE)(
  VOID
  );

typedef struct _SMM_EXCEPTION_TEST_PROTOCOL {
  SMM_ENABLE_EXCEPTION_TEST_MODE    EnableTestMode;
} SMM_EXCEPTION_TEST_PROTOCOL;

#endif // _SMM_EXCEPTION_TEST_PROTOCOL_H_
