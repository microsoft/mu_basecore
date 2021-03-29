/** @file
  It updates TPM2 items in ACPI table and registers SMI2 callback
  functions for Tcg2 physical presence, ClearMemory, and sample
  for dTPM StartMethod.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable and ACPINvs data in SMM mode.
  This external input must be validated carefully to avoid security issue.

  PhysicalPresenceCallback() and MemoryClearCallback() will receive untrusted input and do some check.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Guid/TpmInstance.h>
#include <Guid/TpmNvsMm.h>

#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/Tcg2Protocol.h>
#include <Protocol/MmReadyToLock.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/StandaloneMmMemLib.h>

#include <IndustryStandard/TpmPtp.h>

TCG_NVS                    *mTcgNvs = NULL;
UINT8                       mPpSoftwareSmi;
EFI_HANDLE                  mReadyToLockHandle = NULL;

/**
  Communication service SMI Handler entry.

  This handler takes requests to exchange Mmi channel and Nvs address between MM and DXE.

  Caution: This function may receive untrusted input.
  Communicate buffer and buffer size are external input, so this function will do basic validation.

  @param[in]      DispatchHandle    The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]      RegisterContext   Points to an optional handler context which was specified when the
                                    handler was registered.
  @param[in, out] CommBuffer        A pointer to a collection of data in memory that will
                                    be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize    The size of the CommBuffer.

  @retval EFI_SUCCESS               The interrupt was handled and quiesced. No other handlers
                                    should still be called.
  @retval EFI_UNSUPPORTED           An unknown test function was requested.
  @retval EFI_ACCESS_DENIED         Part of the communication buffer lies in an invalid region.

**/
EFI_STATUS
EFIAPI
TpmNvsCommunciate (
  IN     EFI_HANDLE                   DispatchHandle,
  IN     CONST VOID                   *RegisterContext,
  IN OUT VOID                         *CommBuffer,
  IN OUT UINTN                        *CommBufferSize
  )
{
  EFI_STATUS                Status;
  UINTN                     TempCommBufferSize;
  TPM_NVS_MM_COMM_BUFFER    *CommParams;

  DEBUG ((DEBUG_VERBOSE, "%a()\n", __FUNCTION__));

  //
  // If input is invalid, stop processing this SMI
  //
  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;

  if(TempCommBufferSize != sizeof (TPM_NVS_MM_COMM_BUFFER)) {
    DEBUG ((DEBUG_ERROR, "[%a] MM Communication buffer size is invalid for this handler!\n", __FUNCTION__));
    return EFI_ACCESS_DENIED;
  }
  if (!MmIsBufferOutsideMmValid ((UINTN) CommBuffer, TempCommBufferSize)) {
    DEBUG ((DEBUG_ERROR, "[%a] - MM Communication buffer in invalid location!\n", __FUNCTION__));
    return EFI_ACCESS_DENIED;
  }

  //
  // Farm out the job to individual functions based on what was requested.
  //
  CommParams = (TPM_NVS_MM_COMM_BUFFER*) CommBuffer;
  Status = EFI_SUCCESS;
  switch (CommParams->Function) {
    case TpmNvsMmExchangeInfo:
      DEBUG ((DEBUG_VERBOSE, "[%a] - Function requested: MM_EXCHANGE_NVS_INFO\n", __FUNCTION__));
      CommParams->RegisteredPpSwiValue = mPpSoftwareSmi;
      mTcgNvs = (TCG_NVS*)(UINTN) CommParams->TargetAddress;
      break;

    default:
      DEBUG ((DEBUG_INFO, "[%a] - Unknown function %d!\n", __FUNCTION__, CommParams->Function));
      Status = EFI_UNSUPPORTED;
      break;
  }

  CommParams->ReturnStatus = Status;
  return EFI_SUCCESS;
}

/**
  Notification for SMM ReadyToLock protocol.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification runs successfully.

**/
EFI_STATUS
EFIAPI
TcgMmReadyToLock (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
)
{
  EFI_STATUS Status;

  if (mReadyToLockHandle != NULL) {
    Status = gMmst->MmiHandlerUnRegister (mReadyToLockHandle);
    mReadyToLockHandle = NULL;
  }
  return Status;
}

/**
  Software SMI callback for TPM physical presence which is called from ACPI method.

  Caution: This function may receive untrusted input.
  Variable and ACPINvs are external input, so this function will validate
  its data structure to be valid value.

  @param[in]      DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]      Context         Points to an optional handler context which was specified when the
                                  handler was registered.
  @param[in, out] CommBuffer      A pointer to a collection of data in memory that will
                                  be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS             The interrupt was handled successfully.

**/
EFI_STATUS
EFIAPI
PhysicalPresenceCallback (
  IN EFI_HANDLE                  DispatchHandle,
  IN CONST VOID                  *Context,
  IN OUT VOID                    *CommBuffer,
  IN OUT UINTN                   *CommBufferSize
  )
{
  UINT32                MostRecentRequest;
  UINT32                Response;
  UINT32                OperationRequest;
  UINT32                RequestParameter;

  // If ACPI driver is not run, still cannot host this request
  if (mTcgNvs == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (mTcgNvs->PhysicalPresence.Parameter == TCG_ACPI_FUNCTION_RETURN_REQUEST_RESPONSE_TO_OS) {
    mTcgNvs->PhysicalPresence.ReturnCode = Tcg2PhysicalPresenceLibReturnOperationResponseToOsFunction (
                                             &MostRecentRequest,
                                             &Response
                                             );
    mTcgNvs->PhysicalPresence.LastRequest = MostRecentRequest;
    mTcgNvs->PhysicalPresence.Response = Response;
    return EFI_SUCCESS;
  } else if ((mTcgNvs->PhysicalPresence.Parameter == TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS)
          || (mTcgNvs->PhysicalPresence.Parameter == TCG_ACPI_FUNCTION_SUBMIT_REQUEST_TO_BIOS_2)) {

    OperationRequest = mTcgNvs->PhysicalPresence.Request;
    RequestParameter = mTcgNvs->PhysicalPresence.RequestParameter;
    mTcgNvs->PhysicalPresence.ReturnCode = Tcg2PhysicalPresenceLibSubmitRequestToPreOSFunctionEx (
                                             &OperationRequest,
                                             &RequestParameter
                                             );
    mTcgNvs->PhysicalPresence.Request = OperationRequest;
    mTcgNvs->PhysicalPresence.RequestParameter = RequestParameter;
  } else if (mTcgNvs->PhysicalPresence.Parameter == TCG_ACPI_FUNCTION_GET_USER_CONFIRMATION_STATUS_FOR_REQUEST) {
    mTcgNvs->PhysicalPresence.ReturnCode = Tcg2PhysicalPresenceLibGetUserConfirmationStatusFunction (mTcgNvs->PPRequestUserConfirm);
  }

  return EFI_SUCCESS;
}

/**
  The driver's entry point.

  It install callbacks for TPM physical presence and MemoryClear, and locate
  SMM variable to be used in the callback function.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeTcgSmm (
  IN EFI_HANDLE                     ImageHandle,
  IN EFI_MM_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT    SwContext;
  EFI_HANDLE                     SwHandle;
  EFI_HANDLE                     DiscardedHandle;

  SwHandle           = NULL;
  DiscardedHandle    = NULL;
  SwHandle           = NULL;

  if (!CompareGuid (PcdGetPtr(PcdTpmInstanceGuid), &gEfiTpmDeviceInstanceTpm20DtpmGuid)){
    DEBUG ((EFI_D_ERROR, "No TPM2 DTPM instance required!\n"));
    Status = EFI_UNSUPPORTED;
    goto Cleanup;
  }

  // Register a root handler to communicate the NVS region and SMI channel between MM and DXE
  Status = gMmst->MmiHandlerRegister (TpmNvsCommunciate, &gTpmNvsMmGuid, &mReadyToLockHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to register PP callback as SW MM handler - %r!\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  //
  // Get the Sw dispatch protocol and register SMI callback functions.
  //
  SwDispatch = NULL;
  Status = gMmst->MmLocateProtocol (&gEfiSmmSwDispatch2ProtocolGuid, NULL, (VOID**)&SwDispatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to locate MM Sw handler - %r!\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  SwContext.SwSmiInputValue = (UINTN) -1;
  SwHandle = NULL;
  Status = SwDispatch->Register (SwDispatch, PhysicalPresenceCallback, &SwContext, &SwHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to register PP callback as MM Sw handler - %r!\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }
  mPpSoftwareSmi = (UINT8) SwContext.SwSmiInputValue;

  // Turn off the light before leaving the room... at least, take a remote...
  DiscardedHandle = NULL;
  Status = gMmst->MmRegisterProtocolNotify (&gEfiMmReadyToLockProtocolGuid, TcgMmReadyToLock, &DiscardedHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to register ready to lock notification - %r!\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

Cleanup:
  if (EFI_ERROR (Status)) {
    // Something is whacked, clean up the mess...
    if (DiscardedHandle != NULL) {
      gMmst->MmRegisterProtocolNotify (&gEfiMmReadyToLockProtocolGuid, NULL, &DiscardedHandle);
    }
    if (SwHandle != NULL && SwDispatch != NULL) {
      SwDispatch->UnRegister (SwDispatch, SwDispatch);
    }
    if (mReadyToLockHandle != NULL) {
      gMmst->MmiHandlerUnRegister (DiscardedHandle);
    }
  }
  return Status;
}
