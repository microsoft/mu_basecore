/** @file
  Log protocol designed to allow logging events without extending
  to the TPM.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LOG_PROTOCOL_H_
#define LOG_PROTOCOL_H_

#include <Protocol/Tcg2Protocol.h>

#define LOG_PROTOCOL_GUID \
  { 0xA158DDD1, 0xF3EA, 0x49C6, { 0x9A, 0xBA, 0x33, 0x5A, 0x75, 0x1E, 0x8D, 0x94 }}

#define LOG_PROTOCOL_VERSION  1

typedef struct tdLOG_PROTOCOL LOG_PROTOCOL;

/**
  Provides callers with an interface for only logging events without hashing
  data nor extending anything to the TPM.

  @param[in]  This               Indicates the calling context
  @param[in]  DigestList         Pointer to a list of digest values.
  @param[in]  EfiTcgEvent        Pointer to data buffer containing information about the event.

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_DEVICE_ERROR       The command was unsuccessful.
  @retval EFI_INVALID_PARAMETER  One or more of the parameters are incorrect.
  @retval EFI_OUT_OF_RESOURCES   No enough memory to log the new event.
**/
typedef
EFI_STATUS
(EFIAPI *LOG_EVENT)(
  IN LOG_PROTOCOL         *This,
  IN TPML_DIGEST_VALUES   *DigestList,
  IN EFI_TCG2_EVENT       *Event
  );

struct tdLOG_PROTOCOL {
  UINT32       Version;
  LOG_EVENT    LogEvent;
};

extern EFI_GUID  gLogProtocolGuid;

#endif
