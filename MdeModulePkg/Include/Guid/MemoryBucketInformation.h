/** @file

Guids for Pei storing and retrieving the PEI memory buckets
as well as storing bucket information in DXE.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEMORY_BUCKET_INFORMATION_H_
#define _MEMORY_BUCKET_INFORMATION_H_

#define MEMORY_BUCKET_INFORMATION_GUID \
{ 0x36138737, 0xb6db, 0x4ed6, { 0x9b, 0x4b, 0xf9, 0x61, 0x28, 0xe7, 0x19, 0x3c } }

extern EFI_GUID  gMemoryBucketInformationGuid;

#define MEMORY_TYPE_STATISTICS_GUID \
  { 0x6146C0D6, 0x8E30, 0x4DC2, { 0xA9, 0xCB, 0x5D, 0x85, 0x10, 0xC4, 0x8B, 0x39 }}

extern EFI_GUID  gMemoryTypeStatisticsGuid;

extern UINTN  NumberOfBuckets;

typedef struct {
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  EFI_PHYSICAL_ADDRESS    MaximumAddress;
  UINT64                  CurrentNumberOfPages;
  UINT64                  NumberOfPages;
  UINT32                  InformationIndex;
  BOOLEAN                 Special;
  BOOLEAN                 Runtime;
} EFI_MEMORY_TYPE_STATISTICS;

typedef struct {
  EFI_MEMORY_TYPE_STATISTICS    RuntimeBuckets[4];
  EFI_PHYSICAL_ADDRESS          CurrentTopInBucket[4];
  BOOLEAN                       MemoryBucketsDisabled;
} PEI_MEMORY_BUCKET_INFORMATION;

#endif
