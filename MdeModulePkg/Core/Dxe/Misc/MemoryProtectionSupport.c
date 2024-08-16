/** @file
  Functionality supporting the updated Project Mu memory protections

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/SafeIntLib.h>
#include "MemoryProtectionSupport.h"

IMAGE_PROPERTIES_PRIVATE_DATA  mImagePropertiesPrivate = {
  IMAGE_PROPERTIES_PRIVATE_DATA_SIGNATURE,
  0,
  0,
  INITIALIZE_LIST_HEAD_VARIABLE (mImagePropertiesPrivate.ImageRecordList)
};

NONPROTECTED_IMAGES_PRIVATE_DATA  mNonProtectedImageRangesPrivate = {
  NONPROTECTED_IMAGE_PRIVATE_DATA_SIGNATURE,
  0,
  INITIALIZE_LIST_HEAD_VARIABLE (mNonProtectedImageRangesPrivate.NonProtectedImageList)
};

MEMORY_PROTECTION_SPECIAL_REGION_PRIVATE_LIST_HEAD  mSpecialMemoryRegionsPrivate = {
  0,
  INITIALIZE_LIST_HEAD_VARIABLE (mSpecialMemoryRegionsPrivate.SpecialRegionList)
};

BOOLEAN                        mEnhancedMemoryProtectionActive = TRUE;
EFI_MEMORY_ATTRIBUTE_PROTOCOL  *mMemoryAttributeProtocol       = NULL;
UINT8                          *mBitmapGlobal                  = NULL;
LIST_ENTRY                     **mArrayOfListEntryPointers     = NULL;
BOOLEAN                        mGcdSyncComplete                = FALSE;

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

#define LEGACY_BIOS_WB_LENGTH  0xA0000

/**
  Return the section alignment requirement for the PE image section type.

  @param[in]  MemoryType  PE/COFF image memory type

  @retval     The required section alignment for this memory type

**/
UINT32
GetMemoryProtectionSectionAlignment (
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Get UEFI image protection policy based upon loaded image device path.

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol

  @return UEFI image protection policy
**/
UINT32
GetUefiImageProtectionPolicy (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath
  );

/**
  Set UEFI image protection attributes.

  @param[in]  ImageRecord    A UEFI image record
**/
VOID
SetUefiImageProtectionAttributes (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  );

/**
  Return the EFI memory permission attribute associated with memory
  type 'MemoryType' under the configured DXE memory protection policy.

  @param MemoryType       Memory type.
**/
UINT64
GetPermissionAttributeForMemoryType (
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Merge continous memory map entries whose have same attributes.

  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MemoryMapSize          A pointer to the size, in bytes, of the
                                 MemoryMap buffer. On input, this is the size of
                                 the current memory map.  On output,
                                 it is the size of new memory map after merge.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
VOID
MergeMemoryMap (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      *MemoryMapSize,
  IN UINTN                      DescriptorSize
  );

/**
  Sort memory map entries based upon PhysicalStart, from low to high.

  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MemoryMapSize          Size, in bytes, of the MemoryMap buffer.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
VOID
SortMemoryMap (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      MemoryMapSize,
  IN UINTN                      DescriptorSize
  );

/**
  Set UEFI image memory attributes.

  @param[in]  BaseAddress            Specified start address
  @param[in]  Length                 Specified length
  @param[in]  Attributes             Specified attributes
**/
VOID
SetUefiImageMemoryAttributes (
  IN UINT64  BaseAddress,
  IN UINT64  Length,
  IN UINT64  Attributes
  );

STATIC MEMORY_PROTECTION_DEBUG_PROTOCOL  mMemoryProtectionDebug =
{
  IsGuardPage,
  GetImageList
};

/**
Converts a number of pages to a size in bytes.

NOTE: Do not use EFI_PAGES_TO_SIZE because it handles UINTN only.

@param[in]  Pages     The number of EFI_PAGES.

@retval  The number of bytes associated with the input number of pages.
**/
STATIC
UINT64
EfiPagesToSize (
  IN UINT64  Pages
  )
{
  return LShiftU64 (Pages, EFI_PAGE_SHIFT);
}

/**
  Converts a size, in bytes, to a number of EFI_PAGESs.

  NOTE: Do not use EFI_SIZE_TO_PAGES because it handles UINTN only.

  @param[in]  Size      A size in bytes.

  @retval  The number of pages associated with the input number of bytes.

**/
STATIC
UINT64
EfiSizeToPages (
  IN UINT64  Size
  )
{
  return RShiftU64 (Size, EFI_PAGE_SHIFT) + ((((UINTN)Size) & EFI_PAGE_MASK) ? 1 : 0);
}

extern LIST_ENTRY  mGcdMemorySpaceMap;

// ---------------------------------------
//     LINKED LIST SUPPORT FUNCTIONS
// ---------------------------------------

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
STATIC
EFI_STATUS
OrderedInsertUint64Comparison (
  IN LIST_ENTRY  *List,
  IN LIST_ENTRY  *EntryToInsert,
  IN INT64       ComparisonOffset,
  IN INT64       SignatureOffset OPTIONAL,
  IN UINT32      Signature OPTIONAL
  )
{
  LIST_ENTRY  *ListLink;
  LIST_ENTRY  *ListEndLink;
  UINT64      EntryToInsertVal;
  UINT64      ListEntryVal;

  if ((List == NULL) || (EntryToInsert == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Signature != 0) && (*((UINT32 *)((UINT8 *)EntryToInsert + SignatureOffset)) != Signature)) {
    ASSERT (*((UINT32 *)((UINT8 *)EntryToInsert + SignatureOffset)) == Signature);
    return EFI_INVALID_PARAMETER;
  }

  EntryToInsertVal = *((UINT64 *)((UINT8 *)EntryToInsert + ComparisonOffset));

  ListLink    = List->ForwardLink;
  ListEndLink = List;
  while (ListLink != ListEndLink) {
    if ((Signature != 0) && (*((UINT32 *)((UINT8 *)ListLink + SignatureOffset)) != Signature)) {
      ASSERT (*((UINT32 *)((UINT8 *)ListLink + SignatureOffset)) == Signature);
      return EFI_INVALID_PARAMETER;
    }

    ListEntryVal = *((UINT64 *)((UINT8 *)ListLink + ComparisonOffset));

    if (EntryToInsertVal < ListEntryVal) {
      break;
    }

    ListLink = ListLink->ForwardLink;
  }

  EntryToInsert->BackLink              = ListLink->BackLink;
  EntryToInsert->ForwardLink           = ListLink;
  EntryToInsert->BackLink->ForwardLink = EntryToInsert;
  EntryToInsert->ForwardLink->BackLink = EntryToInsert;
  return EFI_SUCCESS;
}

/**
  Merges every LIST_ENTRY within ArrayOfListEntriesToBeMerged into List

  @param[in] List                             Pointer to the head of the list into which each element
                                              of ArrayOfListEntriesToBeMerged will be inserted
  @param[in] ArrayOfListEntriesToBeMerged     Pointer to an array of LIST_ENTRY* which will be merged
                                              into the input List
  @param[in] ListToBeMergedCount              Number of LIST_ENTRY* which will be merged
                                              into the input List
  @param[in] ComparisonOffset                 Offset of the field to compare each list entry against relative to the
                                              list entry pointer
  @param[in] SignatureOffset                  Offset of the signature to compare each list entry against relative to the
                                              list entry pointer
  @param[in] Signature                        Signature to compare for each list entry. If this is zero,
                                              no signature check will be performed

  @retval EFI_SUCCESS                         ArrayOfListEntriesToBeMerged was successfully merged into List
  @retval EFI_INVALID_PARAMETER               List was NULL                             OR
                                              ArrayOfListEntriesToBeMerged was NULL     OR
                                              ArrayOfListEntriesToBeMerged[n] was NULL  OR
                                              ListToBeMergedCount was zero
  @retval Other                               Return value of OrderedInsertUint64Comparison()
**/
STATIC
EFI_STATUS
OrderedInsertArrayUint64Comparison (
  IN  LIST_ENTRY  *List,
  IN  LIST_ENTRY  **ArrayOfListEntriesToBeMerged,
  IN  UINTN       ListToBeMergedCount,
  IN  INT64       ComparisonOffset,
  IN  INT64       SignatureOffset OPTIONAL,
  IN  UINT32      Signature OPTIONAL
  )
{
  INTN        ListToBeMergedIndex = ListToBeMergedCount - 1;
  EFI_STATUS  Status              = EFI_SUCCESS;

  if ((List == NULL) || (ArrayOfListEntriesToBeMerged == NULL) || (ListToBeMergedCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  // If the input array is sorted, going backwards is the fastest method
  for ( ; ListToBeMergedIndex >= 0; --ListToBeMergedIndex) {
    if (ArrayOfListEntriesToBeMerged[ListToBeMergedIndex] == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    RemoveEntryList (ArrayOfListEntriesToBeMerged[ListToBeMergedIndex]);
    Status = OrderedInsertUint64Comparison (
               List,
               ArrayOfListEntriesToBeMerged[ListToBeMergedIndex],
               ComparisonOffset,
               SignatureOffset,
               Signature
               );

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  return Status;
}

/**
  Merges every LIST_ENTRY within ListToBeMerged into ListToMergeInto

  @param[in]  ListToMergeInto                 Pointer to the head of a list into which the input
                                              ListToBeMerged will be merged
  @param[in]  ListToBeMerged                  Pointer to the head of a list which will be merged
                                              into ListToMergeInto
  @param[in]  ListToBeMergedCount             Number of LIST_ENTRY* in ListToBeMerged
  @param[out] ArrayOfMergedElements           Pointer to an unallocated array of LIST_ENTRY*. The array will be
                                              allocated if the function returns success and contain every
                                              LIST_ENTRY* merged into ListToMergeInto
  @param[in] ComparisonOffset                 Offset of the field to compare each list entry against relative to the
                                              list entry pointer
  @param[in] SignatureOffset                  Offset of the signature to compare each list entry against relative to the
                                              list entry pointer
  @param[in] Signature                        Signature to compare for each list entry. If this is zero,
                                              no signature check will be performed

  @retval EFI_SUCCESS                         ArrayOfListEntriesToBeMerged was successfully merged into
                                              ImagePropertiesRecordList
  @retval EFI_OUT_OF_RESOURCES                Failed to allocate memory
  @retval EFI_INVALID_PARAMETER               ListToMergeInto was NULL                    OR
                                              ListToBeMerged was NULL                     OR
                                              ArrayOfListEntriesToBeMerged was NULL       OR
                                              *ArrayOfListEntriesToBeMerged was not NULL  OR
                                              ListToBeMergedCount was NULL
  @retval other                               Return value of OrderedInsertUint64Comparison()
**/
STATIC
EFI_STATUS
MergeListsUint64Comparison (
  IN  LIST_ENTRY   *ListToMergeInto,
  IN  LIST_ENTRY   *ListToBeMerged,
  IN  CONST UINTN  *ListToBeMergedCount,
  OUT LIST_ENTRY   ***ArrayOfMergedElements,
  IN  INT64        ComparisonOffset,
  IN  INT64        SignatureOffset OPTIONAL,
  IN  UINT32       Signature OPTIONAL
  )
{
  UINTN       Index  = 0;
  EFI_STATUS  Status = EFI_SUCCESS;

  if ((ListToMergeInto == NULL) || (ListToBeMerged == NULL) ||
      (ArrayOfMergedElements == NULL) || (*ArrayOfMergedElements != NULL) ||
      (ListToBeMergedCount == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *ArrayOfMergedElements = AllocateZeroPool (sizeof (LIST_ENTRY *) * *ListToBeMergedCount);

  if (*ArrayOfMergedElements == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Insert each entry in the list to be merged into the
  while (!IsListEmpty (ListToBeMerged) && Index < *ListToBeMergedCount) {
    (*ArrayOfMergedElements)[Index] = ListToBeMerged->ForwardLink;
    RemoveEntryList ((*ArrayOfMergedElements)[Index]);
    Status = OrderedInsertUint64Comparison (
               ListToMergeInto,
               (*ArrayOfMergedElements)[Index++],
               ComparisonOffset,
               SignatureOffset,
               Signature
               );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  // If we did not merge all elements of the list, unmerge them and free the input array
  if (!IsListEmpty (ListToBeMerged)) {
    OrderedInsertArrayUint64Comparison (
      ListToBeMerged,
      *ArrayOfMergedElements,
      Index - 1,
      ComparisonOffset,
      SignatureOffset,
      Signature
      );
    FreePool (*ArrayOfMergedElements);
  }

  return Status;
}

// ---------------------------------------
//         SPECIAL REGION LOGIC
// ---------------------------------------

/**
  Walk through the input special region list to combine overlapping intervals.

  @param[in]  SpecialRegionList      Pointer to the head of the list

  @retval     EFI_INVALID_PARAMTER  SpecialRegionList was NULL
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate memory
  @retval     EFI_SUCCESS           Input special region list was merged
**/
STATIC
EFI_STATUS
MergeOverlappingSpecialRegions (
  IN LIST_ENTRY  *SpecialRegionList
  )
{
  LIST_ENTRY                                   *BackLink, *ForwardLink;
  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY  *BackEntry, *ForwardEntry, *NewEntry;
  EFI_PHYSICAL_ADDRESS                         BackStart, BackEnd, ForwardStart, ForwardEnd;

  if (SpecialRegionList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BackLink = SpecialRegionList->ForwardLink;

  while (BackLink != SpecialRegionList) {
    BackEntry = CR (
                  BackLink,
                  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY,
                  Link,
                  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
                  );

    BackStart = BackEntry->SpecialRegion.Start;
    BackEnd   = BackEntry->SpecialRegion.Start + BackEntry->SpecialRegion.Length;

    ForwardLink = BackLink->ForwardLink;
    while (ForwardLink != SpecialRegionList) {
      ForwardEntry = CR (
                       ForwardLink,
                       MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY,
                       Link,
                       MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
                       );

      ForwardStart = ForwardEntry->SpecialRegion.Start;
      ForwardEnd   = ForwardEntry->SpecialRegion.Start + ForwardEntry->SpecialRegion.Length;

      // If BackEntry and ForwardEntry overlap
      if (CHECK_OVERLAP (BackStart, BackEnd, ForwardStart, ForwardEnd)) {
        // If the attributes are the same between both entries, just expand BackEntry and delete ForwardEntry
        if (BackEntry->SpecialRegion.EfiAttributes == ForwardEntry->SpecialRegion.EfiAttributes) {
          BackEntry->SpecialRegion.Length = ForwardEnd - BackStart;
          ForwardLink                     = ForwardLink->BackLink;
          RemoveEntryList (ForwardLink->ForwardLink);
          FreePool (ForwardEntry);
        }
        // If BackEntry subsumes ForwardEntry, we need to create a new list entry to split BackEntry
        else if (CHECK_SUBSUMPTION (BackStart, BackEnd, ForwardStart, ForwardEnd)) {
          NewEntry = AllocatePool (sizeof (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY));

          if (NewEntry == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }

          BackEntry->SpecialRegion.Length = ForwardStart - BackStart;
          InitializeListHead (&NewEntry->Link);
          NewEntry->Signature                   = MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE;
          NewEntry->SpecialRegion.EfiAttributes = BackEntry->SpecialRegion.EfiAttributes;
          NewEntry->SpecialRegion.Start         = ForwardEnd;
          NewEntry->SpecialRegion.Length        = BackEnd - ForwardEnd;
          OrderedInsertUint64Comparison (
            SpecialRegionList,
            &NewEntry->Link,
            OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, SpecialRegion) + OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION, Start) - OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, Link),
            OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, Signature) - OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, Link),
            MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
            );

          ForwardLink = &NewEntry->Link;
          break;
        }
        // If BackEntry does not subsume FrontEntry, we can trim each entry
        //
        // If ForwardEntry has more strict attributes, trim BackEntry
        else if (CHECK_SUBSET (BackEntry->SpecialRegion.EfiAttributes, ForwardEntry->SpecialRegion.EfiAttributes)) {
          BackEntry->SpecialRegion.Length = ForwardStart - BackStart;
        }
        // If BackEntry has more strict attributes, trim ForwardEntry
        else {
          ForwardEntry->SpecialRegion.Start = BackEnd;
        }
      } else {
        // No overlap
        break;
      }

      ForwardLink = ForwardLink->ForwardLink;
    }

    BackLink    = ForwardLink;
    ForwardLink = BackLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

/**
  Check if the input SpecialRegion conflicts with any special regions in the input SpecialRegionList. The SpecialRegion
  conflicts if the interval overlaps with another interval in SpecialRegionList AND the attributes of
  SpecialRegion are not a subset of the attributes of the region with which it overlaps.

  @param[in]  SpecialRegion       The special region to check against all special regions in SpecialRegionList
  @param[in]  SpecialRegionList   The list of special regions to check against the input SpecialRegion

  @retval     TRUE                SpecialRegion conflicts with a special region in SpecialRegionList
  @retval     FALSE               SpecialRegion does not conflict with a special region in SpecialRegionList
**/
STATIC
BOOLEAN
DoesSpecialRegionConflict (
  IN MEMORY_PROTECTION_SPECIAL_REGION  *SpecialRegion,
  IN LIST_ENTRY                        *SpecialRegionList
  )
{
  LIST_ENTRY                                   *SpecialRegionListLink;
  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY  *SpecialRegionListEntry;
  BOOLEAN                                      OverlapCheck, SubsetCheck;
  EFI_PHYSICAL_ADDRESS                         InputStart, InputEnd, ListEntryStart, ListEntryEnd;

  InputStart = SpecialRegion->Start;
  InputEnd   = SpecialRegion->Start + SpecialRegion->Length;

  SpecialRegionListLink = SpecialRegionList->ForwardLink;

  while (SpecialRegionListLink != SpecialRegionList) {
    SpecialRegionListEntry = CR (
                               SpecialRegionListLink,
                               MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY,
                               Link,
                               MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
                               );

    ListEntryStart = SpecialRegionListEntry->SpecialRegion.Start;
    ListEntryEnd   = SpecialRegionListEntry->SpecialRegion.Start + SpecialRegionListEntry->SpecialRegion.Length;

    OverlapCheck = CHECK_OVERLAP (InputStart, InputEnd, ListEntryStart, ListEntryEnd);
    SubsetCheck  = CHECK_SUBSET (SpecialRegion->EfiAttributes, SpecialRegionListEntry->SpecialRegion.EfiAttributes);

    if (OverlapCheck && !SubsetCheck) {
      return TRUE;
    }

    SpecialRegionListLink = SpecialRegionListLink->ForwardLink;
  }

  return FALSE;
}

/**
  Copy the HOB MEMORY_PROTECTION_SPECIAL_REGION entries into a local list

  @retval EFI_SUCCESS           HOB Entries successfully copied
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate
**/
STATIC
EFI_STATUS
CollectSpecialRegionHobs (
  VOID
  )
{
  EFI_HOB_GUID_TYPE                            *GuidHob          = NULL;
  MEMORY_PROTECTION_SPECIAL_REGION             *HobSpecialRegion = NULL;
  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY  *NewSpecialRegion = NULL;

  GuidHob = GetFirstGuidHob (&gMemoryProtectionSpecialRegionHobGuid);

  while (GuidHob != NULL) {
    HobSpecialRegion = (MEMORY_PROTECTION_SPECIAL_REGION *)GET_GUID_HOB_DATA (GuidHob);
    if (DoesSpecialRegionConflict (HobSpecialRegion, &mSpecialMemoryRegionsPrivate.SpecialRegionList)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - Special region 0x%llx - 0x%llx conflicts with another special region!\n",
        __FUNCTION__,
        HobSpecialRegion->Start,
        HobSpecialRegion->Start + HobSpecialRegion->Length
        ));
      ASSERT (FALSE);
    } else {
      NewSpecialRegion = AllocatePool (sizeof (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY));

      if (NewSpecialRegion == NULL) {
        DEBUG ((
          DEBUG_ERROR,
          "%a - Failed to allocate a special region list entry!\n",
          __FUNCTION__
          ));
        return EFI_OUT_OF_RESOURCES;
      }

      NewSpecialRegion->SpecialRegion.Start         = ALIGN_ADDRESS (HobSpecialRegion->Start);
      NewSpecialRegion->SpecialRegion.Length        = ALIGN_VALUE (HobSpecialRegion->Length, EFI_PAGE_SIZE);
      NewSpecialRegion->SpecialRegion.EfiAttributes = HobSpecialRegion->EfiAttributes & EFI_MEMORY_ACCESS_MASK;
      NewSpecialRegion->Signature                   = MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE;
      OrderedInsertUint64Comparison (
        &mSpecialMemoryRegionsPrivate.SpecialRegionList,
        &NewSpecialRegion->Link,
        OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, SpecialRegion) + OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION, Start) - OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, Link),
        OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, Signature) - OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, Link),
        MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
        );
      mSpecialMemoryRegionsPrivate.Count++;
    }

    GuidHob = GetNextGuidHob (&gMemoryProtectionSpecialRegionHobGuid, GET_NEXT_HOB (GuidHob));
  }

  MergeOverlappingSpecialRegions (&mSpecialMemoryRegionsPrivate.SpecialRegionList);

  return EFI_SUCCESS;
}

/**
  Create a sorted array of MEMORY_PROTECTION_SPECIAL_REGION structs describing
  all memory protection special regions. This memory should be freed by the caller but
  will not be allocated if there are no special regions.

  @param[out] SpecialRegions  Pointer to unallocated MEMORY_PROTECTION_SPECIAL_REGION array
  @param[out] Count           Number of MEMORY_PROTECTION_SPECIAL_REGION structs in the
                              allocated array

  @retval EFI_SUCCESS           Array successfuly created
  @retval EFI_INVALID_PARAMTER  SpecialRegions is NULL or *SpecialRegions is not NULL
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory

**/
EFI_STATUS
EFIAPI
GetSpecialRegions (
  OUT MEMORY_PROTECTION_SPECIAL_REGION  **SpecialRegions,
  OUT UINTN                             *Count
  )
{
  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY  *SpecialRegionEntry;
  LIST_ENTRY                                   *SpecialRegionEntryLink;
  UINTN                                        Index = 0;

  if ((SpecialRegions == NULL) || (*SpecialRegions != NULL) || (Count == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mSpecialMemoryRegionsPrivate.Count == 0) {
    *Count = 0;
    return EFI_SUCCESS;
  }

  *SpecialRegions = AllocatePool (sizeof (MEMORY_PROTECTION_SPECIAL_REGION) * mSpecialMemoryRegionsPrivate.Count);

  if (*SpecialRegions == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (SpecialRegionEntryLink = mSpecialMemoryRegionsPrivate.SpecialRegionList.ForwardLink;
       SpecialRegionEntryLink != &mSpecialMemoryRegionsPrivate.SpecialRegionList;
       SpecialRegionEntryLink = SpecialRegionEntryLink->ForwardLink)
  {
    SpecialRegionEntry = CR (
                           SpecialRegionEntryLink,
                           MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY,
                           Link,
                           MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
                           );

    CopyMem (
      &((*SpecialRegions)[Index++]),
      &SpecialRegionEntry->SpecialRegion,
      sizeof (MEMORY_PROTECTION_SPECIAL_REGION)
      );
  }

  // Fix the private count if it has gotten out of sync somehow
  if (mSpecialMemoryRegionsPrivate.Count != Index) {
    mSpecialMemoryRegionsPrivate.Count = Index;
  }

  *Count = mSpecialMemoryRegionsPrivate.Count;

  return EFI_SUCCESS;
}

/**
  Add the input MEMORY_PROTECTION_SPECIAL_REGION to the internal list of memory protection special regions
  which will have the specified attributes applied when memory protections are initialized or, if memory
  protection initialization has already occurred, will be used by test drivers to ensure the
  region has the specified attributes.

  @param[in]  Start               Start of region which will be added to the special memory regions.
                                  NOTE: This address will be page-aligned
  @param[in]  Length              Length of the region which will be added to the special memory regions
                                  NOTE: This value will be page-aligned
  @param[in]  Attributes          Attributes to apply to the region during memory protection initialization
                                  and which should be active if the region was reported after initialization.

  @retval   EFI_SUCCESS           SpecialRegion was successfully added
  @retval   EFI_INVALID_PARAMTER  Length is zero or the input region overlaps with an
                                  existing special region
  @retval   EFI_OUT_OF_RESOURCES  Failed to allocate memory
**/
EFI_STATUS
EFIAPI
AddSpecialRegion (
  IN EFI_PHYSICAL_ADDRESS  Start,
  IN UINT64                Length,
  IN UINT64                Attributes
  )
{
  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY  *SpecialRegionEntry = NULL;

  if (Length == 0) {
    return EFI_INVALID_PARAMETER;
  }

  SpecialRegionEntry = AllocatePool (sizeof (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY));

  if (SpecialRegionEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SpecialRegionEntry->Signature                   = MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE;
  SpecialRegionEntry->SpecialRegion.Start         = ALIGN_ADDRESS (Start);
  SpecialRegionEntry->SpecialRegion.Length        = ALIGN_VALUE (Length, EFI_PAGE_SIZE);
  SpecialRegionEntry->SpecialRegion.EfiAttributes = Attributes & EFI_MEMORY_ACCESS_MASK;

  if (DoesSpecialRegionConflict (&SpecialRegionEntry->SpecialRegion, &mSpecialMemoryRegionsPrivate.SpecialRegionList)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a - Special region 0x%llx - 0x%llx conflicts with an existing special region!\n",
      __FUNCTION__,
      SpecialRegionEntry->SpecialRegion.Start,
      SpecialRegionEntry->SpecialRegion.Start + SpecialRegionEntry->SpecialRegion.Length
      ));
    ASSERT (FALSE);
    FreePool (SpecialRegionEntry);
    return EFI_INVALID_PARAMETER;
  }

  OrderedInsertUint64Comparison (
    &mSpecialMemoryRegionsPrivate.SpecialRegionList,
    &SpecialRegionEntry->Link,
    OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, SpecialRegion) + OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION, Start) - OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, Link),
    OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, Signature) - OFFSET_OF (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY, Link),
    MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
    );
  mSpecialMemoryRegionsPrivate.Count++;
  MergeOverlappingSpecialRegions (&mSpecialMemoryRegionsPrivate.SpecialRegionList);

  return EFI_SUCCESS;
}

STATIC MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL  mMemoryProtectionSpecialRegion =
{
  GetSpecialRegions,
  AddSpecialRegion
};

/**
  Initialize the memory protection special region reporting.
**/
VOID
EFIAPI
CoreInitializeMemoryProtectionSpecialRegions (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  HandleNull = NULL;

  Status = CollectSpecialRegionHobs ();

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a - Failed to collect the memory protection special region HOB entries!\n",
      __FUNCTION__
      ));
  }

  Status = CoreInstallMultipleProtocolInterfaces (
             &HandleNull,
             &gMemoryProtectionSpecialRegionProtocolGuid,
             &mMemoryProtectionSpecialRegion,
             NULL
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a - Failed to install the memory protection special region protocol!\n",
      __FUNCTION__
      ));
  }

  return;
}

// ---------------------------------------
//       USEFUL DEBUG FUNCTIONS
// ---------------------------------------

/**
  Debug dumps the memory map.

  @param[in]  MemoryMapSize     A pointer to the size, in bytes, of the MemoryMap buffer
  @param[in]  MemoryMap         A pointer to the buffer containing the memory map
  @param[in]  DescriptorSize    Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR
**/
STATIC
VOID
DumpMemoryMap (
  IN CONST UINTN             *MemoryMapSize,
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN CONST UINTN             *DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;

  if ((MemoryMapSize == NULL) || (MemoryMap == NULL) || (DescriptorSize == NULL)) {
    return;
  }

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);
  while (MemoryMapEntry < MemoryMapEnd) {
    DEBUG ((
      DEBUG_INFO,
      "Memory Range: 0x%llx - 0x%llx. Type:%d, Attributes: 0x%llx\n",
      MemoryMapEntry->PhysicalStart,
      MemoryMapEntry->PhysicalStart + EfiPagesToSize (MemoryMapEntry->NumberOfPages),
      MemoryMapEntry->Type,
      MemoryMapEntry->Attribute
      ));
    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
  }
}

/**
  Debug dumps the input bitmap

  @param[in] Bitmap Pointer to the start of the bitmap
  @param[in] Count  Number of bitmap entries
**/
STATIC
VOID
DumpBitmap (
  IN CONST UINT8  *Bitmap,
  IN UINTN        Count
  )
{
  UINTN  Index = 0;

  DEBUG ((DEBUG_INFO, "Bitmap: "));
  for ( ; Index < Count; Index++) {
    DEBUG ((DEBUG_INFO, "%d", IS_BITMAP_INDEX_SET (Bitmap, Index) ? 1 : 0));
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

/**
  Debug dumps the input bitmap

  @param[in] Bitmap Pointer to the start of the bitmap
  @param[in] Count  Number of bitmap entries
**/
STATIC
VOID
DumpMemoryProtectionSpecialRegions (
  VOID
  )
{
  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY  *SpecialRegionEntry;
  LIST_ENTRY                                   *SpecialRegionEntryLink;

  for (SpecialRegionEntryLink = mSpecialMemoryRegionsPrivate.SpecialRegionList.ForwardLink;
       SpecialRegionEntryLink != &mSpecialMemoryRegionsPrivate.SpecialRegionList;
       SpecialRegionEntryLink = SpecialRegionEntryLink->ForwardLink)
  {
    SpecialRegionEntry = CR (
                           SpecialRegionEntryLink,
                           MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY,
                           Link,
                           MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
                           );

    DEBUG ((
      DEBUG_INFO,
      "Memory Protection Special Region: 0x%llx - 0x%llx. Attributes: 0x%llx\n",
      SpecialRegionEntry->SpecialRegion.Start,
      SpecialRegionEntry->SpecialRegion.Start + SpecialRegionEntry->SpecialRegion.Length,
      SpecialRegionEntry->SpecialRegion.EfiAttributes
      ));
  }

  return;
}

// ---------------------------------------
//        GCD MEMORY MAP FUNCTIONS
// ---------------------------------------

/**
  Sort the GCD memory map entries from low to high.

  @param[in, out]   MemoryMap       A pointer to the buffer containing the current memory map
  @param[in]        MemoryMapSize   Size, in bytes, of the MemoryMap buffer
  @param[in]        DescriptorSize  Size, in bytes, of an individual EFI_GCD_MEMORY_SPACE_DESCRIPTOR
**/
STATIC
VOID
SortMemorySpaceMap (
  IN OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemoryMap,
  IN CONST UINTN                          *MemoryMapSize,
  IN CONST UINTN                          *DescriptorSize
  )
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemoryMapEntry;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *NextMemoryMapEntry;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemoryMapEnd;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  TempMemoryMap;

  if ((MemoryMap == NULL) || (MemoryMapSize == NULL) || (DescriptorSize == NULL)) {
    return;
  }

  MemoryMapEntry     = MemoryMap;
  NextMemoryMapEntry = NEXT_MEMORY_SPACE_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
  MemoryMapEnd       = (EFI_GCD_MEMORY_SPACE_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);
  while (MemoryMapEntry < MemoryMapEnd) {
    while (NextMemoryMapEntry < MemoryMapEnd) {
      if (MemoryMapEntry->BaseAddress > NextMemoryMapEntry->BaseAddress) {
        CopyMem (&TempMemoryMap, MemoryMapEntry, *DescriptorSize);
        CopyMem (MemoryMapEntry, NextMemoryMapEntry, *DescriptorSize);
        CopyMem (NextMemoryMapEntry, &TempMemoryMap, *DescriptorSize);
      }

      NextMemoryMapEntry = NEXT_MEMORY_SPACE_DESCRIPTOR (NextMemoryMapEntry, *DescriptorSize);
    }

    MemoryMapEntry     = NEXT_MEMORY_SPACE_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
    NextMemoryMapEntry = NEXT_MEMORY_SPACE_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
  }

  return;
}

/**
  Convert a GCD memory type to an EFI memory type

  @param[in]  GcdType   The GCD memory type to convert

  @retval               The converted EFI memory type
**/
STATIC
EFI_MEMORY_TYPE
GcdTypeToEfiType (
  IN EFI_GCD_MEMORY_TYPE  *GcdType
  )
{
  if (GcdType == NULL) {
    return EfiConventionalMemory;
  }

  switch (*GcdType) {
    case EfiGcdMemoryTypeMemoryMappedIo:
      return EfiMemoryMappedIO;
    case EfiGcdMemoryTypePersistentMemory:
      return EfiPersistentMemory;
    case EfiGcdMemoryTypeReserved:
      return EfiReservedMemoryType;
    default:
      return EfiConventionalMemory;
  }
}

/**
  Find GCD memory type for the input region. If one GCD type does not cover the entire region, return the remaining
  region which are covered by one or more subsequent GCD descriptors.

  @param[in]  MemorySpaceMap        A SORTED array of GCD memory descrptors
  @param[in]  NumberOfDescriptors   The number of descriptors in the GCD descriptor array
  @param[in]  PhysicalStart         Page-aligned starting address to check against GCD descriptors
  @param[in]  Length                Length of the region being checked
  @param[out] Type                  The GCD memory type which applies to
                                    PhyscialStart + NumberOfPages - <remaining uncovered pages>

  @retval Remaining region length not covered by the found GCD Memory region
**/
STATIC
UINT64
GetOverlappingMemorySpaceRegion (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap,
  IN CONST UINTN                      *NumberOfDescriptors,
  IN CONST EFI_PHYSICAL_ADDRESS       *PhysicalStart,
  IN CONST UINT64                     *Length,
  OUT EFI_GCD_MEMORY_TYPE             *Type
  )
{
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  AlignedPhysicalStart, AlignedLength, PhysicalEnd, MapEntryStart, MapEntryEnd;

  if ((MemorySpaceMap == NULL) || (Type == NULL) ||
      (Length == NULL) || (NumberOfDescriptors == NULL) ||
      (PhysicalStart == NULL))
  {
    return 0;
  }

  // Ensure the PhysicalStart is page aligned
  ASSERT ((*PhysicalStart & EFI_PAGE_MASK) == 0);
  ASSERT ((*Length & EFI_PAGE_MASK) == 0);
  AlignedPhysicalStart = ALIGN_VALUE (*PhysicalStart, EFI_PAGE_SIZE);
  AlignedLength        = ALIGN_VALUE (*Length, EFI_PAGE_SIZE);
  PhysicalEnd          = AlignedPhysicalStart + AlignedLength;

  // Go through each memory space map entry
  for (Index = 0; Index < *NumberOfDescriptors; Index++) {
    MapEntryStart = MemorySpaceMap[Index].BaseAddress;
    MapEntryEnd   = MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length;

    MapEntryStart = ALIGN_VALUE (MapEntryStart, EFI_PAGE_SIZE);
    MapEntryEnd   = ALIGN_VALUE (MapEntryEnd, EFI_PAGE_SIZE);

    // Check if the memory map entry contains the physical start
    if ((MapEntryStart <= AlignedPhysicalStart) && (MapEntryEnd > AlignedPhysicalStart)) {
      *Type = MemorySpaceMap[Index].GcdMemoryType;
      // Check if the memory map entry contains the entire physical region
      if (MapEntryEnd >= PhysicalEnd) {
        return 0;
      } else {
        // Return remaining region
        return PhysicalEnd - MapEntryEnd;
      }
    }
  }

  *Type = EfiGcdMemoryTypeNonExistent;
  return 0;
}

/**
  Updates the memory map to contain contiguous entries from StartOfAddressSpace to
  max(EndOfAddressSpace, address + length of the final memory map entry). If DetermineSize
  is TRUE, then this function will just determine the required buffer size for the output
  memory map.

  NOTE: Gaps in the input EFI memory map which correlate with the non-existent GCD type
        will not be added to the map.

  @param[in, out] MemoryMapSize                   IN: Size, in bytes, of MemoryMap.
                                                  OUT: Size, in bytes, of the filled in memory map or the size
                                                       required to fill in the memory map if DetermineSize is TRUE
  @param[in, out] MemoryMap                       IN:  Pointer to the EFI memory map
                                                  OUT: A sorted memory map describing the entire address range
                                                  described in MemorySpaceMap
  @param[in]      MemoryMapBufferSize             Size, in bytes, of the full buffer pointed to by MemoryMap
  @param[in]      InsertionPoint                  Pointer to the pointer where new memory map entries should
                                                  be inserted. This insertion point should be between MemoryMap
                                                  and MemoryMap + MemoryMapBufferSize. If this is NULL, then
                                                  DetermineSize must be TRUE.
  @param[in]      DescriptorSize                  Size, in bytes, of each descriptor region in the array
  @param[in]      MemorySpaceMapDescriptorCount   Number of entries in the MemorySpaceMap array
  @param[in]      MemorySpaceMap                  The system memory space map (GCD memory map)
  @param[in]      MemorySpaceMapDescriptorSize    Size, in bytes, of each descriptor region in the memory space
                                                  map array
  @param[in]      DetermineSize                   If TRUE, then this function will only determine the required
                                                  buffer size for the output memory map. If FALSE, then this
                                                  function will fill in the memory map

  @retval EFI_SUCCESS                   Successfully filled in the memory map
  @retval EFI_INVALID_PARAMETER         An input parameter was invalid.
**/
EFI_STATUS
FillInMemoryMap (
  IN OUT    UINTN                            *MemoryMapSize,
  IN OUT    EFI_MEMORY_DESCRIPTOR            *MemoryMap,
  IN        UINTN                            MemoryMapBufferSize,
  IN        EFI_MEMORY_DESCRIPTOR            *InsertionPoint,
  IN CONST  UINTN                            *DescriptorSize,
  IN CONST  UINTN                            *MemorySpaceMapDescriptorCount,
  IN        EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap,
  IN CONST  UINTN                            *MemorySpaceMapDescriptorSize,
  IN OUT    BOOLEAN                          DetermineSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapCurrent, *MemoryMapEnd;
  EFI_PHYSICAL_ADDRESS   LastEntryEnd, NextEntryStart, StartOfAddressSpace, EndOfAddressSpace;
  EFI_GCD_MEMORY_TYPE    GcdType = 0;
  UINT64                 RemainingLength, OverlapLength;
  UINTN                  AdditionalEntriesCount;

  if ((MemoryMap == NULL) ||
      (MemoryMapSize == NULL) || (MemorySpaceMap == NULL) ||
      (MemorySpaceMapDescriptorSize == NULL) || (MemorySpaceMapDescriptorCount == NULL) ||
      (!DetermineSize && (InsertionPoint == NULL)))
  {
    DEBUG ((DEBUG_ERROR, "%a - Function had NULL input(s)!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((*MemoryMapSize == 0) || (*DescriptorSize == 0)) {
    DEBUG ((DEBUG_ERROR, "%a - MemoryMapSize or DescriptorSize is zero!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((!DetermineSize) &&
      !(((UINTN)MemoryMap < (UINTN)InsertionPoint) &&
        ((UINTN)InsertionPoint >= (UINTN)MemoryMap + *MemoryMapSize) &&
        ((UINTN)InsertionPoint < (UINTN)MemoryMap + MemoryMapBufferSize)))
  {
    DEBUG ((DEBUG_ERROR, "%a - Input InsertionPoint is Invalid!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  SortMemoryMap (MemoryMap, *MemoryMapSize, *DescriptorSize);
  SortMemorySpaceMap (MemorySpaceMap, MemorySpaceMapDescriptorCount, MemorySpaceMapDescriptorSize);
  if ((InsertionPoint != NULL) && !DetermineSize) {
    ZeroMem (InsertionPoint, MemoryMapBufferSize - *MemoryMapSize);
  }

  AdditionalEntriesCount = 0;
  StartOfAddressSpace    = MemorySpaceMap[0].BaseAddress;
  EndOfAddressSpace      = MemorySpaceMap[*MemorySpaceMapDescriptorCount - 1].BaseAddress +
                           MemorySpaceMap[*MemorySpaceMapDescriptorCount - 1].Length;
  MemoryMapCurrent = MemoryMap;
  MemoryMapEnd     = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);

  // Check if we need to insert a new entry at the start of the memory map
  if (MemoryMapCurrent->PhysicalStart > StartOfAddressSpace) {
    do {
      OverlapLength   = MemoryMapCurrent->PhysicalStart - StartOfAddressSpace;
      RemainingLength = GetOverlappingMemorySpaceRegion (
                          MemorySpaceMap,
                          MemorySpaceMapDescriptorCount,
                          &StartOfAddressSpace,
                          &OverlapLength,
                          &GcdType
                          );
      if ((GcdType != EfiGcdMemoryTypeNonExistent)) {
        if (!DetermineSize) {
          POPULATE_MEMORY_DESCRIPTOR_ENTRY (
            InsertionPoint,
            StartOfAddressSpace,
            EfiSizeToPages (MemoryMapCurrent->PhysicalStart - StartOfAddressSpace - RemainingLength),
            GcdTypeToEfiType (&GcdType)
            );

          InsertionPoint = NEXT_MEMORY_DESCRIPTOR (InsertionPoint, *DescriptorSize);
        } else {
          AdditionalEntriesCount++;
        }
      }

      StartOfAddressSpace = MemoryMapCurrent->PhysicalStart - RemainingLength;
    } while (RemainingLength > 0);
  }

  while (MemoryMapCurrent < MemoryMapEnd) {
    if (NEXT_MEMORY_DESCRIPTOR (MemoryMapCurrent, *DescriptorSize) < MemoryMapEnd) {
      LastEntryEnd   = MemoryMapCurrent->PhysicalStart + EfiPagesToSize (MemoryMapCurrent->NumberOfPages);
      NextEntryStart = NEXT_MEMORY_DESCRIPTOR (MemoryMapCurrent, *DescriptorSize)->PhysicalStart;
      // Check for a gap in the memory map
      if (NextEntryStart > LastEntryEnd) {
        // Fill in missing region based on the GCD Memory Map
        do {
          OverlapLength   = NextEntryStart - LastEntryEnd;
          RemainingLength = GetOverlappingMemorySpaceRegion (
                              MemorySpaceMap,
                              MemorySpaceMapDescriptorCount,
                              &LastEntryEnd,
                              &OverlapLength,
                              &GcdType
                              );
          if ((GcdType != EfiGcdMemoryTypeNonExistent)) {
            if (!DetermineSize) {
              POPULATE_MEMORY_DESCRIPTOR_ENTRY (
                InsertionPoint,
                LastEntryEnd,
                EfiSizeToPages (NextEntryStart - LastEntryEnd - RemainingLength),
                GcdTypeToEfiType (&GcdType)
                );
              InsertionPoint = NEXT_MEMORY_DESCRIPTOR (InsertionPoint, *DescriptorSize);
            } else {
              AdditionalEntriesCount++;
            }
          }

          LastEntryEnd = NextEntryStart - RemainingLength;
        } while (RemainingLength > 0);
      }
    }

    MemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (MemoryMapCurrent, *DescriptorSize);
  }

  LastEntryEnd = PREVIOUS_MEMORY_DESCRIPTOR (MemoryMapCurrent, *DescriptorSize)->PhysicalStart +
                 EfiPagesToSize (PREVIOUS_MEMORY_DESCRIPTOR (MemoryMapCurrent, *DescriptorSize)->NumberOfPages);

  // Check if we need to insert a new entry at the end of the memory map
  if (EndOfAddressSpace > LastEntryEnd) {
    do {
      OverlapLength   = EndOfAddressSpace - LastEntryEnd;
      RemainingLength = GetOverlappingMemorySpaceRegion (
                          MemorySpaceMap,
                          MemorySpaceMapDescriptorCount,
                          &LastEntryEnd,
                          &OverlapLength,
                          &GcdType
                          );
      if ((GcdType != EfiGcdMemoryTypeNonExistent)) {
        if (!DetermineSize) {
          POPULATE_MEMORY_DESCRIPTOR_ENTRY (
            InsertionPoint,
            LastEntryEnd,
            EfiSizeToPages (EndOfAddressSpace - LastEntryEnd - RemainingLength),
            GcdTypeToEfiType (&GcdType)
            );
          InsertionPoint = NEXT_MEMORY_DESCRIPTOR (InsertionPoint, *DescriptorSize);
        } else {
          AdditionalEntriesCount++;
        }
      }

      LastEntryEnd = EndOfAddressSpace - RemainingLength;
    } while (RemainingLength > 0);
  }

  if (DetermineSize) {
    *MemoryMapSize = *MemoryMapSize + (AdditionalEntriesCount * *DescriptorSize);
  } else {
    *MemoryMapSize = (UINTN)InsertionPoint - (UINTN)MemoryMap;
    SortMemoryMap (MemoryMap, *MemoryMapSize, *DescriptorSize);
  }

  return EFI_SUCCESS;
}

// ---------------------------------------
//              CORE LOGIC
// ---------------------------------------

/**
 Generate a list of IMAGE_RANGE_DESCRIPTOR structs which describe the data/code regions of protected images or
 the memory ranges of nonprotected images.

 @param[in]  ImageList                  Pointer to NULL IMAGE_RANGE_DESCRIPTOR* which will be updated to the head of the allocated
                                        IMAGE_RANGE_DESCRIPTOR list
 @param[in]  ProtectedOrNonProtected    Enum describing if the returned list will describe the protected or
                                        nonprotected loaded images

 @retval  EFI_SUCCESS             *ImageList points to the head of the IMAGE_RANGE_DESCRIPTOR list
 @retval  EFI_INVALID_PARAMETER   ImageList is NULL or *ImageList is not NULL
 @retval  EFI_OUT_OF_RESOURCES    Allocation of memory failed
**/
EFI_STATUS
EFIAPI
GetImageList (
  IN IMAGE_RANGE_DESCRIPTOR         **ImageList,
  IN IMAGE_RANGE_PROTECTION_STATUS  ProtectedOrNonProtected
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  LIST_ENTRY                            *ImageRecordCodeSectionLink;
  LIST_ENTRY                            *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                            *ImageRecordCodeSectionList;
  IMAGE_PROPERTIES_RECORD               *ImageRecord;
  LIST_ENTRY                            *ImageRecordLink;
  LIST_ENTRY                            *ImageListHead;
  UINT64                                PhysicalStart, PhysicalEnd;
  IMAGE_RANGE_DESCRIPTOR                *CurrentImageRangeDescriptor;

  if ((ImageList == NULL) || (*ImageList != NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProtectedOrNonProtected == Protected) {
    ImageListHead = &mImagePropertiesPrivate.ImageRecordList;
  } else if (ProtectedOrNonProtected == NonProtected) {
    ImageListHead = &mNonProtectedImageRangesPrivate.NonProtectedImageList;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  *ImageList = AllocateZeroPool (sizeof (IMAGE_RANGE_DESCRIPTOR));

  if (*ImageList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&(*ImageList)->Link);

  // Walk through each image
  for (ImageRecordLink = ImageListHead->ForwardLink;
       ImageRecordLink != ImageListHead;
       ImageRecordLink = ImageRecordLink->ForwardLink)
  {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );
    PhysicalStart = ImageRecord->ImageBase;
    PhysicalEnd   = ImageRecord->ImageBase + ImageRecord->ImageSize;

    ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

    ImageRecordCodeSectionLink    = ImageRecordCodeSectionList->ForwardLink;
    ImageRecordCodeSectionEndLink = ImageRecordCodeSectionList;
    while (ImageRecordCodeSectionLink != ImageRecordCodeSectionEndLink) {
      ImageRecordCodeSection = CR (
                                 ImageRecordCodeSectionLink,
                                 IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                                 Link,
                                 IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                                 );
      ImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;

      // Mark the data region
      if (PhysicalStart < ImageRecordCodeSection->CodeSegmentBase) {
        CurrentImageRangeDescriptor = AllocatePool (sizeof (IMAGE_RANGE_DESCRIPTOR));
        if (CurrentImageRangeDescriptor == NULL) {
          goto OutOfResourcesCleanup;
        }

        POPULATE_IMAGE_RANGE_DESCRIPTOR (CurrentImageRangeDescriptor, Data, PhysicalStart, ImageRecordCodeSection->CodeSegmentBase - PhysicalStart);
        PhysicalStart = ImageRecordCodeSection->CodeSegmentBase;
        InsertTailList (&(*ImageList)->Link, &CurrentImageRangeDescriptor->Link);
      }

      // Mark the code region
      CurrentImageRangeDescriptor = AllocatePool (sizeof (IMAGE_RANGE_DESCRIPTOR));
      if (CurrentImageRangeDescriptor == NULL) {
        goto OutOfResourcesCleanup;
      }

      POPULATE_IMAGE_RANGE_DESCRIPTOR (CurrentImageRangeDescriptor, Code, PhysicalStart, ImageRecordCodeSection->CodeSegmentSize);
      PhysicalStart = ImageRecordCodeSection->CodeSegmentBase + ImageRecordCodeSection->CodeSegmentSize;
      InsertTailList (&(*ImageList)->Link, &CurrentImageRangeDescriptor->Link);
    }

    // Mark the remainder of the image as a data section
    if (PhysicalStart < PhysicalEnd) {
      CurrentImageRangeDescriptor = AllocatePool (sizeof (IMAGE_RANGE_DESCRIPTOR));
      if (CurrentImageRangeDescriptor == NULL) {
        goto OutOfResourcesCleanup;
      }

      POPULATE_IMAGE_RANGE_DESCRIPTOR (CurrentImageRangeDescriptor, Data, PhysicalStart, PhysicalEnd - PhysicalStart);
      PhysicalStart = PhysicalEnd;
      InsertTailList (&(*ImageList)->Link, &CurrentImageRangeDescriptor->Link);
    }
  }

  return EFI_SUCCESS;

OutOfResourcesCleanup:
  ImageRecordLink = &(*ImageList)->Link;

  while (!IsListEmpty (ImageRecordLink)) {
    CurrentImageRangeDescriptor = CR (
                                    ImageRecordLink->ForwardLink,
                                    IMAGE_RANGE_DESCRIPTOR,
                                    Link,
                                    IMAGE_RANGE_DESCRIPTOR_SIGNATURE
                                    );

    RemoveEntryList (&CurrentImageRangeDescriptor->Link);
    FreePool (CurrentImageRangeDescriptor);
  }

  FreePool (*ImageList);

  return EFI_OUT_OF_RESOURCES;
}

/**
  Create an image properties record and insert it into the nonprotected image list

  @param[in]  ImageBase               Base of PE image
  @param[in]  ImageSize               Size of PE image

  @retval     EFI_INVALID_PARAMETER   ImageSize was zero
  @retval     EFI_OUT_OF_RESOURCES    Failure to Allocate()
  @retval     EFI_SUCCESS             The image properties record was successfully created and inserted
                                      into the nonprotected image list
**/
STATIC
EFI_STATUS
CreateNonProtectedImagePropertiesRecord (
  IN    EFI_PHYSICAL_ADDRESS  ImageBase,
  IN    UINT64                ImageSize
  )
{
  EFI_STATUS               Status       = EFI_SUCCESS;
  IMAGE_PROPERTIES_RECORD  *ImageRecord = NULL;

  if (ImageSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  ImageRecord = AllocateZeroPool (sizeof (*ImageRecord));

  if (ImageRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ImageRecord->Signature        = IMAGE_PROPERTIES_RECORD_SIGNATURE;
  ImageRecord->ImageBase        = ImageBase;
  ImageRecord->ImageSize        = ImageSize;
  ImageRecord->CodeSegmentCount = 0;
  InitializeListHead (&ImageRecord->CodeSegmentList);

  Status = OrderedInsertUint64Comparison (
             &mNonProtectedImageRangesPrivate.NonProtectedImageList,
             &ImageRecord->Link,
             OFFSET_OF (IMAGE_PROPERTIES_RECORD, ImageBase) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
             OFFSET_OF (IMAGE_PROPERTIES_RECORD, Signature) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
             IMAGE_PROPERTIES_RECORD_SIGNATURE
             );

  if (EFI_ERROR (Status)) {
    DeleteImagePropertiesRecord (ImageRecord);
  } else {
    mNonProtectedImageRangesPrivate.NonProtectedImageCount++;
  }

  return Status;
}

/**
  Split memory map descriptors based on the input special region list. After the function, one or more memory map
  descriptors will divide evenly into every special region so attributes can be targeted at those regions. Every
  descriptor covered by a special region will have its virtual address set to SPECIAL_REGION_PATTERN.

  @param[in, out] MemoryMapSize                   IN:   The size, in bytes, of the old memory map before the split
                                                  OUT:  The size, in bytes, of the used descriptors of the split
                                                        memory map
  @param[in, out] MemoryMap                       IN:   A pointer to the buffer containing a sorted memory map
                                                  OUT:  A pointer to the updated memory map
  @param[in]      DescriptorSize                  The size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR
  @param[in]      BufferSize                      The size, in bytes, of the full memory map buffer
  @param[in]      SpecialRegionList               List of special regions to separate. This list should be sorted.

  @retval         EFI_SUCCESS                     Memory map has been split
  @retval         EFI_NOT_FOUND                   Unable to find a special region
  @retval         EFI_INVALID_PARAMETER           An input was NULL
**/
EFI_STATUS
SeparateSpecialRegionsInMemoryMap (
  IN OUT      UINTN                  *MemoryMapSize,
  IN OUT      EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN CONST    UINTN                  *DescriptorSize,
  IN CONST    UINTN                  *BufferSize,
  IN CONST    LIST_ENTRY             *SpecialRegionList
  )
{
  EFI_MEMORY_DESCRIPTOR                        *MemoryMapEntry, *MemoryMapEnd, *MapEntryInsert;
  LIST_ENTRY                                   *SpecialRegionEntryLink;
  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY  *SpecialRegionEntry;
  UINTN                                        SpecialRegionStart, SpecialRegionEnd, MapEntryStart, MapEntryEnd;

  if ((MemoryMapSize == NULL) || (MemoryMap == NULL) ||
      (DescriptorSize == NULL) || (BufferSize == NULL) ||
      (SpecialRegionList == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);
  MapEntryInsert = MemoryMapEnd;

  SpecialRegionEntryLink = SpecialRegionList->ForwardLink;
  while (SpecialRegionEntryLink != SpecialRegionList) {
    SpecialRegionEntry = CR (
                           SpecialRegionEntryLink,
                           MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY,
                           Link,
                           MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
                           );

    SpecialRegionStart = (UINTN)SpecialRegionEntry->SpecialRegion.Start;
    SpecialRegionEnd   = (UINTN)SpecialRegionEntry->SpecialRegion.Start + (UINTN)SpecialRegionEntry->SpecialRegion.Length;

    while ((MemoryMapEntry < MemoryMapEnd) &&
           (SpecialRegionStart < SpecialRegionEnd) &&
           (((UINTN)MapEntryInsert + *DescriptorSize) < ((UINTN)MemoryMap + *BufferSize)))
    {
      MapEntryStart = (UINTN)MemoryMapEntry->PhysicalStart;
      MapEntryEnd   = (UINTN)MemoryMapEntry->PhysicalStart + (UINTN)EFI_PAGES_TO_SIZE (MemoryMapEntry->NumberOfPages);
      if (CHECK_OVERLAP (SpecialRegionStart, SpecialRegionEnd, MapEntryStart, MapEntryEnd)) {
        // Check if some portion before the map entry isn't covered by the special region
        if (MapEntryStart < SpecialRegionStart) {
          // Populate a new descriptor for the region before the special region. This entry can go to the end
          // of the memory map because the special region list is sorted
          POPULATE_MEMORY_DESCRIPTOR_ENTRY (
            MapEntryInsert,
            MapEntryStart,
            EFI_SIZE_TO_PAGES (SpecialRegionStart - MapEntryStart),
            MemoryMapEntry->Type
            );
          MapEntryInsert->Attribute = MemoryMapEntry->Attribute;

          // Update this descriptor to start at the special region start
          MemoryMapEntry->NumberOfPages -= MapEntryInsert->NumberOfPages;
          MemoryMapEntry->PhysicalStart  = SpecialRegionStart;
          MapEntryStart                  = SpecialRegionStart;

          // Get the next blank map entry
          MapEntryInsert = NEXT_MEMORY_DESCRIPTOR (MapEntryInsert, *DescriptorSize);
        }

        // If the special region ends after this region, get the next entry
        if (SpecialRegionEnd > MapEntryEnd) {
          SpecialRegionStart           = MapEntryEnd;
          MemoryMapEntry->Attribute    = SpecialRegionEntry->SpecialRegion.EfiAttributes;
          MemoryMapEntry->VirtualStart = SPECIAL_REGION_PATTERN;

          // Continue to the next memory map descriptor
          MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
          continue;
        }

        // If the special region ends before the end of this descriptor region, insert a new record at the end
        // of the memory map for the remaining region
        if (SpecialRegionEnd < MapEntryEnd) {
          // SpecialRegionStart is now guaranteed to be equal to MapEntryStart. Populate a new descriptor
          // for the region covered by the special region. This entry needs to go to
          // the end of the memory map in case a subsequent special region will cover some portion
          // of the remaining map entry region
          POPULATE_MEMORY_DESCRIPTOR_ENTRY (
            MapEntryInsert,
            SpecialRegionStart,
            EFI_SIZE_TO_PAGES (SpecialRegionEnd - SpecialRegionStart),
            MemoryMapEntry->Type
            );
          MapEntryInsert->Attribute    = SpecialRegionEntry->SpecialRegion.EfiAttributes;
          MapEntryInsert->VirtualStart = SPECIAL_REGION_PATTERN;

          // Trim the current memory map entry
          MemoryMapEntry->NumberOfPages -= MapEntryInsert->NumberOfPages;
          MemoryMapEntry->PhysicalStart  = SpecialRegionEnd;

          // Get the next blank map entry
          MapEntryInsert = NEXT_MEMORY_DESCRIPTOR (MapEntryInsert, *DescriptorSize);

          // Break the loop to get the next special region which will need to be checked against the remainder
          // of this map entry
          break;
        }

        // This entry is covered entirely by the special region. Update the attributes and mark this
        // entry as a special region
        MemoryMapEntry->Attribute    = SpecialRegionEntry->SpecialRegion.EfiAttributes;
        MemoryMapEntry->VirtualStart = SPECIAL_REGION_PATTERN;
        SpecialRegionStart           = MapEntryEnd;
      }

      // If we've fallen through to this point, we need to get the next memory map entry
      MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
    }

    if (SpecialRegionStart > SpecialRegionEnd) {
      return EFI_NOT_FOUND;
    }

    SpecialRegionEntryLink = SpecialRegionEntryLink->ForwardLink;
  }

  // if we've created new records, sort the map
  if ((UINTN)MapEntryInsert > (UINTN)MemoryMapEnd) {
    // Sort from low to high
    SortMemoryMap (
      MemoryMap,
      (UINTN)MapEntryInsert - (UINTN)MemoryMap,
      *DescriptorSize
      );

    // Update the memory map size to be the new number of records
    *MemoryMapSize = (UINTN)MapEntryInsert - (UINTN)MemoryMap;
  }

  return EFI_SUCCESS;
}

/**
  Applies EFI_MEMORY_ACCESS_MASK to each memory map entry

  @param[in]      MemoryMapSize     A pointer to the size, in bytes, of the
                                    MemoryMap buffer
  @param[in, out] MemoryMap         A pointer to the buffer containing the memory map
  @param[in]      DescriptorSize    Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR
**/
STATIC
VOID
FilterMemoryMapAttributes (
  IN CONST UINTN                *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN CONST UINTN                *DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);

  while (MemoryMapEntry < MemoryMapEnd) {
    MemoryMapEntry->Attribute &= EFI_MEMORY_ACCESS_MASK;
    MemoryMapEntry             = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
  }
}

/**
  Set every bit in the bitmap which corresponds to a memory map descriptor with nonzero attributes.

  @param[in]        MemoryMapSize           A pointer to the size, in bytes, of the MemoryMap buffer
  @param[in]        MemoryMap               A pointer to the current memory map
  @param[in]        DescriptorSize          Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR
  @param[out]       Bitmap                  Pointer to the beginning of the bitmap to be updated

  @retval           EFI_SUCCESS             Bitmap was updated
  @retval           EFI_INVALID_PARAMETER   An input was NULL
**/
STATIC
EFI_STATUS
SyncBitmap (
  IN  CONST   UINTN                  *MemoryMapSize,
  IN          EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN  CONST   UINTN                  *DescriptorSize,
  OUT CONST   UINT8                  *Bitmap
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  UINTN                  Index = 0;

  if ((MemoryMapSize == NULL) || (MemoryMap == NULL) ||
      (DescriptorSize == NULL) || (Bitmap == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);

  while (MemoryMapEntry < MemoryMapEnd) {
    // If attributes are nonzero or the virtual address (always zero during boot services according to UEFI Spec 2.9)
    // is set to the pattern indicating it is a special memory region, set the corresponding bit in the bitmap
    if ((MemoryMapEntry->Attribute != 0) || (MemoryMapEntry->VirtualStart == SPECIAL_REGION_PATTERN)) {
      SET_BITMAP_INDEX (Bitmap, Index);
    }

    Index++;
    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
  }

  return EFI_SUCCESS;
}

/**
  Set access attributes in the memory map based on the memory protection policy and
  mark visited regions in the bitmap.

  @param[in]        MemoryMapSize           A pointer to the size, in bytes, of the MemoryMap buffer
  @param[in, out]   MemoryMap               A pointer to the current memory map
  @param[in]        DescriptorSize          Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR
  @param[in, out]   Bitmap                  Pointer to the beginning of the bitmap to be updated

  @retval           EFI_SUCCESS             Access attributes and bitmap updated
  @retval           EFI_INVALID_PARAMETER   An input was NULL
**/
STATIC
EFI_STATUS
SetAccessAttributesInMemoryMap (
  IN CONST  UINTN                  *MemoryMapSize,
  IN OUT    EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN CONST  UINTN                  *DescriptorSize,
  IN OUT    UINT8                  *Bitmap
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  UINTN                  Index = 0;

  if ((MemoryMapSize == NULL) || (MemoryMap == NULL) ||
      (DescriptorSize == NULL) || (Bitmap == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);

  while (MemoryMapEntry < MemoryMapEnd) {
    if (!IS_BITMAP_INDEX_SET (Bitmap, Index)) {
      MemoryMapEntry->Attribute = GetPermissionAttributeForMemoryType (MemoryMapEntry->Type);
      SET_BITMAP_INDEX (Bitmap, Index);
    }

    Index++;
    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
  }

  return EFI_SUCCESS;
}

/**
  Merge contiguous memory map entries with the same attributes.

  @param  MemoryMap              A pointer to the memory map
  @param  MemoryMapSize          A pointer to the size, in bytes, of the
                                 MemoryMap buffer. On input, this is the size of the current
                                 memory map.  On output, it is the size of new memory map after merge.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
MergeMemoryMapByAttribute (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN OUT UINTN                  *MemoryMapSize,
  IN CONST UINTN                *DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  UINT64                 MemoryBlockLength;
  EFI_MEMORY_DESCRIPTOR  *NewMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *NextMemoryMapEntry;

  if ((MemoryMap == NULL) || (MemoryMapSize == NULL) || (DescriptorSize == NULL)) {
    return;
  }

  MemoryMapEntry    = MemoryMap;
  NewMemoryMapEntry = MemoryMap;
  MemoryMapEnd      = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    CopyMem (NewMemoryMapEntry, MemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);

    do {
      MemoryBlockLength = (UINT64)(EfiPagesToSize (NewMemoryMapEntry->NumberOfPages));
      if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) &&
          (NewMemoryMapEntry->Attribute == NextMemoryMapEntry->Attribute) &&
          ((NewMemoryMapEntry->PhysicalStart + MemoryBlockLength) == NextMemoryMapEntry->PhysicalStart))
      {
        NewMemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
        NextMemoryMapEntry                = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, *DescriptorSize);
        continue;
      } else {
        MemoryMapEntry = PREVIOUS_MEMORY_DESCRIPTOR (NextMemoryMapEntry, *DescriptorSize);
        break;
      }
    } while (TRUE);

    MemoryMapEntry    = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
    NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapEntry, *DescriptorSize);
  }

  *MemoryMapSize = (UINTN)NewMemoryMapEntry - (UINTN)MemoryMap;

  return;
}

/**
  Removes the access attributes from memory map descriptors which match the elements in the
  input IMAGE_PROPERTIES_RECORD list.

  @param[in]      MemoryMapSize           A pointer to the size, in bytes, of the MemoryMap buffer
  @param[out]     MemoryMap               A pointer to the buffer containing the memory map. This
                                          memory map must be sorted.
  @param[in]      DescriptorSize          Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR
  @param[in]      NonProtectedImageList   List of IMAGE_PROPERTIES_RECORD entries. This list
                                          must be sorted.

  @retval   EFI_SUCCESS                   The bitmap was updated
  @retval   EFI_INVALID_PARAMETER         MemoryMapSize was NULL, DescriptorSize was NULL,
                                          Bitmap was NULL, or MemoryMap was NULL
  @retval   EFI_NOT_FOUND                 No memory map entry matched an image properties record described
                                          in the input IMAGE_PROPERTIES_RECORD list
**/
STATIC
EFI_STATUS
RemoveAttributesOfNonProtectedImageRanges (
  IN CONST  UINTN                  *MemoryMapSize,
  OUT       EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN CONST  UINTN                  *DescriptorSize,
  IN        LIST_ENTRY             *NonProtectedImageList
  )
{
  LIST_ENTRY               *NonProtectedImageRecordLink = NULL;
  IMAGE_PROPERTIES_RECORD  *NonProtectedImageRecord = NULL;
  EFI_MEMORY_DESCRIPTOR    *MemoryMapEntry = NULL;
  EFI_MEMORY_DESCRIPTOR    *MemoryMapEnd = NULL;
  UINTN                    NonProtectedStart, NonProtectedEnd, MapEntryStart, MapEntryEnd;

  if ((MemoryMapSize == NULL) || (MemoryMap == NULL) ||
      (NonProtectedImageList == NULL) || (DescriptorSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  NonProtectedImageRecordLink = NonProtectedImageList->ForwardLink;
  MemoryMapEntry              = MemoryMap;
  MemoryMapEnd                = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);

  while (NonProtectedImageRecordLink != NonProtectedImageList) {
    if (MemoryMapEntry >= MemoryMapEnd) {
      break;
    }

    NonProtectedImageRecord = CR (
                                NonProtectedImageRecordLink,
                                IMAGE_PROPERTIES_RECORD,
                                Link,
                                IMAGE_PROPERTIES_RECORD_SIGNATURE
                                );

    NonProtectedStart = (UINTN)NonProtectedImageRecord->ImageBase;
    NonProtectedEnd   = (UINTN)NonProtectedImageRecord->ImageBase + (UINTN)NonProtectedImageRecord->ImageSize;

    while ((MemoryMapEntry < MemoryMapEnd) && (NonProtectedStart < NonProtectedEnd)) {
      MapEntryStart = (UINTN)MemoryMapEntry->PhysicalStart;
      MapEntryEnd   = (UINTN)MemoryMapEntry->PhysicalStart +  (UINTN)EFI_PAGES_TO_SIZE (MemoryMapEntry->NumberOfPages);

      if ((NonProtectedStart == MapEntryStart)) {
        if (MemoryMapEntry->VirtualStart != SPECIAL_REGION_PATTERN) {
          MemoryMapEntry->Attribute = 0;
        } else {
          MemoryMapEntry->VirtualStart = 0;
        }

        NonProtectedStart = MapEntryEnd;
      }

      MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
    }

    NonProtectedImageRecordLink = NonProtectedImageRecordLink->ForwardLink;
  }

  if (NonProtectedImageRecordLink != NonProtectedImageList) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Create a memory map which describes all of memory. The returned memory map will have access
  attributes (EFI_MEMORY_XP, EFI_MEMORY_RP, EFI_MEMORY_RO) consistent with
  the memory protection policy and can be used to secure system memory.

  NOTE: To create the memory map, several allocation and free calls will need to be performed.
        To avoid changing the memory map during the creation process, some calls to free memory
        will be deferred until the caller is done with the produced memory map. The caller of
        this function is responsible for calling CleanupMemoryMapWithPopulatedAccessAttributes()
        to free the memory allocated by this function.

  @param[out]     MemoryMapSize           A pointer to the size, in bytes, of the
                                          MemoryMap buffer
  @param[out]     MemoryMap               A pointer to the buffer containing the memory map
                                          with EFI access attributes which should be applied
  @param[out]     DescriptorSize          A pointer to the size, in bytes, of an individual
                                          EFI_MEMORY_DESCRIPTOR

  @retval         EFI_SUCCESS             The memory map was returned in the MemoryMap buffer
  @retval         EFI_OUT_OF_RESOURCES    Failed to allocate memory
  @retval         Other                   One of the supporting functions failed
**/
EFI_STATUS
EFIAPI
GetMemoryMapWithPopulatedAccessAttributes (
  OUT UINTN                  *MemoryMapSize,
  OUT EFI_MEMORY_DESCRIPTOR  **MemoryMap,
  OUT UINTN                  *DescriptorSize
  )
{
  EFI_STATUS  Status;
  UINTN       AdditionalRecordsForImages, NumMemoryMapDescriptors, NumBitmapEntries, \
              NumMemorySpaceMapDescriptors, MemorySpaceMapDescriptorSize, MapKey, \
              BitmapIndex, FinalMemoryMapBufferSize;
  UINT32                           DescriptorVersion;
  LIST_ENTRY                       *MergedImageList = NULL;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap  = NULL;

  if ((MemoryMapSize == NULL) || (MemoryMap == NULL) ||
      (*MemoryMap != NULL) || (DescriptorSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // STEP 1: Get the Initial Memory Map
  //
  // The memory map fetched from this routine will be used to determine
  // the amount of memory needed to create a hybrid memory map which
  // describes the full address space with all the attributes required
  // to secure memory as much as possible based on the platform protection
  // policy.
  {
    *MemoryMapSize = 0;

    Status = CoreGetMemoryMap (
               MemoryMapSize,
               *MemoryMap,
               &MapKey,
               DescriptorSize,
               &DescriptorVersion
               );

    ASSERT (Status == EFI_BUFFER_TOO_SMALL);

    do {
      *MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocatePool (*MemoryMapSize);
      if (*MemoryMap == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        ASSERT_EFI_ERROR (Status);
        goto Cleanup;
      }

      Status = CoreGetMemoryMap (
                 MemoryMapSize,
                 *MemoryMap,
                 &MapKey,
                 DescriptorSize,
                 &DescriptorVersion
                 );
      if (EFI_ERROR (Status)) {
        FreePool (*MemoryMap);
      }
    } while (Status == EFI_BUFFER_TOO_SMALL);

    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto Cleanup;
    }
  }

  // STEP 2: Get the GCD Memory Map
  //
  // The GCD memory map will be used to fill in memory regions contained
  // in the full address space but not described by the EFI memory map.
  // While the logic could get the address width from the CPU info HOB,
  // the GCD memory type of regions being filled in will be used to
  // deterimine the appropriate access attributes.
  {
    Status = CoreGetMemorySpaceMap (&NumMemorySpaceMapDescriptors, &MemorySpaceMap);
    ASSERT_EFI_ERROR (Status);
  }

  // STEP 3: Determine the Space Required for the Memory Map with Gaps Filled in
  //
  // Calling FillInMemoryMap() with DetermineSize == TRUE will return the
  // size required to add the extra descriptors which fill in the address gaps.
  {
    if (MemorySpaceMap != NULL) {
      MemorySpaceMapDescriptorSize = sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR);

      // Determine how large the filled in memory map will be.
      Status = FillInMemoryMap (
                 MemoryMapSize,
                 *MemoryMap,
                 *MemoryMapSize,
                 NULL,
                 DescriptorSize,
                 &NumMemorySpaceMapDescriptors,
                 MemorySpaceMap,
                 &MemorySpaceMapDescriptorSize,
                 TRUE
                 );

      ASSERT_EFI_ERROR (Status);
    }
  }

  // STEP 4: Create a Unified List Describing All Images Loaded on the Platform
  //
  // To ensure attributes are not applied to image memory ranges based on the
  // memory type, the logic needs to track all loaded images. This module hosts
  // a list of protected and non-protected images present on the system which is
  // updated each time an image is loaded. For obvious reasons, this logic must
  // differentiate between protected and non-protected images when determining
  // the appropriate access attributes.
  //
  // To split up the memory map based on loaded images, this logic unifies
  // both lists of protected and non-protected images into a single list.
  // After the memory map has been broken up and before the access attributes
  // for each range are determined, the list will be split back to its original
  // state.
  {
    if (mImagePropertiesPrivate.ImageRecordCount == 0) {
      MergedImageList = &mNonProtectedImageRangesPrivate.NonProtectedImageList;
    } else if (mNonProtectedImageRangesPrivate.NonProtectedImageCount == 0) {
      MergedImageList = &mImagePropertiesPrivate.ImageRecordList;
    }
    // If both protected and non-protected images are loaded, merge the two lists
    else {
      MergedImageList = &mImagePropertiesPrivate.ImageRecordList;
      Status          = MergeListsUint64Comparison (
                          MergedImageList,
                          &mNonProtectedImageRangesPrivate.NonProtectedImageList,
                          &mNonProtectedImageRangesPrivate.NonProtectedImageCount,
                          &mArrayOfListEntryPointers,
                          OFFSET_OF (IMAGE_PROPERTIES_RECORD, ImageBase) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
                          OFFSET_OF (IMAGE_PROPERTIES_RECORD, Signature) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
                          IMAGE_PROPERTIES_RECORD_SIGNATURE
                          );

      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto Cleanup;
      }
    }
  }

  // STEP 5: Determine the Number of Descriptors Required for Loaded Images
  //
  // The below formula is used to determine the number of descriptors
  // required to describe loaded images in the worst case.
  //
  // |         |      |      |      |      |      |         |
  // | 4K PAGE | DATA | CODE | DATA | CODE | DATA | 4K PAGE |
  // |         |      |      |      |      |      |         |
  // Assume the above memory region is currently one memory map descriptor. This image layout example contains
  // two code sections oriented in a way that maximizes the number of descriptors which would be required
  // to describe each section. We can see that there are two code sections (let's say CodeSegmentMax == 2),
  // three data sections, and two memory regions flanking the image. Using the first part of the below formula,
  // the number of required descriptors to describe this layout will be 2 * 2 + 3 == 7 which matches the above example.
  // To ensure we have enough space for every descriptor of the broken up memory map, we assume that every
  // image will have the maximum number of code sections oriented in a way which maximizes the number of
  // data sections with unrelated memory regions flanking each image within a single descriptor.

  // |         |       |         |
  // | 4K PAGE | IMAGE | 4K PAGE |
  // |         |       |         |
  // Assume the above memory region is currently one memory map descriptor. This layout describes a nonprotected
  // image (so we don't split it by code/data sections) with flanking unrelated memory regions. In this case, the
  // number of descriptors required to describe this region will be 3. To ensure we have enough descriptors to
  // describe every nonprotected image, we must have 3 * <number of nonprotected images> additional descriptors.
  {
    AdditionalRecordsForImages = ((2 * mImagePropertiesPrivate.CodeSegmentCountMax + 3) * mImagePropertiesPrivate.ImageRecordCount) +
                                 (mNonProtectedImageRangesPrivate.NonProtectedImageCount * 3);
  }

  // STEP 6: Determine the Size of the Final Memory Map
  //
  // The final memory map buffer size will include:
  //
  // 1. The size required to describe the EFI memory map returned from GetMemoryMap()
  // 2. The size required to populate entries which are not in the EFI memory map but are included
  //    in the full address space described by the GCD memory map (which inherits its address width
  //    from the CPU info HOB).
  // 3. The size required to split all protected and non-protected image ranges into their own descriptors.
  // 4. The size required to split all special regions into their own descriptors (done by multiplying
  //    the memory map size by 2 to accomodate the worst-case scenario).
  {
    *MemoryMapSize           = (*MemoryMapSize * 2) + ((*DescriptorSize) * AdditionalRecordsForImages);
    FinalMemoryMapBufferSize = *MemoryMapSize;
  }

  // STEP 7: Allocate Memory for the Final Memory Map
  {
    FreePool (*MemoryMap);
    *MemoryMap = AllocateZeroPool (FinalMemoryMapBufferSize);

    if (*MemoryMap == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      ASSERT_EFI_ERROR (Status);
      goto Cleanup;
    }
  }

  // STEP 8: Create a Bitmap to Track Which Descriptors Have Been Updated with Attributes
  //
  // The pool allocated for the bitmap will be large enough to hold a bit for every descriptor
  // in the expanded memory map, but in almost all cases the bitmap will be smaller than this.
  // This bitmap must be freed after the memory map is no longer in use to avoid changing the
  // real memory map layout.
  {
    NumMemoryMapDescriptors = FinalMemoryMapBufferSize / *DescriptorSize;
    NumBitmapEntries        = (NumMemoryMapDescriptors % 8) == 0 ? NumMemoryMapDescriptors : (((NumMemoryMapDescriptors / 8) * 8) + 8);
    mBitmapGlobal           = AllocateZeroPool (NumBitmapEntries / 8);

    if (mBitmapGlobal == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      ASSERT_EFI_ERROR (Status);
      goto Cleanup;
    }
  }

  // STEP 9: Get the EFI Memory Map
  //
  // From this point onward there will be no more allocations or frees within this function
  // to ensure the memory map is not changed during the creation process. This block fetches
  // the current EFI memory map now that it has stabilized.
  {
    Status = CoreGetMemoryMap (
               MemoryMapSize,
               *MemoryMap,
               &MapKey,
               DescriptorSize,
               &DescriptorVersion
               );

    ASSERT_EFI_ERROR (Status);
  }

  // STEP 10: Update the EFI Memory Map to Describe the Full Address Space
  {
    Status = FillInMemoryMap (
               MemoryMapSize,
               *MemoryMap,
               FinalMemoryMapBufferSize,
               (EFI_MEMORY_DESCRIPTOR *)(((UINT8 *)*MemoryMap) + *MemoryMapSize),
               DescriptorSize,
               &NumMemorySpaceMapDescriptors,
               MemorySpaceMap,
               &MemorySpaceMapDescriptorSize,
               FALSE
               );

    ASSERT_EFI_ERROR (Status);
  }

  // STEP 11: Filter the Memory Map Attributes to Only Access Attributes
  //
  // The memory map returned from CoreGetMemoryMap() may include non-access
  // attributes such as caching attributes. These extra attributes are
  // not the output of this function so they should be filtered out.
  {
    FilterMemoryMapAttributes (MemoryMapSize, *MemoryMap, DescriptorSize);
  }

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Currently Protected Images---\n"));
    DumpImageRecords (&mImagePropertiesPrivate.ImageRecordList);
    );

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Memory Protection Special Regions---\n"));
    DumpMemoryProtectionSpecialRegions ();
    );

  // STEP 12: Break up the Memory Map Based on Loaded Images
  //
  // Because images are loaded into memory with a single type, descriptors
  // need to be broken up so that image sections are isolated to their
  // own descriptors. See STEP 5 for examples.
  //
  // After the following function, image sections containing CODE
  // will have the EFI_MEMORY_RO attribute set and image sections
  // containing DATA will have the EFI_MEMORY_XP attribute set.
  // These attributes will be cleared for nonprotected images
  // in STEP 17.
  {
    SplitTable (
      MemoryMapSize,
      *MemoryMap,
      *DescriptorSize,
      MergedImageList,
      AdditionalRecordsForImages
      );
  }

  // STEP 13: Break up the Memory Map Based on Special Regions
  //
  // Memory Protection Special Regions describe sections of memory
  // which should have their access attributes set to a specific
  // value. This functionality is used when it is critical for
  // regions to have specific attributes which may deviate from
  // the platform protection policy or which are not configurable
  // by the policy knobs.
  {
    if (mSpecialMemoryRegionsPrivate.Count > 0) {
      Status = SeparateSpecialRegionsInMemoryMap (
                 MemoryMapSize,
                 *MemoryMap,
                 DescriptorSize,
                 &FinalMemoryMapBufferSize,
                 &mSpecialMemoryRegionsPrivate.SpecialRegionList
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto Cleanup;
      }
    }
  }

  // STEP 14: Update the Bitmap Variables to Isolate Used Descriptors
  //
  // STEP 8 ensured a pool large was allocated to handle the worst case
  // scenario. Now that the memory map has been broken up, the bitmap
  // variables need to be updated to reflect the number of descriptors
  // which are actually in use.
  {
    NumMemoryMapDescriptors = *MemoryMapSize / *DescriptorSize;
    NumBitmapEntries        = (NumMemoryMapDescriptors % 8) == 0 ? NumMemoryMapDescriptors : (((NumMemoryMapDescriptors / 8) * 8) + 8);

    // Set the extra bits
    if ((NumMemoryMapDescriptors % 8) != 0) {
      mBitmapGlobal[NumMemoryMapDescriptors / 8] |= ~((1 << (NumMemoryMapDescriptors % 8)) - 1);
    }
  }

  // STEP 15: Sync the Bitmap with the Memory Map
  //
  // The bitmap is used to track which descriptors have been updated
  // with access attributes. This function will set the bits in the
  // bitmap which correspond to descriptors which have nonzero attributes so
  // they are not updated again based on memory type.
  //
  // After SyncBitmap(), Descriptors covering special regions will have their
  // virtual address set to SPECIAL_REGION_PATTERN so they can be identified
  // in subsequent steps.
  {
    Status = SyncBitmap (MemoryMapSize, *MemoryMap, DescriptorSize, mBitmapGlobal);
    ASSERT_EFI_ERROR (Status);
  }

  // STEP 16: Restore the Non-Protected Image List
  //
  // If the non-protected image list was merged with the protected image list
  // in STEP 4 to create a unified list, split the list back into its original
  // state.
  {
    if (mArrayOfListEntryPointers != NULL) {
      Status = OrderedInsertArrayUint64Comparison (
                 &mNonProtectedImageRangesPrivate.NonProtectedImageList,
                 mArrayOfListEntryPointers,
                 mNonProtectedImageRangesPrivate.NonProtectedImageCount,
                 OFFSET_OF (IMAGE_PROPERTIES_RECORD, ImageBase) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
                 OFFSET_OF (IMAGE_PROPERTIES_RECORD, Signature) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
                 IMAGE_PROPERTIES_RECORD_SIGNATURE
                 );

      ASSERT_EFI_ERROR (Status);
    }
  }

  // STEP 17: Remove Access Attributes from Non-Protected Images
  //
  // A side effect of STEP 12 was all image section descriptors were given
  // access attributes. This step will clear the attributes for non-protected
  // image regions.
  {
    if (mNonProtectedImageRangesPrivate.NonProtectedImageCount > 0) {
      Status = RemoveAttributesOfNonProtectedImageRanges (
                 MemoryMapSize,
                 *MemoryMap,
                 DescriptorSize,
                 &mNonProtectedImageRangesPrivate.NonProtectedImageList
                 );
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        goto Cleanup;
      }
    }
  }

  // STEP 18: Set the Remaining Access Attributes of the Memory Map
  //
  // Set the access attributes of descriptor ranges which have not been checked
  // against our memory protection policy (determined by the bitmap).
  {
    Status = SetAccessAttributesInMemoryMap (MemoryMapSize, *MemoryMap, DescriptorSize, mBitmapGlobal);
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Final Bitmap---\n"));
    DumpBitmap (mBitmapGlobal, NumBitmapEntries);
    );

  // STEP 19: Ensure Every Bit in the Bitmap is Set
  //
  // Every bit in the bitmap should be set at this point.
  {
    for (BitmapIndex = 0; BitmapIndex < NumBitmapEntries; BitmapIndex++) {
      ASSERT (IS_BITMAP_INDEX_SET (mBitmapGlobal, BitmapIndex));
    }
  }

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Memory Map with Populated Access Attributes---\n"));
    DumpMemoryMap (MemoryMapSize, *MemoryMap, DescriptorSize);
    DEBUG ((DEBUG_INFO, "---------------------------------------\n"));
    );

  return Status;

  // This point should only be reached if there was an error.
Cleanup:
  if (*MemoryMap != NULL) {
    FreePool (*MemoryMap);
  }

  if (mArrayOfListEntryPointers != NULL) {
    Status = OrderedInsertArrayUint64Comparison (
               &mNonProtectedImageRangesPrivate.NonProtectedImageList,
               mArrayOfListEntryPointers,
               mNonProtectedImageRangesPrivate.NonProtectedImageCount,
               OFFSET_OF (IMAGE_PROPERTIES_RECORD, ImageBase) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
               OFFSET_OF (IMAGE_PROPERTIES_RECORD, Signature) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
               IMAGE_PROPERTIES_RECORD_SIGNATURE
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (mArrayOfListEntryPointers);
  }

  *MemoryMapSize  = 0;
  *DescriptorSize = 0;

  return Status;
}

/**
  Frees the memory allocated by GetMemoryMapWithPopulatedAccessAttributes().

  @param[in]     MemoryMap               A pointer to the buffer containing the memory map
                                          with EFI access attributes which should be applied
**/
VOID
EFIAPI
CleanupMemoryMapWithPopulatedAccessAttributes (
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap
  )
{
  if (mArrayOfListEntryPointers != NULL) {
    FreePool (mArrayOfListEntryPointers);
  }

  if (mBitmapGlobal != NULL) {
    FreePool (mBitmapGlobal);
  }

  if (MemoryMap != NULL) {
    FreePool (MemoryMap);
  }
}

/**
  Protect UEFI PE/COFF image (Project Mu Version).

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol

  @retval     EFI_INVALID_PARAMETER   This function was called in SMM or the image
                                      type has an undefined protection policy
  @retval     EFI_OUT_OF_RESOURCES    Failure to Allocate()
  @retval     EFI_UNSUPPORTED         Image type will not be protected in accordance with memory
                                      protection policy settings
  @retval     EFI_LOAD_ERROR          The image is unaligned or the code segment count is zero
  @retval     EFI_SUCCESS             The image was successfully protected or the protection policy
                                      is PROTECT_IF_ALIGNED_ELSE_ALLOW
**/
EFI_STATUS
EFIAPI
ProtectUefiImageMu (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath
  )
{
  EFI_STATUS               Status           = EFI_SUCCESS;
  IMAGE_PROPERTIES_RECORD  *ImageRecord     = NULL;
  UINT32                   ProtectionPolicy = 0;
  UINT32                   RequiredAlignment;

  DEBUG ((DEBUG_INFO, "%a - 0x%x\n", __FUNCTION__, LoadedImage));
  DEBUG ((DEBUG_INFO, "  - 0x%016lx - 0x%016lx\n", (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase, LoadedImage->ImageSize));

  ProtectionPolicy = GetUefiImageProtectionPolicy (LoadedImage, LoadedImageDevicePath);
  switch (ProtectionPolicy) {
    case DO_NOT_PROTECT:
      ClearAccessAttributesFromMemoryRange (
        (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase,
        ALIGN_VALUE ((UINTN)LoadedImage->ImageSize, EFI_PAGE_SIZE)
        );
      CreateNonProtectedImagePropertiesRecord ((EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase, LoadedImage->ImageSize);
      return EFI_SUCCESS;
    case PROTECT_IF_ALIGNED_ELSE_ALLOW:
    case PROTECT_ELSE_RAISE_ERROR:
      break;
    default:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }

  ImageRecord = AllocateZeroPool (sizeof (*ImageRecord));

  if (ImageRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  }

  if (!EFI_ERROR (Status)) {
    RequiredAlignment = GetMemoryProtectionSectionAlignment (LoadedImage->ImageCodeType);
    // Create a new image properties record
    Status = CreateImagePropertiesRecord (LoadedImage->ImageBase, LoadedImage->ImageSize, &RequiredAlignment, ImageRecord);

    if (!EFI_ERROR (Status)) {
      // Record the image record in the list so we can undo the protections later
      Status = OrderedInsertUint64Comparison (
                 &mImagePropertiesPrivate.ImageRecordList,
                 &ImageRecord->Link,
                 OFFSET_OF (IMAGE_PROPERTIES_RECORD, ImageBase) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
                 OFFSET_OF (IMAGE_PROPERTIES_RECORD, Signature) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
                 IMAGE_PROPERTIES_RECORD_SIGNATURE
                 );
      ASSERT_EFI_ERROR (Status);

      mImagePropertiesPrivate.ImageRecordCount++;

      // When breaking up the memory map to include image code/data ranges, we need
      // to know the maximum number of code segments a single image will have
      if (mImagePropertiesPrivate.CodeSegmentCountMax < ImageRecord->CodeSegmentCount) {
        mImagePropertiesPrivate.CodeSegmentCountMax = ImageRecord->CodeSegmentCount;
      }

      // if gCpu is NULL, this image will be protected when CPU Arch is installed
      if (gCpu != NULL) {
        SetUefiImageProtectionAttributes (ImageRecord);
      }

      return EFI_SUCCESS;
    }
  }

  if (ImageRecord != NULL) {
    FreePool (ImageRecord);
  }

  if ((ProtectionPolicy == PROTECT_IF_ALIGNED_ELSE_ALLOW)) {
    ClearAccessAttributesFromMemoryRange (
      (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase,
      ALIGN_VALUE ((UINTN)LoadedImage->ImageSize, EFI_PAGE_SIZE)
      );
    CreateNonProtectedImagePropertiesRecord ((EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase, LoadedImage->ImageSize);
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Unprotect UEFI image (Project Mu Version).

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol
**/
VOID
UnprotectUefiImageMu (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  LIST_ENTRY               *ImageRecordLink;

  for (ImageRecordLink = mImagePropertiesPrivate.ImageRecordList.ForwardLink;
       ImageRecordLink != &mImagePropertiesPrivate.ImageRecordList;
       ImageRecordLink = ImageRecordLink->ForwardLink)
  {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );

    if (ImageRecord->ImageBase == (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase) {
      mImagePropertiesPrivate.ImageRecordCount--;
      goto Free;
    }
  }

  for (ImageRecordLink = mNonProtectedImageRangesPrivate.NonProtectedImageList.ForwardLink;
       ImageRecordLink != &mNonProtectedImageRangesPrivate.NonProtectedImageList;
       ImageRecordLink = ImageRecordLink->ForwardLink)
  {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );

    if (ImageRecord->ImageBase == (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase) {
      mNonProtectedImageRangesPrivate.NonProtectedImageCount--;
      goto Free;
    }
  }

  return;

Free:
  if (gCpu != NULL) {
    SetUefiImageMemoryAttributes (
      ImageRecord->ImageBase,
      ImageRecord->ImageSize,
      0
      );
  }

  // DeleteImagePropertiesRecord() will remove the record from the global list
  DeleteImagePropertiesRecord (ImageRecord);
  return;
}

/**
  Remove execution permissions from all regions whose type is identified by
  the NX Protection Policy, set appropriate attributes to image memory
  based on the image protection policy, and set the stack guard.
**/
VOID
EFIAPI
InitializePageAttributesForMemoryProtectionPolicy (
  VOID
  )
{
  UINTN                      MemoryMapSize;
  UINTN                      DescriptorSize;
  EFI_MEMORY_DESCRIPTOR      *MemoryMap;
  EFI_MEMORY_DESCRIPTOR      *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR      *MemoryMapEnd;
  EFI_STATUS                 Status;
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;
  EFI_PHYSICAL_ADDRESS       StackBase;

  // Get the EFI memory map.
  MemoryMapSize = 0;
  MemoryMap     = NULL;

  Status = GetMemoryMapWithPopulatedAccessAttributes (
             &MemoryMapSize,
             &MemoryMap,
             &DescriptorSize
             );

  ASSERT_EFI_ERROR (Status);

  if (MemoryMap != NULL) {
    // Merge contiguous entries with the same attributes to reduce the number
    // of calls to SetUefiImageMemoryAttributes()
    MergeMemoryMapByAttribute (MemoryMap, &MemoryMapSize, &DescriptorSize);
  }

  StackBase = 0;
  if (gDxeMps.CpuStackGuard) {
    // Get the base of stack from Hob.
    Hob.Raw = GetHobList ();
    while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
      MemoryHob = Hob.MemoryAllocation;
      if (CompareGuid (&gEfiHobMemoryAllocStackGuid, &MemoryHob->AllocDescriptor.Name)) {
        DEBUG ((
          DEBUG_INFO,
          "%a: StackBase = 0x%016lx  StackSize = 0x%016lx\n",
          __FUNCTION__,
          MemoryHob->AllocDescriptor.MemoryBaseAddress,
          MemoryHob->AllocDescriptor.MemoryLength
          ));

        StackBase = MemoryHob->AllocDescriptor.MemoryBaseAddress;

        // Ensure the base of the stack is page-size aligned.
        ASSERT ((StackBase & EFI_PAGE_MASK) == 0);
        break;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
    }

    // Ensure the base of stack can be found from Hob when stack guard is enabled.
    ASSERT (StackBase != 0);
  }

  if (EFI_ERROR (Status)) {
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: applying strict permissions to active memory regions\n",
    __FUNCTION__
    ));

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    if (MemoryMapEntry->Attribute != 0) {
      SetUefiImageMemoryAttributes (
        MemoryMapEntry->PhysicalStart,
        LShiftU64 (MemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT),
        MemoryMapEntry->Attribute
        );
    }

    // Add EFI_MEMORY_RP attribute for the first page of the stack if stack
    // guard is enabled.
    if (gDxeMps.CpuStackGuard &&
        (StackBase != 0) &&
        ((StackBase >= MemoryMapEntry->PhysicalStart) &&
         (StackBase <  MemoryMapEntry->PhysicalStart +
          LShiftU64 (MemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT))))
    {
      SetUefiImageMemoryAttributes (
        StackBase,
        EFI_PAGES_TO_SIZE (1),
        EFI_MEMORY_RP | MemoryMapEntry->Attribute
        );
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  CleanupMemoryMapWithPopulatedAccessAttributes (MemoryMap);
}

/**
  A notification for CPU_ARCH protocol.

  @param[in]  Event                 Event whose notification function is being invoked.
  @param[in]  Context               Pointer to the notification function's context,
                                    which is implementation-dependent.

**/
VOID
EFIAPI
MemoryProtectionCpuArchProtocolNotifyMu (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if (gCpu == NULL) {
    goto Done;
  }

  //
  // Call notify function meant for Heap Guard.
  //
  HeapGuardCpuArchProtocolNotify ();

Done:
  CoreCloseEvent (Event);
}

/**
  A notification for the Memory Attribute Protocol.

  @param[in]  Event                 Event whose notification function is being invoked.
  @param[in]  Context               Pointer to the notification function's context,
                                    which is implementation-dependent.

**/
VOID
EFIAPI
MemoryAttributeProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&mMemoryAttributeProtocol);

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a - Unable to locate the memory attribute protocol! Status = %r\n",
      __FUNCTION__,
      Status
      ));
  }

  CoreCloseEvent (Event);
}

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
  )
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Desc;
  EFI_STATUS                       Status;

  if (gCpu == NULL) {
    return EFI_NOT_READY;
  }

  if (((VOID *)((UINTN)BaseAddress) == NULL) || (Length == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = CoreGetMemorySpaceDescriptor (
             BaseAddress,
             &Desc
             );

  if (!EFI_ERROR (Status)) {
    SetUefiImageMemoryAttributes (
      BaseAddress,
      Length,
      0
      );
  }

  return Status;
}

/**
  Fetches a pointer to the DXE memory protection settings HOB.
**/
DXE_MEMORY_PROTECTION_SETTINGS *
EFIAPI
GetDxeMemoryProtectionSettings (
  VOID
  )
{
  VOID  *Ptr;

  Ptr = GetFirstGuidHob (&gDxeMemoryProtectionSettingsGuid);
  if (Ptr != NULL) {
    if (*((UINT8 *)GET_GUID_HOB_DATA (Ptr)) == (UINT8)DXE_MEMORY_PROTECTION_SETTINGS_CURRENT_VERSION) {
      return (DXE_MEMORY_PROTECTION_SETTINGS *)GET_GUID_HOB_DATA (Ptr);
    }
  }

  return NULL;
}

/**
  Enable NULL pointer detection by changing the attributes of page 0. The assumption is that PEI
  has set page zero to allocated so this operation can be done safely.

  @retval EFI_SUCCESS       Page zero successfully marked as read protected
  @retval Other             Page zero could not be marked as read protected

**/
VOID
EFIAPI
EnableNullDetection (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Desc;

  Status = CoreGetMemorySpaceDescriptor (0, &Desc);

  if (EFI_ERROR (Status)) {
    return;
  }

  if ((Desc.Capabilities & EFI_MEMORY_RP) == 0) {
    Status = CoreSetMemorySpaceCapabilities (
               0,
               EFI_PAGES_TO_SIZE (1),
               Desc.Capabilities | EFI_MEMORY_RP
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return;
    }
  }

  Status = CoreSetMemorySpaceAttributes (
             0,
             EFI_PAGES_TO_SIZE (1),
             Desc.Attributes | EFI_MEMORY_RP
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Disable NULL pointer detection.
**/
VOID
DisableNullDetection (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Desc;

  DEBUG ((DEBUG_INFO, "%a - Enter\n", __func__));

  Status = CoreGetMemorySpaceDescriptor (0, &Desc);

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a - Failed to get memory space descriptor for NULL address! Status = %r\n",
      __func__,
      Status
      ));
    return;
  }

  // Only re-enable the null page if it is system memory. If this page belongs to
  // another memory type or is unmapped in general, leave it RP
  if (Desc.GcdMemoryType != EfiGcdMemoryTypeSystemMemory) {
    DEBUG ((
      DEBUG_WARN,
      "%a - Not disabling null detection as page 0 is not marked as system memory\n",
      __func__
      ));
    return;
  }

  Status = CoreSetMemorySpaceAttributes (
             0,
             EFI_PAGE_SIZE,
             Desc.Attributes & ~EFI_MEMORY_RP
             );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "%a - Exit\n", __func__));

  return;
}

/**
  Disable NULL pointer detection after EndOfDxe. This is a workaround resort in
  order to skip unfixable NULL pointer access issues detected in OptionROM or
  boot loaders.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
DisableNullDetectionCallback (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  DisableNullDetection ();
  CoreCloseEvent (Event);
  return;
}

/**
  Uninstalls the Memory Attribute Protocol from all handles.
**/
VOID
EFIAPI
UninstallMemoryAttributeProtocol (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  UINTN       Index;
  EFI_HANDLE  *HandleBuffer;

  if (mMemoryAttributeProtocol == NULL) {
    Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&mMemoryAttributeProtocol);
    if (EFI_ERROR (Status)) {
      return;
    }
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiMemoryAttributeProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->UninstallProtocolInterface (
                      HandleBuffer[Index],
                      &gEfiMemoryAttributeProtocolGuid,
                      mMemoryAttributeProtocol
                      );
      ASSERT_EFI_ERROR (Status);
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }
}

/**
  Maps memory below 640K (legacy BIOS write-back memory) as readable, writeable, and executable.
**/
STATIC
VOID
MapLegacyBiosMemoryRWX (
  VOID
  )
{
  EFI_STATUS                       Status = EFI_SUCCESS;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Desc;
  EFI_PHYSICAL_ADDRESS             Start = 0x0;
  UINT64                           Length;
  UINT64                           DescLengthFromStart;

  if (gCpu == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a cannot remap Legacy BIOS memory as RWX because gCpu is NULL\n",
      __func__
      ));
    return;
  }

  // Ensure that this memory is marked as system memory. If it is not system memory, do not change
  // the memory attributes as we do not want to map something that shouldn't be mapped or map something
  // incorrectly
  while (Start < LEGACY_BIOS_WB_LENGTH) {
    Status = CoreGetMemorySpaceDescriptor (Start, &Desc);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - Failed to get memory space descriptor for address 0x%llx! Status = %r\n",
        __func__,
        Start,
        Status
        ));
      return;
    }

    // find the length from Start to the end of this descriptor, in the case Start != Desc.BaseAddress
    // these should all be well formed descriptors, but use SafeIntLib functions just to be sure we don't
    // over/underflow
    Status = SafeUint64Add (Desc.BaseAddress, Desc.Length, &DescLengthFromStart);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - Memory space descriptor has UINT64 overflowing address 0x%llx + length 0x%llx! Status = %r\n",
        __func__,
        Desc.BaseAddress,
        Desc.Length,
        Status
        ));

      // if we fail here this is very bad and means the GCD is malformed
      ASSERT (FALSE);
      return;
    }

    Status = SafeUint64Sub (DescLengthFromStart, Start, &DescLengthFromStart);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - Memory space descriptor has UINT64 underflowing address 0x%llx + length 0x%llx Start: 0x%llx! Status = %r\n",
        __func__,
        Desc.BaseAddress,
        Desc.Length,
        Start,
        Status
        ));

      // if we fail here this is very bad and means the GCD is malformed
      ASSERT (FALSE);
      return;
    }

    // Ensure we only go up to LEGACY_BIOS_WB_LENGTH here, we know Start is less than it due to while condition
    // We also know Start + DescLengthFromStart won't overflow because above we did a safe subtraction of Start from
    // DescLengthFromStart
    if (Start + DescLengthFromStart > LEGACY_BIOS_WB_LENGTH) {
      Length = LEGACY_BIOS_WB_LENGTH - Start;
    } else {
      Length = DescLengthFromStart;
    }

    // remove this chunk from being remapped if it is not system memory
    if (Desc.GcdMemoryType != EfiGcdMemoryTypeSystemMemory) {
      DEBUG ((
        DEBUG_WARN,
        "%a Not mapping 0x%llx for 0x%llx as RWX because it is not system memory\n",
        __func__,
        Desc.BaseAddress,
        Length
        ));

      // we know this doesn't overflow because we did a safe subtraction of Start from DescLengthFromStart above
      Start += DescLengthFromStart;
      continue;
    }

    // https://wiki.osdev.org/Memory_Map_(x86)
    //
    // Map the legacy BIOS write-back memory as RWX.
    Status = gCpu->SetMemoryAttributes (
                     gCpu,
                     Start,
                     Length,
                     0
                     );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a failed to map 0x%llx for length 0x%llx as RWX\n",
        __func__,
        Start,
        Length
        ));

      ASSERT_EFI_ERROR (Status);
    }

    // we know this doesn't overflow because we did a safe subtraction of Start from DescLengthFromStart above
    Start += DescLengthFromStart;
  }
}

/**
  Sets the NX compatibility global to FALSE so future checks to
  IsEnhancedMemoryProtectionActive() will return FALSE.
**/
VOID
EFIAPI
ActivateCompatibilityMode (
  VOID
  )
{
  if (!mEnhancedMemoryProtectionActive) {
    return;
  }

  DEBUG ((DEBUG_ERROR, "%a - Activating Memory Protection Compatibility Mode!\n", __FUNCTION__));

  mEnhancedMemoryProtectionActive = FALSE;

  DisableNullDetection ();
  UninstallMemoryAttributeProtocol ();
  MapLegacyBiosMemoryRWX ();
  CoreNotifySignalList (&gCompatibilityModeActivatedEventGuid);
}

/**
  Returns TRUE if ActivateCompatibilityMode() has never been called.
**/
BOOLEAN
EFIAPI
IsEnhancedMemoryProtectionActive (
  VOID
  )
{
  return mEnhancedMemoryProtectionActive;
}

/**
  Event function called when gEdkiiGcdSyncCompleteProtocolGuid is
  installed to initialize access attributes on tested and untested memory.
**/
VOID
EFIAPI
InitializePageAttributesCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   DisableNullDetectionEvent;

  // Inidcates that the GCD sync process has been completed. This BOOLEAN
  // must set this to TRUE so calls to ApplyMemoryProtectionPolicy() are not
  // blocked. This BOOLEAN also allows guard pages to be set.
  mGcdSyncComplete = TRUE;

  // Initialize paging attributes
  InitializePageAttributesForMemoryProtectionPolicy ();

  // Set all the guard pages
  HeapGuardCpuArchProtocolNotify ();

  if (gDxeMps.NullPointerDetectionPolicy.Fields.UefiNullDetection) {
    // Enable NULL pointer detection
    EnableNullDetection ();

    // Register for NULL pointer detection disabling if policy dictates
    if (gDxeMps.NullPointerDetectionPolicy.Fields.DisableEndOfDxe) {
      Status = CoreCreateEventEx (
                 EVT_NOTIFY_SIGNAL,
                 TPL_NOTIFY,
                 DisableNullDetectionCallback,
                 NULL,
                 &gEfiEndOfDxeEventGroupGuid,
                 &DisableNullDetectionEvent
                 );
    } else if (gDxeMps.NullPointerDetectionPolicy.Fields.DisableReadyToBoot) {
      Status = CoreCreateEventEx (
                 EVT_NOTIFY_SIGNAL,
                 TPL_NOTIFY,
                 DisableNullDetectionCallback,
                 NULL,
                 &gEfiEventReadyToBootGuid,
                 &DisableNullDetectionEvent
                 );
    }

    ASSERT_EFI_ERROR (Status);
  } else {
    // The NULL page may be EFI_MEMORY_RP in the page tables inherited
    // from PEI so clear the attribute now
    DisableNullDetection ();
  }

  CoreCloseEvent (Event);
}

/**
  Initialize Memory Protection support.
**/
VOID
EFIAPI
CoreInitializeMemoryProtectionMu (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;
  EFI_EVENT   Event;
  EFI_HANDLE  HgBmHandle = NULL;

  // Register an event to populate the memory attribute protocol
  Status = CoreCreateEvent (
             EVT_NOTIFY_SIGNAL,
             TPL_CALLBACK,
             MemoryAttributeProtocolNotify,
             NULL,
             &Event
             );
  ASSERT_EFI_ERROR (Status);

  // Register for protocol notification
  Status = CoreRegisterProtocolNotify (
             &gEfiMemoryAttributeProtocolGuid,
             Event,
             &Registration
             );
  ASSERT_EFI_ERROR (Status);

  // Register an event to initialize memory protection
  Status = CoreCreateEvent (
             EVT_NOTIFY_SIGNAL,
             TPL_CALLBACK,
             InitializePageAttributesCallback,
             NULL,
             &Event
             );
  ASSERT_EFI_ERROR (Status);

  // Register for protocol notification
  Status = CoreRegisterProtocolNotify (
             &gEdkiiGcdSyncCompleteProtocolGuid,
             Event,
             &Registration
             );
  ASSERT_EFI_ERROR (Status);

  // Register protocol for auditing memory protection (used by DxePagingAuditTestApp)
  if (gDxeMps.HeapGuardPolicy.Data ||
      gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromFv ||
      gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromUnknown)
  {
    Status = CoreInstallMultipleProtocolInterfaces (
               &HgBmHandle,
               &gMemoryProtectionDebugProtocolGuid,
               &mMemoryProtectionDebug,
               NULL
               );
    DEBUG ((DEBUG_INFO, "Installed gMemoryProtectionDebugProtocolGuid - %r\n", Status));
  }

  return;
}
