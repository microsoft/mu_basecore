/** @file
  Provides Set/Get time operations.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2018 - 2020, ARM Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DxeServicesTableLib.h>
#include "PcRtc.h"

PC_RTC_MODULE_GLOBALS  mModuleGlobal;

EFI_HANDLE  mHandle = NULL;

STATIC EFI_EVENT  mVirtualAddrChangeEvent;

UINTN   mRtcIndexRegister;
UINTN   mRtcTargetRegister;
UINT16  mRtcDefaultYear;
UINT16  mMinimalValidYear;
UINT16  mMaximalValidYear;

/**
  Returns the current time and date information, and the time-keeping capabilities
  of the hardware platform.

  @param  Time          A pointer to storage to receive a snapshot of the current time.
  @param  Capabilities  An optional pointer to a buffer to receive the real time
                        clock device's capabilities.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  Time is NULL.
  @retval EFI_DEVICE_ERROR       The time could not be retrieved due to hardware error.

**/
EFI_STATUS
EFIAPI
PcRtcEfiGetTime (
  OUT EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities  OPTIONAL
  )
{
  return PcRtcGetTime (Time, Capabilities, &mModuleGlobal);
}

/**
  Sets the current local time and date information.

  @param  Time                   A pointer to the current time.

  @retval EFI_SUCCESS            The operation completed successfully.
  @retval EFI_INVALID_PARAMETER  A time field is out of range.
  @retval EFI_DEVICE_ERROR       The time could not be set due due to hardware error.

**/
EFI_STATUS
EFIAPI
PcRtcEfiSetTime (
  IN EFI_TIME  *Time
  )
{
  return PcRtcSetTime (Time, &mModuleGlobal);
}

/**
  Returns the current wakeup alarm clock setting.

  @param  Enabled  Indicates if the alarm is currently enabled or disabled.
  @param  Pending  Indicates if the alarm signal is pending and requires acknowledgement.
  @param  Time     The current alarm setting.

  @retval EFI_SUCCESS           The alarm settings were returned.
  @retval EFI_INVALID_PARAMETER Enabled is NULL.
  @retval EFI_INVALID_PARAMETER Pending is NULL.
  @retval EFI_INVALID_PARAMETER Time is NULL.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be retrieved due to a hardware error.
  @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.

**/
EFI_STATUS
EFIAPI
PcRtcEfiGetWakeupTime (
  OUT BOOLEAN   *Enabled,
  OUT BOOLEAN   *Pending,
  OUT EFI_TIME  *Time
  )
{
  return PcRtcGetWakeupTime (Enabled, Pending, Time, &mModuleGlobal);
}

/**
  Sets the system wakeup alarm clock time.

  @param  Enabled  Enable or disable the wakeup alarm.
  @param  Time     If Enable is TRUE, the time to set the wakeup alarm for.
                   If Enable is FALSE, then this parameter is optional, and may be NULL.

  @retval EFI_SUCCESS            If Enable is TRUE, then the wakeup alarm was enabled.
                                 If Enable is FALSE, then the wakeup alarm was disabled.
  @retval EFI_INVALID_PARAMETER  A time field is out of range.
  @retval EFI_DEVICE_ERROR       The wakeup time could not be set due to a hardware error.
  @retval EFI_UNSUPPORTED        A wakeup timer is not supported on this platform.

**/
EFI_STATUS
EFIAPI
PcRtcEfiSetWakeupTime (
  IN BOOLEAN   Enabled,
  IN EFI_TIME  *Time       OPTIONAL
  )
{
  return PcRtcSetWakeupTime (Enabled, Time, &mModuleGlobal);
}

/**
  Fixup internal data so that EFI can be called in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
STATIC
VOID
EFIAPI
VirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  // Only needed if you are going to support the OS calling RTC functions in
  // virtual mode. You will need to call EfiConvertPointer (). To convert any
  // stored physical addresses to virtual address. After the OS transitions to
  // calling in virtual mode, all future runtime calls will be made in virtual
  // mode.
  EfiConvertPointer (0x0, (VOID **)&mRtcIndexRegister);
  EfiConvertPointer (0x0, (VOID **)&mRtcTargetRegister);
}

//
// MU_CHANGE begin
//

/**
    OnVariablePolicyProtocolNotification

    Sets the AdvancedLogger Locator variable policy.

    @param[in]      Event   - NULL if called from Entry, Event if called from notification
    @param[in]      Context - VariablePolicy if called from Entry, NULL if called from notification

  **/
STATIC
VOID
EFIAPI
OnVariablePolicyProtocolNotification (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy = NULL;
  EFI_STATUS                      Status;

  DEBUG ((DEBUG_INFO, "%a: Setting policy for RTC variables, Context=%p\n", __FUNCTION__, Context));

  if (Context != NULL) {
    VariablePolicy = (EDKII_VARIABLE_POLICY_PROTOCOL *)Context;
  } else {
    Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: - Locating Variable Policy failed - Code=%r\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR (Status);
      return;
    }
  }

  Status = RegisterBasicVariablePolicy (
             VariablePolicy,
             &gEfiCallerIdGuid,
             L"RTCALARM",
             sizeof (EFI_TIME),
             sizeof (EFI_TIME),
             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
             (UINT32) ~(EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE),
             VARIABLE_POLICY_TYPE_NO_LOCK
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: - Error setting policy for RTCALARM - Code=%r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  Status = RegisterBasicVariablePolicy (
             VariablePolicy,
             &gEfiCallerIdGuid,
             L"RTC",
             sizeof (UINT32),
             sizeof (UINT32),
             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
             (UINT32) ~(EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE),
             VARIABLE_POLICY_TYPE_NO_LOCK
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: - Error setting policy for RTC - Code=%r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  return;
}

//
// MU_CHANGE end
//

/**
  The user Entry Point for PcRTC module.

  This is the entry point for PcRTC module. It installs the UEFI runtime service
  including GetTime(),SetTime(),GetWakeupTime(),and SetWakeupTime().

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Others         Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializePcRtc (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_EVENT                       Event;
  VOID                            *ProtocolRegistration;    // MU_CHANGE
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy = NULL;   // MU_CHANGE

  EfiInitializeLock (&mModuleGlobal.RtcLock, TPL_CALLBACK);
  mModuleGlobal.CenturyRtcAddress = GetCenturyRtcAddress ();

  if (FeaturePcdGet (PcdRtcUseMmio)) {
    mRtcIndexRegister  = (UINTN)PcdGet64 (PcdRtcIndexRegister64);
    mRtcTargetRegister = (UINTN)PcdGet64 (PcdRtcTargetRegister64);
  } else {
    mRtcIndexRegister  = (UINTN)PcdGet8 (PcdRtcIndexRegister);
    mRtcTargetRegister = (UINTN)PcdGet8 (PcdRtcTargetRegister);
  }

  mRtcDefaultYear   = PcdGet16 (PcdRtcDefaultYear);
  mMinimalValidYear = PcdGet16 (PcdMinimalValidYear);
  mMaximalValidYear = PcdGet16 (PcdMaximalValidYear);

  Status = PcRtcInit (&mModuleGlobal);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PcRtcAcpiTableChangeCallback,
                  NULL,
                  &gEfiAcpi10TableGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PcRtcAcpiTableChangeCallback,
                  NULL,
                  &gEfiAcpiTableGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  gRT->GetTime       = PcRtcEfiGetTime;
  gRT->SetTime       = PcRtcEfiSetTime;
  gRT->GetWakeupTime = PcRtcEfiGetWakeupTime;
  gRT->SetWakeupTime = PcRtcEfiSetWakeupTime;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiRealTimeClockArchProtocolGuid,
                  NULL,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if (FeaturePcdGet (PcdRtcUseMmio)) {
    // Register for the virtual address change event
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    VirtualNotifyEvent,
                    NULL,
                    &gEfiEventVirtualAddressChangeGuid,
                    &mVirtualAddrChangeEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // MU_CHANGE begin
  //
  // There is no dependency for VariablePolicy Protocol in case this code is used
  // in firmware without VariablePolicy.  And, VariablePolicy may or may not be installed
  // before this driver is run.  If the Variable Policy Protocol is not found, register for
  // a notification that may not occur.

  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (EFI_ERROR (Status)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    OnVariablePolicyProtocolNotification,
                    NULL,
                    &Event
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to create notification callback event (%r)\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR (Status);
    } else {
      Status = gBS->RegisterProtocolNotify (
                      &gEdkiiVariablePolicyProtocolGuid,
                      Event,
                      &ProtocolRegistration
                      );

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: failed to register for notification (%r)\n", __FUNCTION__, Status));
        gBS->CloseEvent (Event);
        ASSERT_EFI_ERROR (Status);
      }
    }
  } else {
    OnVariablePolicyProtocolNotification (NULL, VariablePolicy);
  }

  //
  // MU_CHANGE end
  //

  return Status;
}
