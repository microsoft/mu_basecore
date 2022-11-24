/** @file
  Common private definitions used by the policy service modules.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _POLICY_COMMON_H_
#define _POLICY_COMMON_H_

#pragma pack(1)

typedef struct _POLICY_HOB_HEADER {
  EFI_GUID    PolicyGuid;
  UINT64      Attributes;
  UINT16      PolicySize;
  UINT16      AllocationSize;
  UINT16      Removed  : 1;
  UINT16      Reserved : 15;
  UINT16      NameSize;
  // CHAR16 Name[NameSize]
} POLICY_HOB_HEADER;

#pragma pack()

#define GET_HOB_POLICY_NAME(_hob_header)  \
  (_hob_header->NameSize == 0 ? NULL : (CHAR16 *)(((UINT8*)_hob_header) + sizeof(POLICY_HOB_HEADER)))

#define GET_HOB_POLICY_DATA(_hob_header)  \
  ((VOID *)(((UINT8*)_hob_header) + sizeof(POLICY_HOB_HEADER) + _hob_header->NameSize))

#define GET_POLICY_HOB_SIZE(_hob_header)  \
(sizeof(POLICY_HOB_HEADER) + _hob_header->NameSize + _hob_header->PolicySize)

/**
  Compares two strings considering NULL pointers equivalent.

  @param[in]  Name1     The first string pointer to be compared
  @param[in]  Name2     The second string pointer to be compared

  @retval   TRUE    The pointers are both NULL or identical.
  @retval   FALSE   One of the pointers is NULL or string are not identical.
**/
BOOLEAN
EFIAPI
PolicyCompareNames (
  CONST CHAR16  *Name1 OPTIONAL,
  CONST CHAR16  *Name2 OPTIONAL
  );

#endif
