/** @file
  Functionality supporting the updated Project Mu memory protections

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

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

BOOLEAN                        mIsSystemNxCompatible    = TRUE;
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

/**
  Swap two image records.

  @param  FirstImageRecord   first image record.
  @param  SecondImageRecord  second image record.
**/
VOID
SwapImageRecord (
  IN IMAGE_PROPERTIES_RECORD  *FirstImageRecord,
  IN IMAGE_PROPERTIES_RECORD  *SecondImageRecord
  );

/**
  Swap two code sections in image record.

  @param  FirstImageRecordCodeSection    first code section in image record
  @param  SecondImageRecordCodeSection   second code section in image record
**/
VOID
SwapImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION  *FirstImageRecordCodeSection,
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION  *SecondImageRecordCodeSection
  );

/**
  Check if code section in image record is valid.

  @param  ImageRecord    image record to be checked

  @retval TRUE  image record is valid
  @retval FALSE image record is invalid
**/
BOOLEAN
IsImageRecordCodeSectionValid (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  );

/**
  Find image record according to image base and size.

  @param  ImageBase    Base of PE image
  @param  ImageSize    Size of PE image

  @return image record
**/
IMAGE_PROPERTIES_RECORD *
EFIAPI
FindImageRecord (
  IN EFI_PHYSICAL_ADDRESS  ImageBase,
  IN UINT64                ImageSize
  );

/**
  Converts a number of EFI_PAGEs to a size in bytes.

  NOTE: Do not use EFI_PAGES_TO_SIZE because it handles UINTN only.

  @param  Pages     The number of EFI_PAGES.

  @return  The number of bytes associated with the number of EFI_PAGEs specified
           by Pages.
**/
UINT64
EfiPagesToSize (
  IN UINT64  Pages
  );

/**
  Converts a size, in bytes, to a number of EFI_PAGESs.

  NOTE: Do not use EFI_SIZE_TO_PAGES because it handles UINTN only.

  @param  Size      A size in bytes.

  @return  The number of EFI_PAGESs associated with the number of bytes specified
           by Size.

**/
UINT64
EfiSizeToPages (
  IN UINT64  Size
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
  Free Image record.

  @param[in]  ImageRecord    A UEFI image record
**/
VOID
FreeImageRecord (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  );

/**
  Return if the PE image section is aligned.

  @param[in]  SectionAlignment    PE/COFF section alignment
  @param[in]  MemoryType          PE/COFF image memory type

  @retval TRUE  The PE image section is aligned.
  @retval FALSE The PE image section is not aligned.
**/
BOOLEAN
IsMemoryProtectionSectionAligned (
  IN UINT32           SectionAlignment,
  IN EFI_MEMORY_TYPE  MemoryType
  );

/**
  Set the memory map to new entries, according to one old entry,
  based upon PE code section and data section in image record

  @param  ImageRecord            An image record whose [ImageBase, ImageSize] covered
                                 by old memory map entry.
  @param  NewRecord              A pointer to several new memory map entries.
                                 The caller gurantee the buffer size be 1 +
                                 (SplitRecordCount * DescriptorSize) calculated
                                 below.
  @param  OldRecord              A pointer to one old memory map entry.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
UINTN
SetNewRecord (
  IN IMAGE_PROPERTIES_RECORD    *ImageRecord,
  IN OUT EFI_MEMORY_DESCRIPTOR  *NewRecord,
  IN EFI_MEMORY_DESCRIPTOR      *OldRecord,
  IN UINTN                      DescriptorSize
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
  Debug dumps the input list of IMAGE_PROPERTIES_RECORD_CODE_SECTION structs

  @param[in] ImageRecordCodeSectionList Head of the IMAGE_PROPERTIES_RECORD_CODE_SECTION list
**/
STATIC
VOID
DumpCodeSectionList (
  IN CONST LIST_ENTRY  *ImageRecordCodeSectionList
  )
{
  LIST_ENTRY                            *CodeSectionLink;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *CurrentImageRecord;

  if (ImageRecordCodeSectionList == NULL) {
    return;
  }

  CodeSectionLink = ImageRecordCodeSectionList->ForwardLink;

  while (CodeSectionLink != ImageRecordCodeSectionList) {
    CurrentImageRecord = CR (
                           CodeSectionLink,
                           IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                           Link,
                           IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                           );

    DEBUG ((
      DEBUG_INFO,
      "\tCode Section Memory Range 0x%llx - 0x%llx\n",
      CurrentImageRecord->CodeSegmentBase,
      CurrentImageRecord->CodeSegmentBase + CurrentImageRecord->CodeSegmentSize
      ));

    CodeSectionLink = CodeSectionLink->ForwardLink;
  }
}

/**
  Debug dumps the input list of IMAGE_PROPERTIES_RECORD structs

  @param[in] ImageRecordList Head of the IMAGE_PROPERTIES_RECORD list
**/
STATIC
VOID
DumpImageRecords (
  IN CONST LIST_ENTRY  *ImageRecordList
  )
{
  LIST_ENTRY               *ImageRecordLink;
  IMAGE_PROPERTIES_RECORD  *CurrentImageRecord;

  if (ImageRecordList == NULL) {
    return;
  }

  ImageRecordLink = ImageRecordList->ForwardLink;

  while (ImageRecordLink != ImageRecordList) {
    CurrentImageRecord = CR (
                           ImageRecordLink,
                           IMAGE_PROPERTIES_RECORD,
                           Link,
                           IMAGE_PROPERTIES_RECORD_SIGNATURE
                           );

    DEBUG ((
      DEBUG_INFO,
      "Image Record Memory Range 0x%llx - 0x%llx\n",
      CurrentImageRecord->ImageBase,
      CurrentImageRecord->ImageBase + CurrentImageRecord->ImageSize
      ));
    DumpCodeSectionList (&CurrentImageRecord->CodeSegmentList);
    ImageRecordLink = ImageRecordLink->ForwardLink;
  }
}

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
  max(EndOfAddressSpace, address + length of the final memory map entry). Gaps in the
  input EFI memory map which correlate with the non-existent GCD type will not be added
  to the map.

  @param[in, out] MemoryMapSize         Size, in bytes, of MemoryMap
  @param[in, out] MemoryMap             IN:  Pointer to the EFI memory map which will have all gaps filled. The
                                             original buffer will be freed and updated to a newly allocated buffer
                                        OUT: A sorted memory map describing the entire memory region
  @param[in]      DescriptorSize        Size, in bytes, of each descriptor region in the array
  @param[in]      StartOfAddressSpace   Starting address from which there should be contiguous entries
  @param[in]      EndOfAddressSpace     Ending address at which the memory map should at least reach

  @retval EFI_SUCCESS                   Successfully filled in the memory map
  @retval EFI_OUT_OF_RECOURCES          Failed to allocate pools
  @retval EFI_INVALID_PARAMETER         MemoryMap == NULL, *MemoryMap == NULL, *MemoryMapSize == 0, or
                                        DescriptorSize == 0
**/
EFI_STATUS
FillInMemoryMap (
  IN OUT    UINTN                            *MemoryMapSize,
  IN OUT    EFI_MEMORY_DESCRIPTOR            **MemoryMap,
  IN CONST  UINTN                            *DescriptorSize,
  IN CONST  UINTN                            *MemorySpaceMapDescriptorCount,
  IN        EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap,
  IN CONST  UINTN                            *MemorySpaceMapDescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *OldMemoryMapCurrent, *OldMemoryMapEnd, *NewMemoryMapStart, *NewMemoryMapCurrent;
  EFI_PHYSICAL_ADDRESS   LastEntryEnd, NextEntryStart, StartOfAddressSpace, EndOfAddressSpace;
  EFI_GCD_MEMORY_TYPE    GcdType = 0;
  UINT64                 RemainingLength, OverlapLength;
  UINTN                  NewMemoryMapSize, AdditionalEntriesCount, LoopIteration;

  if ((MemoryMap == NULL) || (*MemoryMap == NULL) ||
      (MemoryMapSize == NULL) || (*MemoryMapSize == 0) ||
      (*DescriptorSize == 0) || (MemorySpaceMap == NULL) ||
      (MemorySpaceMapDescriptorSize == NULL) || (MemorySpaceMapDescriptorCount == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  SortMemoryMap (*MemoryMap, *MemoryMapSize, *DescriptorSize);
  SortMemorySpaceMap (MemorySpaceMap, MemorySpaceMapDescriptorCount, MemorySpaceMapDescriptorSize);

  AdditionalEntriesCount = 0;
  NewMemoryMapSize       = 0;
  NewMemoryMapStart      = NULL;
  for (LoopIteration = 0; LoopIteration < 2; LoopIteration++) {
    StartOfAddressSpace = MemorySpaceMap[0].BaseAddress;
    EndOfAddressSpace   = MemorySpaceMap[*MemorySpaceMapDescriptorCount - 1].BaseAddress +
                          MemorySpaceMap[*MemorySpaceMapDescriptorCount - 1].Length;

    if (LoopIteration == 1) {
      NewMemoryMapSize = *MemoryMapSize + (AdditionalEntriesCount * *DescriptorSize);
      // Allocate a buffer for the new memory map
      NewMemoryMapStart = AllocatePool (NewMemoryMapSize);

      if (NewMemoryMapStart == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    NewMemoryMapCurrent = NewMemoryMapStart;
    OldMemoryMapCurrent = *MemoryMap;
    OldMemoryMapEnd     = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)*MemoryMap + *MemoryMapSize);

    // Check if we need to insert a new entry at the start of the memory map
    if (OldMemoryMapCurrent->PhysicalStart > StartOfAddressSpace) {
      do {
        OverlapLength   = OldMemoryMapCurrent->PhysicalStart - StartOfAddressSpace;
        RemainingLength = GetOverlappingMemorySpaceRegion (
                            MemorySpaceMap,
                            MemorySpaceMapDescriptorCount,
                            &StartOfAddressSpace,
                            &OverlapLength,
                            &GcdType
                            );
        if ((GcdType != EfiGcdMemoryTypeNonExistent)) {
          if ((LoopIteration == 1)) {
            POPULATE_MEMORY_DESCRIPTOR_ENTRY (
              NewMemoryMapCurrent,
              StartOfAddressSpace,
              EfiSizeToPages (OldMemoryMapCurrent->PhysicalStart - StartOfAddressSpace - RemainingLength),
              GcdTypeToEfiType (&GcdType)
              );

            NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize);
          } else {
            AdditionalEntriesCount++;
          }
        }

        StartOfAddressSpace = OldMemoryMapCurrent->PhysicalStart - RemainingLength;
      } while (RemainingLength > 0);
    }

    while (OldMemoryMapCurrent < OldMemoryMapEnd) {
      if (LoopIteration == 1) {
        CopyMem (NewMemoryMapCurrent, OldMemoryMapCurrent, *DescriptorSize);
      }

      if (NEXT_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, *DescriptorSize) < OldMemoryMapEnd) {
        LastEntryEnd   = OldMemoryMapCurrent->PhysicalStart + EfiPagesToSize (OldMemoryMapCurrent->NumberOfPages);
        NextEntryStart = NEXT_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, *DescriptorSize)->PhysicalStart;
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
              if ((LoopIteration == 1)) {
                NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize);
                POPULATE_MEMORY_DESCRIPTOR_ENTRY (
                  NewMemoryMapCurrent,
                  LastEntryEnd,
                  EfiSizeToPages (NextEntryStart - LastEntryEnd - RemainingLength),
                  GcdTypeToEfiType (&GcdType)
                  );
              } else {
                AdditionalEntriesCount++;
              }
            }

            LastEntryEnd = NextEntryStart - RemainingLength;
          } while (RemainingLength > 0);
        }
      }

      if (LoopIteration == 1) {
        NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize);
      }

      OldMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, *DescriptorSize);
    }

    LastEntryEnd = PREVIOUS_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, *DescriptorSize)->PhysicalStart +
                   EfiPagesToSize (PREVIOUS_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, *DescriptorSize)->NumberOfPages);

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
          if ((LoopIteration == 1)) {
            POPULATE_MEMORY_DESCRIPTOR_ENTRY (
              NewMemoryMapCurrent,
              LastEntryEnd,
              EfiSizeToPages (EndOfAddressSpace - LastEntryEnd - RemainingLength),
              GcdTypeToEfiType (&GcdType)
              );
            NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize);
          } else {
            AdditionalEntriesCount++;
          }
        }

        LastEntryEnd = EndOfAddressSpace - RemainingLength;
      } while (RemainingLength > 0);
    }
  }

  FreePool (*MemoryMap);
  *MemoryMap     = NewMemoryMapStart;
  *MemoryMapSize = NewMemoryMapSize;

  return EFI_SUCCESS;
}

// ---------------------------------------
//              CORE LOGIC
// ---------------------------------------

/**
  Find the first image record contained by the memory range Buffer -> Buffer + Length

  @param[in] Buffer           Start Address to check
  @param[in] Length           Length to check
  @param[in] ImageRecordList  A list of IMAGE_PROPERTIES_RECORD entries to check against
                              the memory range Buffer -> Buffer + Length

  @retval A pointer to the image properties record contained by the input buffer or NULL
**/
STATIC
IMAGE_PROPERTIES_RECORD *
GetImageRecordContainedByBuffer (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length,
  IN LIST_ENTRY            *ImageRecordList
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  LIST_ENTRY               *ImageRecordLink;

  for (ImageRecordLink = ImageRecordList->ForwardLink;
       ImageRecordLink != ImageRecordList;
       ImageRecordLink = ImageRecordLink->ForwardLink)
  {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );

    if ((Buffer <= ImageRecord->ImageBase) &&
        (Buffer + Length >= (ImageRecord->ImageBase + ImageRecord->ImageSize)))
    {
      return ImageRecord;
    }
  }

  return NULL;
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
    FreeImageRecord (ImageRecord);
  } else {
    mNonProtectedImageRangesPrivate.NonProtectedImageCount++;
  }

  return Status;
}

/**
  Creates and image properties record from a loaded PE image

  @param[in]  ImageBase               Base of PE image
  @param[in]  ImageSize               Size of PE image
  @param[in]  MemoryType              EFI memory type
  @param[in,out] ImageRecord          IN:  an allocated pool of length sizeof(IMAGE_PROPERTIES_RECORD)
                                      OUT: a populated image properties record

  @retval     EFI_INVALID_PARAMETER   This function was called in SMM or the image
                                      type has an undefined protection policy
  @retval     EFI_OUT_OF_RESOURCES    Failure to Allocate()
  @retval     EFI_UNSUPPORTED         Image type will not be protected in accordance with memory
                                      protection policy settings
  @retval     EFI_LOAD_ERROR          The image is unaligned or the code segment count is zero
  @retval     EFI_SUCCESS             The image was successfully protected or the protection policy
                                      is PROTECT_IF_ALIGNED_ELSE_ALLOW
**/
STATIC
EFI_STATUS
CreateImagePropertiesRecord (
  IN    VOID                     *ImageBase,
  IN    UINT64                   ImageSize,
  IN    EFI_MEMORY_TYPE          ImageCodeType,
  OUT   IMAGE_PROPERTIES_RECORD  *ImageRecord
  )
{
  EFI_STATUS                            Status = EFI_SUCCESS;
  VOID                                  *ImageAddress;
  CHAR8                                 *PdbPointer;
  EFI_IMAGE_DOS_HEADER                  *DosHdr;
  BOOLEAN                               IsAligned;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  EFI_IMAGE_SECTION_HEADER              *Section;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  UINTN                                 Index;
  UINT8                                 *Name;
  UINT32                                SectionAlignment;
  UINT32                                PeCoffHeaderOffset;

  DEBUG ((DEBUG_INFO, "%a - Enter...\n", __FUNCTION__));

  if ((ImageRecord == NULL) || (ImageBase == NULL)) {
    return EFI_OUT_OF_RESOURCES;
  }

  ImageRecord->Signature        = IMAGE_PROPERTIES_RECORD_SIGNATURE;
  ImageRecord->CodeSegmentCount = 0;
  InitializeListHead (&ImageRecord->CodeSegmentList);

  //
  // Step 1: record whole region
  //
  ImageRecord->ImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)ImageBase;
  ImageRecord->ImageSize = ImageSize;

  ImageAddress = ImageBase;

  PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
  if (PdbPointer != NULL) {
    DEBUG ((DEBUG_INFO, "  Image: %a\n", PdbPointer));
  }

  //
  // Check PE/COFF image
  //
  DosHdr             = (EFI_IMAGE_DOS_HEADER *)(UINTN)ImageAddress;
  PeCoffHeaderOffset = 0;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    PeCoffHeaderOffset = DosHdr->e_lfanew;
  }

  Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINT8 *)(UINTN)ImageAddress + PeCoffHeaderOffset);
  if (Hdr.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_VERBOSE, "Hdr.Pe32->Signature invalid - 0x%x\n", Hdr.Pe32->Signature));
    // It might be image in SMM.
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get SectionAlignment
  //
  if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    SectionAlignment = Hdr.Pe32->OptionalHeader.SectionAlignment;
  } else {
    SectionAlignment = Hdr.Pe32Plus->OptionalHeader.SectionAlignment;
  }

  IsAligned = IsMemoryProtectionSectionAligned (SectionAlignment, ImageCodeType);
  if (!IsAligned) {
    DEBUG ((
      DEBUG_ERROR,
      "%a - Section Alignment(0x%x) is incorrect!\n",
      __FUNCTION__,
      SectionAlignment
      ));
    return EFI_LOAD_ERROR;
  }

  Section = (EFI_IMAGE_SECTION_HEADER *)(
                                         (UINT8 *)(UINTN)ImageAddress +
                                         PeCoffHeaderOffset +
                                         sizeof (UINT32) +
                                         sizeof (EFI_IMAGE_FILE_HEADER) +
                                         Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                                         );
  for (Index = 0; Index < Hdr.Pe32->FileHeader.NumberOfSections; Index++) {
    Name = Section[Index].Name;
    DEBUG ((
      DEBUG_VERBOSE,
      "  Section - '%c%c%c%c%c%c%c%c'\n",
      Name[0],
      Name[1],
      Name[2],
      Name[3],
      Name[4],
      Name[5],
      Name[6],
      Name[7]
      ));

    //
    // Instead of assuming that a PE/COFF section of type EFI_IMAGE_SCN_CNT_CODE
    // can always be mapped read-only, classify a section as a code section only
    // if it has the executable attribute set and the writable attribute cleared.
    //
    // This adheres more closely to the PE/COFF spec, and avoids issues with
    // Linux OS loaders that may consist of a single read/write/execute section.
    //
    if ((Section[Index].Characteristics & (EFI_IMAGE_SCN_MEM_WRITE | EFI_IMAGE_SCN_MEM_EXECUTE)) == EFI_IMAGE_SCN_MEM_EXECUTE) {
      DEBUG ((DEBUG_VERBOSE, "  VirtualSize          - 0x%08x\n", Section[Index].Misc.VirtualSize));
      DEBUG ((DEBUG_VERBOSE, "  VirtualAddress       - 0x%08x\n", Section[Index].VirtualAddress));
      DEBUG ((DEBUG_VERBOSE, "  SizeOfRawData        - 0x%08x\n", Section[Index].SizeOfRawData));
      DEBUG ((DEBUG_VERBOSE, "  PointerToRawData     - 0x%08x\n", Section[Index].PointerToRawData));
      DEBUG ((DEBUG_VERBOSE, "  PointerToRelocations - 0x%08x\n", Section[Index].PointerToRelocations));
      DEBUG ((DEBUG_VERBOSE, "  PointerToLinenumbers - 0x%08x\n", Section[Index].PointerToLinenumbers));
      DEBUG ((DEBUG_VERBOSE, "  NumberOfRelocations  - 0x%08x\n", Section[Index].NumberOfRelocations));
      DEBUG ((DEBUG_VERBOSE, "  NumberOfLinenumbers  - 0x%08x\n", Section[Index].NumberOfLinenumbers));
      DEBUG ((DEBUG_VERBOSE, "  Characteristics      - 0x%08x\n", Section[Index].Characteristics));

      //
      // Step 2: record code section
      //
      ImageRecordCodeSection = AllocatePool (sizeof (IMAGE_PROPERTIES_RECORD_CODE_SECTION));
      if (ImageRecordCodeSection == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      ImageRecordCodeSection->Signature = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;

      ImageRecordCodeSection->CodeSegmentBase = (UINTN)ImageAddress + Section[Index].VirtualAddress;
      ImageRecordCodeSection->CodeSegmentSize = EfiPagesToSize (EfiSizeToPages (Section[Index].SizeOfRawData));

      OrderedInsertUint64Comparison (
        &ImageRecord->CodeSegmentList,
        &ImageRecordCodeSection->Link,
        OFFSET_OF (IMAGE_PROPERTIES_RECORD_CODE_SECTION, CodeSegmentBase) - OFFSET_OF (IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link),
        OFFSET_OF (IMAGE_PROPERTIES_RECORD_CODE_SECTION, Signature) - OFFSET_OF (IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link),
        IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
        );
      ImageRecord->CodeSegmentCount++;
    }
  }

  if (ImageRecord->CodeSegmentCount == 0) {
    //
    // If a UEFI executable consists of a single read+write+exec PE/COFF
    // section, the image can still be launched but image protection
    // cannot be applied.
    //
    // One example that elicits this is (some) Linux kernels (with the EFI stub
    // of course).
    //
    DEBUG ((DEBUG_WARN, "%a - CodeSegmentCount is 0!\n", __FUNCTION__));
    return EFI_LOAD_ERROR;
  }

  //
  // Check overlap all section in ImageBase/Size
  //
  if (!IsImageRecordCodeSectionValid (ImageRecord)) {
    DEBUG ((DEBUG_ERROR, "IsCodeSectionValid - FAIL\n"));
    return EFI_LOAD_ERROR;
  }

  //
  // Round up the ImageSize, some CPU arch may return EFI_UNSUPPORTED if ImageSize is not aligned.
  // Given that the loader always allocates full pages, we know the space after the image is not used.
  //
  ImageRecord->ImageSize = ALIGN_VALUE (ImageSize, EFI_PAGE_SIZE);

  return Status;
}

/**
  Split the memory map to new entries, according to one old entry,
  based upon PE code section and data section.

  @param[in]        OldRecord             A pointer to one old memory map entry.
  @param[in, out]   NewRecord             A pointer to several new memory map entries.
                                          The caller gurantee the buffer size be 1 +
                                          (SplitRecordCount * DescriptorSize) calculated
                                          below.
  @param[in]        MaxSplitRecordCount   The max number of splitted entries
  @param[in]        DescriptorSize        Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
  @param[in]        ImageRecordList       A list of IMAGE_PROPERTIES_RECORD entries used when searching
                                          for an image record contained by the memory range described in
                                          the existing EFI memory map descriptor OldRecord

  @retval  0 no entry is splitted.
  @return  the real number of splitted record.
**/
STATIC
UINTN
SplitMemoryDescriptor (
  IN EFI_MEMORY_DESCRIPTOR      *OldRecord,
  IN OUT EFI_MEMORY_DESCRIPTOR  *NewRecord,
  IN UINTN                      MaxSplitRecordCount,
  IN UINTN                      DescriptorSize,
  IN LIST_ENTRY                 *ImageRecordList
  )
{
  EFI_MEMORY_DESCRIPTOR    TempRecord;
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  IMAGE_PROPERTIES_RECORD  *NewImageRecord;
  UINT64                   PhysicalStart;
  UINT64                   PhysicalEnd;
  UINTN                    NewRecordCount;
  UINTN                    TotalNewRecordCount;

  if (MaxSplitRecordCount == 0) {
    CopyMem (NewRecord, OldRecord, DescriptorSize);
    return 0;
  }

  TotalNewRecordCount = 0;

  //
  // Override previous record
  //
  CopyMem (&TempRecord, OldRecord, sizeof (EFI_MEMORY_DESCRIPTOR));
  PhysicalStart = TempRecord.PhysicalStart;
  PhysicalEnd   = TempRecord.PhysicalStart + EfiPagesToSize (TempRecord.NumberOfPages);

  ImageRecord = NULL;
  do {
    NewImageRecord = GetImageRecordContainedByBuffer (PhysicalStart, PhysicalEnd - PhysicalStart, ImageRecordList);
    if (NewImageRecord == NULL) {
      //
      // No more images cover this range, check if we've reached the end of the old descriptor. If not,
      // add the remaining range to the new descriptor list.
      //
      if (PhysicalEnd > PhysicalStart) {
        NewRecord->Type          = TempRecord.Type;
        NewRecord->PhysicalStart = PhysicalStart;
        NewRecord->VirtualStart  = 0;
        NewRecord->NumberOfPages = EfiSizeToPages (PhysicalEnd - PhysicalStart);
        NewRecord->Attribute     = TempRecord.Attribute;
        TotalNewRecordCount++;
      }

      break;
    }

    ImageRecord = NewImageRecord;

    //
    // Update PhysicalStart to exclude the portion before the image buffer
    //
    if (TempRecord.PhysicalStart < ImageRecord->ImageBase) {
      NewRecord->Type          = TempRecord.Type;
      NewRecord->PhysicalStart = TempRecord.PhysicalStart;
      NewRecord->VirtualStart  = 0;
      NewRecord->NumberOfPages = EfiSizeToPages (ImageRecord->ImageBase - TempRecord.PhysicalStart);
      NewRecord->Attribute     = TempRecord.Attribute;
      TotalNewRecordCount++;

      PhysicalStart            = ImageRecord->ImageBase;
      TempRecord.PhysicalStart = PhysicalStart;
      TempRecord.NumberOfPages = EfiSizeToPages (PhysicalEnd - PhysicalStart);

      NewRecord = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)NewRecord + DescriptorSize);
    }

    //
    // Set new record
    //
    NewRecordCount       = SetNewRecord (ImageRecord, NewRecord, &TempRecord, DescriptorSize);
    TotalNewRecordCount += NewRecordCount;
    NewRecord            = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)NewRecord + (NewRecordCount * DescriptorSize));

    //
    // Update PhysicalStart, in order to exclude the image buffer which was already split.
    //
    PhysicalStart            = ImageRecord->ImageBase + ImageRecord->ImageSize;
    TempRecord.PhysicalStart = PhysicalStart;
    TempRecord.NumberOfPages = EfiSizeToPages (PhysicalEnd - PhysicalStart);
  } while ((ImageRecord != NULL) && (PhysicalStart < PhysicalEnd));

  //
  // This function will only be entered if we need to split a record, so ensure at least one split was made.
  //
  ASSERT (TotalNewRecordCount != 0);
  return TotalNewRecordCount - 1;
}

/**
  Return the max number of new splitted entries, according to one old entry,
  based upon PE code section and data section.

  @param[in]  OldRecord         A pointer to one old memory map entry.
  @param[in]  ImageRecordList   A list of IMAGE_PROPERTIES_RECORD entries used when searching
                                for an image record contained by the memory range described in
                                the existing EFI memory map descriptor OldRecord

  @retval  0 no entry needs to be split
  @return  the maximum number of new splitted entries
**/
STATIC
UINTN
GetMaximumRecordSplit (
  IN EFI_MEMORY_DESCRIPTOR  *OldRecord,
  IN LIST_ENTRY             *ImageRecordList
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  UINTN                    SplitRecordCount;
  UINT64                   PhysicalStart;
  UINT64                   PhysicalEnd;

  SplitRecordCount = 0;
  PhysicalStart    = OldRecord->PhysicalStart;
  PhysicalEnd      = OldRecord->PhysicalStart + EfiPagesToSize (OldRecord->NumberOfPages);

  do {
    ImageRecord = GetImageRecordContainedByBuffer (PhysicalStart, PhysicalEnd - PhysicalStart, ImageRecordList);
    if (ImageRecord == NULL) {
      break;
    }

    PhysicalStart     = ImageRecord->ImageBase + ImageRecord->ImageSize;
    SplitRecordCount += (2 * ImageRecord->CodeSegmentCount + 3);
  } while ((ImageRecord != NULL) && (PhysicalStart < PhysicalEnd));

  if (SplitRecordCount != 0) {
    SplitRecordCount--;
  }

  return SplitRecordCount;
}

/**
  Update the input memory map to add entries which describe PE code section and data sections for
  images within the described memory ranges. Within the updated memory map, image code sections
  can be identified by the attribute EFI_MEMORY_RP and image data sections can be identified
  by the attribute EFI_MEMORY_XP. The memory map will be sorted by base address.

  NOTE: This logic assumes PE code/data section are page-aligned

  @param[in, out] MemoryMapSize                   IN:   The size, in bytes, of the old memory map before the split.
                                                  OUT:  The size, in bytes, of the used descriptors of the split
                                                        memory map
  @param[in, out] MemoryMap                       IN:   A pointer to the buffer containing the current memory map.
                                                        This buffer must have enough space to accomodate the "worst case"
                                                        scenario where every image in ImageRecordList needs a new descriptor
                                                        to describe its code and data sections.
                                                  OUT:  A pointer to the updated memory map with separated image section
                                                        descriptors.
  @param[in]      DescriptorSize                  The size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
  @param[in]      ImageRecordList                 A list of IMAGE_PROPERTIES_RECORD entries used when searching
                                                  for an image record contained by the memory range described in
                                                  EFI memory map descriptors.
  @param[in]      NumberOfAdditionalDescriptors   The number of unused descriptors at the end of the input MemoryMap.
**/
VOID
SeparateImagesInMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize,
  IN     LIST_ENTRY             *ImageRecordList,
  IN     UINTN                  NumberOfAdditionalDescriptors
  )
{
  INTN   IndexOld;
  INTN   IndexNewCurrent;
  INTN   IndexNewStarting;
  UINTN  MaxSplitRecordCount;
  UINTN  RealSplitRecordCount;
  UINTN  TotalSkippedRecords;

  TotalSkippedRecords = 0;

  //
  // Let old record point to end of valid MemoryMap buffer.
  //
  IndexOld = ((*MemoryMapSize) / DescriptorSize) - 1;

  //
  // Let new record point to end of full MemoryMap buffer.
  //
  IndexNewCurrent  = ((*MemoryMapSize) / DescriptorSize) - 1 + NumberOfAdditionalDescriptors;
  IndexNewStarting = IndexNewCurrent;
  for ( ; IndexOld >= 0; IndexOld--) {
    MaxSplitRecordCount = GetMaximumRecordSplit ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + (IndexOld * DescriptorSize)), ImageRecordList);
    //
    // Split this MemoryMap record
    //
    IndexNewCurrent     -= MaxSplitRecordCount;
    RealSplitRecordCount = SplitMemoryDescriptor (
                             (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + (IndexOld * DescriptorSize)),
                             (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + (IndexNewCurrent * DescriptorSize)),
                             MaxSplitRecordCount,
                             DescriptorSize,
                             ImageRecordList
                             );

    // If we didn't utilize all the extra allocated descriptor slots, set the physical address of the unused slots
    // to MAX_ADDRESS so they are moved to the bottom of the list when sorting.
    for ( ; RealSplitRecordCount < MaxSplitRecordCount; RealSplitRecordCount++) {
      ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + ((IndexNewCurrent + RealSplitRecordCount + 1) * DescriptorSize)))->PhysicalStart = MAX_ADDRESS;
      TotalSkippedRecords++;
    }

    IndexNewCurrent--;
  }

  // Move all records to the beginning.
  CopyMem (
    MemoryMap,
    (UINT8 *)MemoryMap + ((IndexNewCurrent + 1) * DescriptorSize),
    (IndexNewStarting - IndexNewCurrent) * DescriptorSize
    );

  // Sort from low to high
  SortMemoryMap (
    MemoryMap,
    (IndexNewStarting - IndexNewCurrent) * DescriptorSize,
    DescriptorSize
    );

  // Update the memory map size to be the actual number of used records
  *MemoryMapSize = (IndexNewStarting - IndexNewCurrent - TotalSkippedRecords) * DescriptorSize;

  return;
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
      if ((MapEntryStart <= SpecialRegionStart) && (MapEntryEnd > SpecialRegionStart)) {
        // Check if some portion of the map entry isn't covered by the special region
        if (MapEntryStart != SpecialRegionStart) {
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
          MapEntryInsert->Attribute = SpecialRegionEntry->SpecialRegion.EfiAttributes;

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
  UINTN       AdditionalRecordCount, NumMemoryMapDescriptors, NumBitmapEntries, \
              NumMemorySpaceMapDescriptors, MemorySpaceMapDescriptorSize, MapKey, \
              ExpandedMemoryMapSize, BitmapIndex;
  UINT32                           DescriptorVersion;
  UINT8                            *Bitmap                    = NULL;
  EFI_MEMORY_DESCRIPTOR            *ExpandedMemoryMap         = NULL;
  LIST_ENTRY                       *MergedImageList           = NULL;
  LIST_ENTRY                       **ArrayOfListEntryPointers = NULL;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap            = NULL;

  if ((MemoryMapSize == NULL) || (MemoryMap == NULL) ||
      (*MemoryMap != NULL) || (DescriptorSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

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

  // Filter each map entry to only contain access attributes
  FilterMemoryMapAttributes (MemoryMapSize, *MemoryMap, DescriptorSize);

  Status = CoreGetMemorySpaceMap (&NumMemorySpaceMapDescriptors, &MemorySpaceMap);
  ASSERT_EFI_ERROR (Status);

  if (MemorySpaceMap != NULL) {
    MemorySpaceMapDescriptorSize = sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR);

    // Fill in the memory map with regions described in the GCD memory map but not the EFI memory map
    Status = FillInMemoryMap (
               MemoryMapSize,
               MemoryMap,
               DescriptorSize,
               &NumMemorySpaceMapDescriptors,
               MemorySpaceMap,
               &MemorySpaceMapDescriptorSize
               );

    ASSERT_EFI_ERROR (Status);
  }

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
  AdditionalRecordCount = ((2 * mImagePropertiesPrivate.CodeSegmentCountMax + 3) * mImagePropertiesPrivate.ImageRecordCount) +
                          (mNonProtectedImageRangesPrivate.NonProtectedImageCount * 3);

  ExpandedMemoryMapSize = (*MemoryMapSize * 2) + ((*DescriptorSize) * AdditionalRecordCount);
  ExpandedMemoryMap     = AllocatePool (ExpandedMemoryMapSize);

  if (ExpandedMemoryMap == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  CopyMem (ExpandedMemoryMap, *MemoryMap, *MemoryMapSize);
  FreePool (*MemoryMap);

  *MemoryMap = ExpandedMemoryMap;

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Currently Protected Images---\n"));
    DumpImageRecords (&mImagePropertiesPrivate.ImageRecordList);
    );

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Memory Protection Special Regions---\n"));
    DumpMemoryProtectionSpecialRegions ();
    );

  // Set MergedImageList to the list of IMAGE_PROPERTIES_RECORD entries which will
  // be used to breakup the memory map
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
                        &ArrayOfListEntryPointers,
                        OFFSET_OF (IMAGE_PROPERTIES_RECORD, ImageBase) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
                        OFFSET_OF (IMAGE_PROPERTIES_RECORD, Signature) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
                        IMAGE_PROPERTIES_RECORD_SIGNATURE
                        );

    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto Cleanup;
    }
  }

  // Break up the memory map so every image range divides evenly into some number of
  // EFI memory descriptors
  SeparateImagesInMemoryMap (
    MemoryMapSize,
    *MemoryMap,
    *DescriptorSize,
    MergedImageList,
    AdditionalRecordCount
    );

  // Update the separated memory map based on the special regions (if any)
  if (mSpecialMemoryRegionsPrivate.Count > 0) {
    Status = SeparateSpecialRegionsInMemoryMap (
               MemoryMapSize,
               *MemoryMap,
               DescriptorSize,
               &ExpandedMemoryMapSize,
               &mSpecialMemoryRegionsPrivate.SpecialRegionList
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto Cleanup;
    }
  }

  // Create a bitmap to track which descriptors have been updated with the correct attributes
  NumMemoryMapDescriptors = *MemoryMapSize / *DescriptorSize;
  NumBitmapEntries        = (NumMemoryMapDescriptors % 8) == 0 ? NumMemoryMapDescriptors : (((NumMemoryMapDescriptors / 8) * 8) + 8);
  Bitmap                  = AllocateZeroPool (NumBitmapEntries / 8);

  if (Bitmap == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT_EFI_ERROR (Status);
    goto Cleanup;
  }

  // Set the extra bits
  if ((NumMemoryMapDescriptors % 8) != 0) {
    Bitmap[NumMemoryMapDescriptors / 8] |= ~((1 << (NumMemoryMapDescriptors % 8)) - 1);
  }

  // Restore the nonprotected image list if it was merged with the protected
  // image list
  if (ArrayOfListEntryPointers != NULL) {
    Status = OrderedInsertArrayUint64Comparison (
               &mNonProtectedImageRangesPrivate.NonProtectedImageList,
               ArrayOfListEntryPointers,
               mNonProtectedImageRangesPrivate.NonProtectedImageCount,
               OFFSET_OF (IMAGE_PROPERTIES_RECORD, ImageBase) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
               OFFSET_OF (IMAGE_PROPERTIES_RECORD, Signature) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
               IMAGE_PROPERTIES_RECORD_SIGNATURE
               );

    ASSERT_EFI_ERROR (Status);
    FreePool (ArrayOfListEntryPointers);
  }

  // All image regions will now have nonzero attributes and special regions will have their
  // virtual address set to SPECIAL_REGION_PATTERN
  //
  // Set the bits in the bitmap to mark that the corresponding memory descriptor
  // has been set based on the memory protection policy and special regions
  Status = SyncBitmap (MemoryMapSize, *MemoryMap, DescriptorSize, Bitmap);
  ASSERT_EFI_ERROR (Status);

  // Remove the access attributes from descriptors which correspond to nonprotected images
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

  // Set the access attributes of descriptor ranges which have not been checked
  // against our memory protection policy
  Status = SetAccessAttributesInMemoryMap (MemoryMapSize, *MemoryMap, DescriptorSize, Bitmap);
  ASSERT_EFI_ERROR (Status);

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Final Bitmap---\n"));
    DumpBitmap (Bitmap, NumBitmapEntries);
    );

  // ASSERT if a bit in the bitmap is not set
  for (BitmapIndex = 0; BitmapIndex < NumBitmapEntries; BitmapIndex++) {
    ASSERT (IS_BITMAP_INDEX_SET (Bitmap, BitmapIndex));
  }

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Memory Map with Populated Access Attributes---\n"));
    DumpMemoryMap (MemoryMapSize, *MemoryMap, DescriptorSize);
    DEBUG ((DEBUG_INFO, "---------------------------------------\n"));
    );

  return Status;

Cleanup:
  if (*MemoryMap != NULL) {
    FreePool (*MemoryMap);
  }

  if (ArrayOfListEntryPointers != NULL) {
    Status = OrderedInsertArrayUint64Comparison (
               &mNonProtectedImageRangesPrivate.NonProtectedImageList,
               ArrayOfListEntryPointers,
               mNonProtectedImageRangesPrivate.NonProtectedImageCount,
               OFFSET_OF (IMAGE_PROPERTIES_RECORD, ImageBase) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
               OFFSET_OF (IMAGE_PROPERTIES_RECORD, Signature) - OFFSET_OF (IMAGE_PROPERTIES_RECORD, Link),
               IMAGE_PROPERTIES_RECORD_SIGNATURE
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (ArrayOfListEntryPointers);
  }

  *MemoryMapSize  = 0;
  *DescriptorSize = 0;

  return Status;
}

/**
  Copy an existing image properties record created by MemoryAttributesTable

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
CopyExistingPropertiesRecord (
  IN  EFI_PHYSICAL_ADDRESS     ImageBase,
  IN  UINT64                   ImageSize,
  OUT IMAGE_PROPERTIES_RECORD  *NewImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *NewImageRecordCodeSection;
  IMAGE_PROPERTIES_RECORD               *ExistingImageRecord;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ExistingImageRecordCodeSection;
  LIST_ENTRY                            *ExistingImageRecordCodeSectionLink;
  LIST_ENTRY                            *ExistingImageRecordCodeSectionEndLink;

  if (NewImageRecord == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // If the image is a runtime image, the properties record may already have been created in
  // MemoryAttributesTable.c, so fetch that record if it exists for performance
  ExistingImageRecord = FindImageRecord (ImageBase, ImageSize);

  if (ExistingImageRecord == NULL) {
    return EFI_NOT_FOUND;
  }

  NewImageRecord->Signature        = ExistingImageRecord->Signature;
  NewImageRecord->ImageBase        = ExistingImageRecord->ImageBase;
  NewImageRecord->ImageSize        = ExistingImageRecord->ImageSize;
  NewImageRecord->CodeSegmentCount = ExistingImageRecord->CodeSegmentCount;

  InitializeListHead (&NewImageRecord->CodeSegmentList);

  ExistingImageRecordCodeSectionLink    = GetFirstNode (&ExistingImageRecord->CodeSegmentList);
  ExistingImageRecordCodeSectionEndLink = &ExistingImageRecord->CodeSegmentList;

  while (ExistingImageRecordCodeSectionLink != ExistingImageRecordCodeSectionEndLink) {
    NewImageRecordCodeSection = AllocatePool (sizeof (*NewImageRecordCodeSection));

    if (NewImageRecordCodeSection == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ExistingImageRecordCodeSection = CR (
                                       ExistingImageRecordCodeSectionLink,
                                       IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                                       Link,
                                       IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                                       );

    NewImageRecordCodeSection->Signature       = ExistingImageRecordCodeSection->Signature;
    NewImageRecordCodeSection->CodeSegmentSize = EfiPagesToSize (EfiSizeToPages (ExistingImageRecordCodeSection->CodeSegmentSize));
    NewImageRecordCodeSection->CodeSegmentBase = ExistingImageRecordCodeSection->CodeSegmentBase;

    InsertTailList (&NewImageRecord->CodeSegmentList, &NewImageRecordCodeSection->Link);

    ExistingImageRecordCodeSectionLink = ExistingImageRecordCodeSectionLink->ForwardLink;
  }

  return EFI_SUCCESS;
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
    // If a record was already created for the memory attributes table, copy it
    Status = CopyExistingPropertiesRecord ((EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase, LoadedImage->ImageSize, ImageRecord);

    if (EFI_ERROR (Status)) {
      // Create a new image properties record
      Status = CreateImagePropertiesRecord (LoadedImage->ImageBase, LoadedImage->ImageSize, LoadedImage->ImageCodeType, ImageRecord);
    }

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

  // FreeImageRecord() will remove the record from the global list
  FreeImageRecord (ImageRecord);
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

      // Add EFI_MEMORY_RP attribute for the first page of the stack if stack
      // guard is enabled.
      if ((StackBase != 0) &&
          ((StackBase >= MemoryMapEntry->PhysicalStart) &&
           (StackBase <  MemoryMapEntry->PhysicalStart +
            LShiftU64 (MemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT))) &&
          gDxeMps.CpuStackGuard)
      {
        SetUefiImageMemoryAttributes (
          StackBase,
          EFI_PAGES_TO_SIZE (1),
          EFI_MEMORY_RP | MemoryMapEntry->Attribute
          );
      }
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  FreePool (MemoryMap);
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

  InitializePageAttributesForMemoryProtectionPolicy ();

  //
  // Call notify function meant for Heap Guard.
  //
  HeapGuardCpuArchProtocolNotify ();

Done:
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
  Sets the NX compatibility global to FALSE so future checks to
  IsSystemNxCompatible() will return FALSE.
**/
VOID
EFIAPI
TurnOffNxCompatibility (
  VOID
  )
{
  if (mIsSystemNxCompatible) {
    DEBUG ((DEBUG_INFO, "%a - Setting Nx on Code types to FALSE\n", __FUNCTION__));
  }

  mIsSystemNxCompatible = FALSE;
}

/**
  Returns TRUE if TurnOffNxCompatibility() has never been called.
**/
BOOLEAN
EFIAPI
IsSystemNxCompatible (
  VOID
  )
{
  return mIsSystemNxCompatible;
}
