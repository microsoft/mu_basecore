/** @file
  Common private definitions used by the policy service modules.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _POLICY_HOB_H_
#define _POLICY_HOB_H_

typedef struct _POLICY_HOB_HEADER {
  EFI_GUID    PolicyGuid;
  UINT64      Attributes;
  UINT16      PolicySize;
  UINT16      AllocationSize;
  UINT16      Removed  : 1;
  UINT16      Reserved : 15;
  UINT16      NotifyDepth; // PEI Only
} POLICY_HOB_HEADER;

#define GET_HOB_POLICY_DATA(_hob_header)  ((VOID *)(((UINT8*)_hob_header) + sizeof(POLICY_HOB_HEADER)))

#endif
