/** @file
  IPMI Command - NetFnApp NULL instance library.

  Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <IndustryStandard/Ipmi.h>

/**
 * Send the Set Sensor Threshold command for a specified SensorNumber
 *
 * @param[in]   SetSensorThresholdRequestData   The filled out Set Sensor Threshold command structure
 * @param[out]  CompletionCode                  Pointer to a buffer for returning the completion code
 *
 * @retval  EFI_SUCCESS
 * @retval  EFI_INVALID_PARAMETER
 */
EFI_STATUS
EFIAPI
IpmiSetSensorThreshold (
  IN IPMI_SENSOR_SET_SENSOR_THRESHOLD_REQUEST_DATA  *SetSensorThresholdRequestData,
  OUT UINT8                                         *CompletionCode
  )
{
  return RETURN_UNSUPPORTED;
}

/**
 * Query the threshold data of the specified SensorNumber.
 *
 * @param[in]   SensorNumber                The unique identifier of the sensor being queried.
 * @param[out]  GetSensorThresholdResponse  Pointer to a buffer for returning the threshold response data.
 *
 * @retval  EFI_SUCCESS
 * @retval  EFI_INVALID_PARAMETER
 */
EFI_STATUS
EFIAPI
IpmiGetSensorThreshold (
  IN UINT8                                            SensorNumber,
  OUT IPMI_SENSOR_GET_SENSOR_THRESHOLD_RESPONSE_DATA  *GetSensorThresholdResponse
  )
{
  return RETURN_UNSUPPORTED;
}