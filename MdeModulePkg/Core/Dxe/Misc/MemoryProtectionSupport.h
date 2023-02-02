/** @file
  Functionality supporting enhanced memory protections

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEMORY_PROTECTION_SUPPORT_H_
#define MEMORY_PROTECTION_SUPPORT_H_

#include "DxeMain.h"
#include "Mem/HeapGuard.h"
#include <Library/ImagePropertiesRecordLib.h>
#include <Protocol/MemoryProtectionDebug.h>
#include <Protocol/MemoryProtectionSpecialRegionProtocol.h>

#define DO_NOT_PROTECT                 0x00000000
#define PROTECT_IF_ALIGNED_ELSE_ALLOW  0x00000001
#define PROTECT_ELSE_RAISE_ERROR       0x00000002

#define IMAGE_PROPERTIES_PRIVATE_DATA_SIGNATURE                SIGNATURE_32 ('I','P','P','D')
#define NONPROTECTED_IMAGE_PRIVATE_DATA_SIGNATURE              SIGNATURE_32 ('N','I','P','D')
#define MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE  SIGNATURE_32 ('M','P','S','R')

typedef struct {
  UINT32        Signature;
  UINTN         NonProtectedImageCount;
  LIST_ENTRY    NonProtectedImageList;
} NONPROTECTED_IMAGES_PRIVATE_DATA;

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

#define IsCodeType(a)  ((a == EfiLoaderCode) || (a == EfiBootServicesCode) || (a == EfiRuntimeServicesCode))
#define IsDataType(a)  ((a == EfiLoaderData) || (a == EfiBootServicesData) || (a == EfiRuntimeServicesData))

#define IS_BITMAP_INDEX_SET(Bitmap, Index)  ((((UINT8*)Bitmap)[Index / 8] & (1 << (Index % 8))) != 0 ? TRUE : FALSE)
#define SET_BITMAP_INDEX(Bitmap, Index)     (((UINT8*)Bitmap)[Index / 8] |= (1 << (Index % 8)))

#define NEXT_MEMORY_SPACE_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_GCD_MEMORY_SPACE_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) + (Size)))

#define POPULATE_IMAGE_RANGE_DESCRIPTOR(descriptor, type, base, length) \
          ((IMAGE_RANGE_DESCRIPTOR*) descriptor)->Signature = IMAGE_RANGE_DESCRIPTOR_SIGNATURE; \
          ((IMAGE_RANGE_DESCRIPTOR*) descriptor)->Type = type; \
          ((IMAGE_RANGE_DESCRIPTOR*) descriptor)->Base = base; \
          ((IMAGE_RANGE_DESCRIPTOR*) descriptor)->Length = length

#define POPULATE_MEMORY_DESCRIPTOR_ENTRY(Entry, Start, Pages, EfiType)                  \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->PhysicalStart  = (EFI_PHYSICAL_ADDRESS)Start;        \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->NumberOfPages  = (UINT64)Pages;                      \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->Attribute      = 0;                                  \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->Type           = (EFI_MEMORY_TYPE)EfiType;           \
  ((EFI_MEMORY_DESCRIPTOR*)Entry)->VirtualStart   = 0

#define ALIGN_ADDRESS(Address)  ((Address / EFI_PAGE_SIZE) * EFI_PAGE_SIZE)

#define SPECIAL_REGION_PATTERN  7732426 // 7-(S) 7-(P) 3-(E) 2-(C) 4-(I) 2-(A) 6-(L)

// TRUE if A and B have overlapping intervals
#define CHECK_OVERLAP(AStart, AEnd, BStart, BEnd) \
  ((AStart <= BStart && AEnd > BStart) || \
  (BStart <= AStart && BEnd > AStart))

// TRUE if A interval subsumes B interval
#define CHECK_SUBSUMPTION(AStart, AEnd, BStart, BEnd) \
  ((AStart < BStart) && (AEnd > BEnd))

// TRUE if A is a bitwise subset of B
#define CHECK_SUBSET(A, B)  ((A | B) == B)

typedef struct {
  UINT32        Signature;
  UINTN         ImageRecordCount;
  UINTN         CodeSegmentCountMax;
  LIST_ENTRY    ImageRecordList;
} IMAGE_PROPERTIES_PRIVATE_DATA;

typedef struct {
  UINT32                              Signature;
  MEMORY_PROTECTION_SPECIAL_REGION    SpecialRegion;
  LIST_ENTRY                          Link;
} MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY;

typedef struct {
  UINTN         Count;
  LIST_ENTRY    SpecialRegionList;
} MEMORY_PROTECTION_SPECIAL_REGION_PRIVATE_LIST_HEAD;

/**
  Clears the attributes from a memory range.

  @param  BaseAddress            The base address of the pages which need their attributes cleared
  @param  Length                 Length in bytes

  @retval EFI_SUCCESS            Attributes updated if necessary
  @retval EFI_INVALID_PARAMETER  BaseAddress is NULL or Length is zero
  @retval EFI_NOT_READY          Cpu Arch is not installed yet
  @retval Other                  Return value of CoreGetMemorySpaceDescriptor()

**/
EFI_STATUS
EFIAPI
ClearAccessAttributesFromMemoryRange (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 Length
  );

/**
  Fetches a pointer to the DXE memory protection settings HOB.
**/
DXE_MEMORY_PROTECTION_SETTINGS *
EFIAPI
GetDxeMemoryProtectionSettings (
  VOID
  );

/**
  Create an image properties record and insert it into the nonprotected image list

  @param[in]  ImageBase               Base of PE image
  @param[in]  ImageSize               Size of PE image

  @retval     EFI_INVALID_PARAMETER   ImageSize was zero
  @retval     EFI_OUT_OF_RESOURCES    Failure to Allocate()
  @retval     EFI_SUCCESS             The image properties record was successfully created and inserted
                                      into the nonprotected image list
**/
EFI_STATUS
CreateNonProtectedImagePropertiesRecord (
  IN    EFI_PHYSICAL_ADDRESS  ImageBase,
  IN    UINT64                ImageSize
  );

/**
  Inserts the input EntryToInsert into List by comparing UINT64 values at LIST_ENTRY + ComparisonOffset. If the input
  Signature is non-zero, a signature check based on each LIST_ENTRY + SignatureOffset will be performed.

  @param[in] List                       Pointer to the head of the list into which EntryToInsert will be inserted
  @param[in] EntryToInsert              Pointer to the list entry to insert into List
  @param[in] ComparisonOffset           Offset of the field to compare each list entry against relative to the
                                        list entry pointer
  @param[in] SignatureOffset            Offset of the signature to compare each list entry against relative to the
                                        list entry pointer
  @param[in] Signature                  Signature to compare for each list entry. If this is zero, no signature check
                                        will be performed

  @retval EFI_SUCCESS                   EntryToInsert was inserted into List
  @retval EFI_INVALID_PARAMETER         List or EntryToInsert were NULL, or a signature check failed
**/
EFI_STATUS
OrderedInsertUint64Comparison (
  IN LIST_ENTRY  *List,
  IN LIST_ENTRY  *EntryToInsert,
  IN INT64       ComparisonOffset,
  IN INT64       SignatureOffset OPTIONAL,
  IN UINT32      Signature OPTIONAL
  );

/**
  Remove execution permissions from all regions whose type is identified by
  the NX Protection Policy, set appropriate attributes to image memory
  based on the image protection policy, and set the stack guard.
**/
VOID
EFIAPI
InitializePageAttributesForMemoryProtectionPolicy (
  VOID
  );

#endif
