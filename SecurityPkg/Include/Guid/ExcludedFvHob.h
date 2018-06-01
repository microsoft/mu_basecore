/** @file
  Defines the HOB GUID used to pass all excluded FVs to DXE Driver.
    
  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EXCLUDED_FV_HOB_H_
#define _EXCLUDED_FV_HOB_H_


extern EFI_GUID gExcludedFvHobGuid;

typedef struct {
  UINT32                     Num;
  EFI_PEI_FIRMWARE_VOLUME_INFO_MEASUREMENT_EXCLUDED_FV ExcludedFvs[0];
} EXCLUDED_HOB_DATA;

#endif
