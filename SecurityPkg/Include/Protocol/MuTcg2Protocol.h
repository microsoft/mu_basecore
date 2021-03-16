/** @file
  Mu TPM2 Protocol. Exposes additional interfaces to the ones provided by Tcg2Protocol

  Copyright (c) Microsoft Corporation.  <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MU_TCG2_PROTOCOL_H__
#define __MU_TCG2_PROTOCOL_H__

#include <IndustryStandard/UefiTcgPlatform.h>
#include <IndustryStandard/Tpm20.h>
#include <Protocol/Tcg2Protocol.h>

#define MU_TCG2_PROTOCOL_VERSION        1

typedef struct tdMU_TCG2_PROTOCOL MU_TCG2_PROTOCOL;

// MU_CHANGE - START - Add a new protocol to support Log-only events.
/**
  The EFI_MU_TCG2_PROTOCOL MuLogEvent function call provides callers with
  an interface for only logging events without hashing data nor extending anything to the TPM.

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
(EFIAPI * MU_TCG2_LOG_EVENT) (
  IN MU_TCG2_PROTOCOL     *This,
  IN TPML_DIGEST_VALUES   *DigestList,
  IN EFI_TCG2_EVENT       *Event
  );
// MU_CHANGE - END - Add a new protocol to support Log-only events.

struct tdMU_TCG2_PROTOCOL {
  UINT32                                  Version;
  MU_TCG2_LOG_EVENT                       Tcg2LogEvent;
};

extern EFI_GUID gMuTcg2ProtocolExGuid;



#endif //__MU_TCG2_PROTOCOL_H__
