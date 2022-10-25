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
EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttributeProtocol = NULL;

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
//         SPECIAL REGION LOGIC
// ---------------------------------------

/**
  Sorts the MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY list by Start

  @param[in] SpecialRegionList Head of the list to be sorted
**/
STATIC
VOID
SortMemoryProtectionSpecialRegionList (
  IN LIST_ENTRY  *SpecialRegionList
  )
{
  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY  *SpecialRegionEntry;
  MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY  *NextSpecialRegionEntry;
  MEMORY_PROTECTION_SPECIAL_REGION             TempSpecialRegion;
  LIST_ENTRY                                   *SpecialRegionEntryLink;
  LIST_ENTRY                                   *NextSpecialRegionEntryLink;
  LIST_ENTRY                                   *SpecialRegionEndLink;

  if (SpecialRegionList == NULL) {
    return;
  }

  SpecialRegionEntryLink     = SpecialRegionList->ForwardLink;
  NextSpecialRegionEntryLink = SpecialRegionEntryLink->ForwardLink;
  SpecialRegionEndLink       = SpecialRegionList;

  while (SpecialRegionEntryLink != SpecialRegionEndLink) {
    SpecialRegionEntry = CR (
                           SpecialRegionEntryLink,
                           MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY,
                           Link,
                           MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
                           );
    while (NextSpecialRegionEntryLink != SpecialRegionEndLink) {
      NextSpecialRegionEntry = CR (
                                 NextSpecialRegionEntryLink,
                                 MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY,
                                 Link,
                                 MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY_SIGNATURE
                                 );
      if (SpecialRegionEntry->SpecialRegion.Start > NextSpecialRegionEntry->SpecialRegion.Start) {
        // Temp = Current
        TempSpecialRegion.Start         = SpecialRegionEntry->SpecialRegion.Start;
        TempSpecialRegion.Length        = SpecialRegionEntry->SpecialRegion.Length;
        TempSpecialRegion.EfiAttributes = SpecialRegionEntry->SpecialRegion.EfiAttributes;

        // Current = Next
        SpecialRegionEntry->SpecialRegion.Start         = NextSpecialRegionEntry->SpecialRegion.Start;
        SpecialRegionEntry->SpecialRegion.Length        = NextSpecialRegionEntry->SpecialRegion.Length;
        SpecialRegionEntry->SpecialRegion.EfiAttributes = NextSpecialRegionEntry->SpecialRegion.EfiAttributes;

        // Next = Temp
        NextSpecialRegionEntry->SpecialRegion.Start         = TempSpecialRegion.Start;
        NextSpecialRegionEntry->SpecialRegion.Length        = TempSpecialRegion.Length;
        NextSpecialRegionEntry->SpecialRegion.EfiAttributes = TempSpecialRegion.EfiAttributes;
      }

      NextSpecialRegionEntryLink = NextSpecialRegionEntryLink->ForwardLink;
    }

    SpecialRegionEntryLink     = SpecialRegionEntryLink->ForwardLink;
    NextSpecialRegionEntryLink = SpecialRegionEntryLink->ForwardLink;
  }
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
    NewSpecialRegion = AllocateCopyPool (sizeof (MEMORY_PROTECTION_SPECIAL_REGION_LIST_ENTRY), HobSpecialRegion);

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
    InsertHeadList (&mSpecialMemoryRegionsPrivate.SpecialRegionList, &NewSpecialRegion->Link);
    mSpecialMemoryRegionsPrivate.Count++;

    GuidHob = GetNextGuidHob (&gMemoryProtectionSpecialRegionHobGuid, GET_NEXT_HOB (GuidHob));
  }

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

  SortMemoryProtectionSpecialRegionList (&mSpecialMemoryRegionsPrivate.SpecialRegionList);

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
  @retval   EFI_INVALID_PARAMTER  Length is zero
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
  InsertTailList (&mSpecialMemoryRegionsPrivate.SpecialRegionList, &SpecialRegionEntry->Link);
  mSpecialMemoryRegionsPrivate.Count++;

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
//     LINKED LIST SUPPORT FUNCTIONS
// ---------------------------------------

/**
  Inserts the input ImageRecordToInsertLink into ImageRecordList based on the IMAGE_PROPERTIES_RECORD.ImageBase field

  @param[in] ImageRecordList           Pointer to the head of the IMAGE_PROPERTIES_RECORD list
  @param[in] ImageRecordToInsertLink   Pointer to the list entry of the IMAGE_PROPERTIES_RECORD to insert

  @retval EFI_SUCCESS             IMAGE_PROPERTIES_RECORD inserted into the list
  @retval EFI_INVALID_PARAMETER   ImageRecordList or ImageRecordToInsertLink were NULL
**/
STATIC
EFI_STATUS
OrderedInsertImageRecordListEntry (
  IN LIST_ENTRY  *ImageRecordList,
  IN LIST_ENTRY  *ImageRecordToInsertLink
  )
{
  IMAGE_PROPERTIES_RECORD  *CurrentImageRecord;
  IMAGE_PROPERTIES_RECORD  *ImageRecordToInsert;
  LIST_ENTRY               *ImageRecordLink;
  LIST_ENTRY               *ImageRecordEndLink;
  EFI_PHYSICAL_ADDRESS     ImageRecordToInsertBase;

  if ((ImageRecordList == NULL) || (ImageRecordToInsertLink == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ImageRecordToInsert = CR (
                          ImageRecordToInsertLink,
                          IMAGE_PROPERTIES_RECORD,
                          Link,
                          IMAGE_PROPERTIES_RECORD_SIGNATURE
                          );
  ImageRecordToInsertBase = ImageRecordToInsert->ImageBase;

  ImageRecordLink    = ImageRecordList->ForwardLink;
  ImageRecordEndLink = ImageRecordList;
  while (ImageRecordLink != ImageRecordEndLink) {
    CurrentImageRecord = CR (
                           ImageRecordLink,
                           IMAGE_PROPERTIES_RECORD,
                           Link,
                           IMAGE_PROPERTIES_RECORD_SIGNATURE
                           );
    if (ImageRecordToInsertBase < CurrentImageRecord->ImageBase) {
      break;
    }

    ImageRecordLink = ImageRecordLink->ForwardLink;
  }

  ImageRecordToInsertLink->BackLink              = ImageRecordLink->BackLink;
  ImageRecordToInsertLink->ForwardLink           = ImageRecordLink;
  ImageRecordToInsertLink->BackLink->ForwardLink = ImageRecordToInsertLink;
  ImageRecordToInsertLink->ForwardLink->BackLink = ImageRecordToInsertLink;
  return EFI_SUCCESS;
}

/**
  Inserts the input CodeSectionToInsertLink into CodeSectionList based on the
  IMAGE_PROPERTIES_RECORD_CODE_SECTION.CodeSegmentBase field

  @param[in] CodeSectionList           Pointer to the head of the IMAGE_PROPERTIES_RECORD_CODE_SECTION list
  @param[in] CodeSectionToInsertLink   Pointer to the list entry of the IMAGE_PROPERTIES_RECORD_CODE_SECTION to insert

  @retval EFI_SUCCESS             IMAGE_PROPERTIES_RECORD_CODE_SECTION inserted into the list
  @retval EFI_INVALID_PARAMETER   CodeSectionList or CodeSectionToInsertLink were NULL
**/
STATIC
EFI_STATUS
OrderedInsertCodeSectionListEntry (
  IN LIST_ENTRY  *CodeSectionList,
  IN LIST_ENTRY  *CodeSectionToInsertLink
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *CurrentCodeSection;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *CodeSectionToInsert;
  LIST_ENTRY                            *CodeSectionLink;
  LIST_ENTRY                            *CodeSectionEndLink;
  EFI_PHYSICAL_ADDRESS                  CodeSectionToInsertBase;

  if ((CodeSectionList == NULL) || (CodeSectionToInsertLink == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CodeSectionToInsert = CR (
                          CodeSectionToInsertLink,
                          IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                          Link,
                          IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                          );

  CodeSectionToInsertBase = CodeSectionToInsert->CodeSegmentBase;

  CodeSectionLink    = CodeSectionList->ForwardLink;
  CodeSectionEndLink = CodeSectionList;
  while (CodeSectionLink != CodeSectionEndLink) {
    CurrentCodeSection = CR (
                           CodeSectionLink,
                           IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                           Link,
                           IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                           );
    if (CodeSectionToInsertBase < CurrentCodeSection->CodeSegmentBase) {
      break;
    }

    CodeSectionLink = CodeSectionLink->ForwardLink;
  }

  CodeSectionToInsertLink->BackLink              = CodeSectionLink->BackLink;
  CodeSectionToInsertLink->ForwardLink           = CodeSectionLink;
  CodeSectionToInsertLink->BackLink->ForwardLink = CodeSectionToInsertLink;
  CodeSectionToInsertLink->ForwardLink->BackLink = CodeSectionToInsertLink;
  return EFI_SUCCESS;
}

/**
  Merges every IMAGE_PROPERTIES_RECORD entry within ArrayOfListEntriesToBeMerged into ImagePropertiesRecordList

  @param[in] ImagePropertiesRecordList        Pointer to the head of a list of IMAGE_PROPERTIES_RECORD entries
                                              into which the input ArrayOfListEntriesToBeMerged will be merged
  @param[in] ArrayOfListEntriesToBeMerged     Pointer to an array of LIST_ENTRY* which will be merged
                                              into the input ImagePropertiesRecordList
  @param[in] ListToBeMergedCount              Number of LIST_ENTRY* which will be merged
                                              into the input ImagePropertiesRecordList

  @retval EFI_SUCCESS                         ArrayOfListEntriesToBeMerged was successfully merged into ImagePropertiesRecordList
  @retval EFI_INVALID_PARAMETER               ImagePropertiesRecordList was NULL        OR
                                              ArrayOfListEntriesToBeMerged was NULL     OR
                                              ArrayOfListEntriesToBeMerged[n] was NULL  OR
                                              ListToBeMergedCount was zero
**/
STATIC
EFI_STATUS
OrderedInsertImagePropertiesRecordArray (
  IN  LIST_ENTRY  *ImagePropertiesRecordList,
  IN  LIST_ENTRY  **ArrayOfListEntriesToBeMerged,
  IN  UINTN       ListToBeMergedCount
  )
{
  INTN  ListToBeMergedIndex = ListToBeMergedCount - 1;

  if ((ImagePropertiesRecordList == NULL) || (ArrayOfListEntriesToBeMerged == NULL) || (ListToBeMergedCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  // The input array should be sorted, so going backwards is the fastest method
  for ( ; ListToBeMergedIndex >= 0; --ListToBeMergedIndex) {
    if (ArrayOfListEntriesToBeMerged[ListToBeMergedIndex] == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    RemoveEntryList (ArrayOfListEntriesToBeMerged[ListToBeMergedIndex]);
    OrderedInsertImageRecordListEntry (ImagePropertiesRecordList, ArrayOfListEntriesToBeMerged[ListToBeMergedIndex]);
  }

  return EFI_SUCCESS;
}

/**
  Merges every LIST_ENTRY within ArrayOfListEntriesToBeMerged into ImagePropertiesRecordList

  @param[in]  ListToMergeInto                 Pointer to the head of a list of IMAGE_PROPERTIES_RECORD entries
                                              into which the input ListToBeMerged will be merged
  @param[in]  ListToBeMerged                  Pointer to the head of a list of IMAGE_PROPERTIES_RECORD entries
                                              which will be merged into ListToMergeInto
  @param[in]  ListToBeMergedCount             Number of IMAGE_PROPERTIES_RECORD entries in ListToBeMerged
  @param[out] ArrayOfListEntriesToBeMerged    Pointer to an allocated array of LIST_ENTRY* which were merged
                                              into the input ListToMergeInto. This array should be size
                                              ListToBeMergedCount * sizeof(LIST_ENTRY*)

  @retval EFI_SUCCESS                         ArrayOfListEntriesToBeMerged was successfully merged into
                                              ImagePropertiesRecordList
  @retval EFI_OUT_OF_RESOURCES                Failed to allocate memory
  @retval EFI_INVALID_PARAMETER               ListToMergeInto was NULL                  OR
                                              ListToBeMerged was NULL                   OR
                                              ArrayOfListEntriesToBeMerged was NULL     OR
                                              ListToBeMergedCount was zero
  @retval other                               Return value of OrderedInsertImageRecordListEntry()
**/
STATIC
EFI_STATUS
MergeImagePropertiesRecordLists (
  IN  LIST_ENTRY  *ListToMergeInto,
  IN  LIST_ENTRY  *ListToBeMerged,
  IN  UINTN       ListToBeMergedCount,
  OUT LIST_ENTRY  **ArrayOfMergedElements
  )
{
  UINTN       Index = 0;
  EFI_STATUS  Status;

  if ((ListToMergeInto == NULL) || (ListToBeMerged == NULL) ||
      (ArrayOfMergedElements == NULL) || (ListToBeMergedCount == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Insert each entry in the list to be merged into the
  while (!IsListEmpty (ListToBeMerged) && Index < ListToBeMergedCount) {
    ArrayOfMergedElements[Index] = ListToBeMerged->ForwardLink;
    RemoveEntryList (ArrayOfMergedElements[Index]);
    Status = OrderedInsertImageRecordListEntry (ListToMergeInto, ArrayOfMergedElements[Index++]);
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  // If we did not merge all elements of the list, unmerge them and free the input array
  if (!IsListEmpty (ListToBeMerged)) {
    OrderedInsertImagePropertiesRecordArray (ListToBeMerged, ArrayOfMergedElements, Index - 1);
    FreePool (*ArrayOfMergedElements);
    return Status;
  }

  return EFI_SUCCESS;
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
  max(EndOfAddressSpace, address + length of the final memory map entry)

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

  if ((MemoryMap == NULL) || (*MemoryMap == NULL) ||
      (MemoryMapSize == NULL) || (*MemoryMapSize == 0) ||
      (*DescriptorSize == 0) || (MemorySpaceMap == NULL) ||
      (MemorySpaceMapDescriptorSize == NULL) || (MemorySpaceMapDescriptorCount == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  SortMemoryMap (*MemoryMap, *MemoryMapSize, *DescriptorSize);
  SortMemorySpaceMap (MemorySpaceMap, MemorySpaceMapDescriptorCount, MemorySpaceMapDescriptorSize);

  StartOfAddressSpace = MemorySpaceMap[0].BaseAddress;
  EndOfAddressSpace   = MemorySpaceMap[*MemorySpaceMapDescriptorCount - 1].BaseAddress +
                        MemorySpaceMap[*MemorySpaceMapDescriptorCount - 1].Length;

  NewMemoryMapStart = NULL;

  // Double the size of the memory map for the worst case of every entry being non-contiguous
  NewMemoryMapStart = AllocatePool ((*MemoryMapSize * 2) + (*DescriptorSize * 2));

  if (NewMemoryMapStart == NULL) {
    return EFI_OUT_OF_RESOURCES;
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

      POPULATE_MEMORY_DESCRIPTOR_ENTRY (
        NewMemoryMapCurrent,
        StartOfAddressSpace,
        EfiSizeToPages (OldMemoryMapCurrent->PhysicalStart - StartOfAddressSpace - RemainingLength),
        GcdTypeToEfiType (&GcdType)
        );

      NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize);
      StartOfAddressSpace = OldMemoryMapCurrent->PhysicalStart - RemainingLength;
    } while (RemainingLength > 0);
  }

  while (OldMemoryMapCurrent < OldMemoryMapEnd) {
    CopyMem (NewMemoryMapCurrent, OldMemoryMapCurrent, *DescriptorSize);
    if (NEXT_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, *DescriptorSize) < OldMemoryMapEnd) {
      LastEntryEnd   = NewMemoryMapCurrent->PhysicalStart + EfiPagesToSize (NewMemoryMapCurrent->NumberOfPages);
      NextEntryStart = NEXT_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, *DescriptorSize)->PhysicalStart;
      // Check for a gap in the memory map
      if (NextEntryStart > LastEntryEnd) {
        // Fill in missing region based on the GCD Memory Map
        do {
          NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize);
          OverlapLength       = NextEntryStart - LastEntryEnd;
          RemainingLength     = GetOverlappingMemorySpaceRegion (
                                  MemorySpaceMap,
                                  MemorySpaceMapDescriptorCount,
                                  &LastEntryEnd,
                                  &OverlapLength,
                                  &GcdType
                                  );

          POPULATE_MEMORY_DESCRIPTOR_ENTRY (
            NewMemoryMapCurrent,
            LastEntryEnd,
            EfiSizeToPages (NextEntryStart - LastEntryEnd - RemainingLength),
            GcdTypeToEfiType (&GcdType)
            );
          LastEntryEnd = NextEntryStart - RemainingLength;
        } while (RemainingLength > 0);
      }
    }

    NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize);
    OldMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (OldMemoryMapCurrent, *DescriptorSize);
  }

  LastEntryEnd = PREVIOUS_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize)->PhysicalStart +
                 EfiPagesToSize (PREVIOUS_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize)->NumberOfPages);

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

      POPULATE_MEMORY_DESCRIPTOR_ENTRY (
        NewMemoryMapCurrent,
        LastEntryEnd,
        EfiSizeToPages (EndOfAddressSpace - LastEntryEnd - RemainingLength),
        GcdTypeToEfiType (&GcdType)
        );
      NewMemoryMapCurrent = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapCurrent, *DescriptorSize);
      LastEntryEnd        = EndOfAddressSpace - RemainingLength;
    } while (RemainingLength > 0);
  }

  // Re-use this stack variable as an intermediate to ensure we can allocate a buffer before updating the old memory map
  OldMemoryMapCurrent = AllocateCopyPool ((UINTN)((UINT8 *)NewMemoryMapCurrent - (UINT8 *)NewMemoryMapStart), NewMemoryMapStart);

  if (OldMemoryMapCurrent == NULL ) {
    FreePool (NewMemoryMapStart);
    return EFI_OUT_OF_RESOURCES;
  }

  FreePool (*MemoryMap);
  *MemoryMap = OldMemoryMapCurrent;

  *MemoryMapSize = (UINTN)((UINT8 *)NewMemoryMapCurrent - (UINT8 *)NewMemoryMapStart);
  FreePool (NewMemoryMapStart);

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

  Status = OrderedInsertImageRecordListEntry (&mNonProtectedImageRangesPrivate.NonProtectedImageList, &ImageRecord->Link);

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

      OrderedInsertCodeSectionListEntry (&ImageRecord->CodeSegmentList, &ImageRecordCodeSection->Link);
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
  @param[in]      SpecialRegionList               List of special regions to separate

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
           (((UINTN)MapEntryInsert + *DescriptorSize) > *BufferSize))
    {
      MapEntryStart = (UINTN)MemoryMapEntry->PhysicalStart;
      MapEntryEnd   = (UINTN)MemoryMapEntry->PhysicalStart + (UINTN)EFI_PAGES_TO_SIZE (MemoryMapEntry->NumberOfPages);
      if ((MapEntryStart <= SpecialRegionStart) && (MapEntryEnd >= SpecialRegionStart)) {
        // Check if some portion of the map entry isn't covered by the special region
        if (MapEntryStart != SpecialRegionStart) {
          // Populate a new descriptor for the region before the special region
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
          // Populate a new descriptor for the region after the special region
          POPULATE_MEMORY_DESCRIPTOR_ENTRY (
            MapEntryInsert,
            SpecialRegionEnd,
            EFI_SIZE_TO_PAGES (MapEntryEnd - SpecialRegionEnd),
            MemoryMapEntry->Type
            );
          MapEntryInsert->Attribute = MemoryMapEntry->Attribute;

          // Trim the current memory map entry
          MemoryMapEntry->NumberOfPages -= MapEntryInsert->NumberOfPages;
          MapEntryEnd                    = SpecialRegionEnd;

          // Get the next blank map entry
          MapEntryInsert = NEXT_MEMORY_DESCRIPTOR (MapEntryInsert, *DescriptorSize);
        }

        // This entry is now covered entirely by the special region - update the attributes and mark this
        // entry as a special region
        MemoryMapEntry->Attribute    = SpecialRegionEntry->SpecialRegion.EfiAttributes;
        MemoryMapEntry->VirtualStart = SPECIAL_REGION_PATTERN;
        SpecialRegionStart           = MapEntryEnd;
      }

      MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
    }

    if (SpecialRegionStart > SpecialRegionEnd) {
      return EFI_NOT_FOUND;
    }

    SpecialRegionEntryLink = SpecialRegionEntryLink->ForwardLink;
  }

  // if we've created new records, sort the map
  if ((UINTN)MapEntryInsert > (UINTN)MapEntryEnd) {
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
    if (!IS_BITMAP_INDEX_SET (Bitmap, Index) &&
        !IsCodeType (MemoryMapEntry->Type))
    {
      MemoryMapEntry->Attribute = GetPermissionAttributeForMemoryType (MemoryMapEntry->Type);
      SET_BITMAP_INDEX (Bitmap, Index);
    }

    Index++;
    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, *DescriptorSize);
  }

  return EFI_SUCCESS;
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
  @retval         EFI_INVALID_PARAMETER   One of the input parameters has an invalid value
  @retval         EFI_OUT_OF_RESOURCES    Failed to allocate memory
**/
EFI_STATUS
EFIAPI
GetMemoryMapWithPopulatedAccessAttributes (
  OUT UINTN                  *MemoryMapSize,
  OUT EFI_MEMORY_DESCRIPTOR  **MemoryMap,
  OUT UINTN                  *DescriptorSize
  )
{
  EFI_STATUS             Status;
  UINTN                  AdditionalRecordCount, NumberOfDescriptors, NumberOfBitmapEntries;
  UINT8                  *Bitmap = NULL;
  UINTN                  MapKey;
  UINT32                 DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR  *ExpandedMemoryMap;
  UINTN                  ExpandedMemoryMapSize;
  LIST_ENTRY             *MergedImageList           = NULL;
  LIST_ENTRY             **ArrayOfListEntryPointers = NULL;

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
      goto OutOfResourcesCleanup;
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
    return Status;
  }

  // Filter each map entry to only contain access attributes
  FilterMemoryMapAttributes (MemoryMapSize, *MemoryMap, DescriptorSize);

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
    goto OutOfResourcesCleanup;
  }

  CopyMem (ExpandedMemoryMap, *MemoryMap, *MemoryMapSize);
  FreePool (*MemoryMap);

  *MemoryMap = ExpandedMemoryMap;

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Currently protected images---\n"));
    DumpImageRecords (&mImagePropertiesPrivate.ImageRecordList);
    );

  DEBUG_CODE (
    DumpMemoryProtectionSpecialRegions ();
    );

  // Set MergedImageList to the list of IMAGE_PROPERTIES_RECORD entries which will
  // be used to breakup the memory map
  if (mImagePropertiesPrivate.ImageRecordCount == 0) {
    MergedImageList = &mNonProtectedImageRangesPrivate.NonProtectedImageList;
  } else if (mNonProtectedImageRangesPrivate.NonProtectedImageCount == 0) {
    MergedImageList = &mImagePropertiesPrivate.ImageRecordList;
  } else {
    MergedImageList          = &mImagePropertiesPrivate.ImageRecordList;
    ArrayOfListEntryPointers = AllocateZeroPool (mNonProtectedImageRangesPrivate.NonProtectedImageCount * sizeof (LIST_ENTRY *));

    if (ArrayOfListEntryPointers == NULL) {
      goto OutOfResourcesCleanup;
    }

    Status = MergeImagePropertiesRecordLists (
               MergedImageList,
               &mNonProtectedImageRangesPrivate.NonProtectedImageList,
               mNonProtectedImageRangesPrivate.NonProtectedImageCount,
               ArrayOfListEntryPointers
               );

    ASSERT_EFI_ERROR (Status);
  }

  SeparateImagesInMemoryMap (
    MemoryMapSize,
    *MemoryMap,
    *DescriptorSize,
    MergedImageList,
    AdditionalRecordCount
    );

  NumberOfDescriptors   = *MemoryMapSize / *DescriptorSize;
  NumberOfBitmapEntries = (NumberOfDescriptors % 8) == 0 ? NumberOfDescriptors : (((NumberOfDescriptors / 8) * 8) + 8);

  Bitmap = AllocateZeroPool (NumberOfBitmapEntries / 8);

  if (Bitmap == NULL) {
    goto OutOfResourcesCleanup;
  }

  // Set the extra bits
  if ((NumberOfDescriptors % 8) != 0) {
    Bitmap[NumberOfDescriptors / 8] |= ~((1 << (NumberOfDescriptors % 8)) - 1);
  }

  // Restore the nonprotected image list
  if (ArrayOfListEntryPointers != NULL) {
    Status = OrderedInsertImagePropertiesRecordArray (
               &mNonProtectedImageRangesPrivate.NonProtectedImageList,
               ArrayOfListEntryPointers,
               mNonProtectedImageRangesPrivate.NonProtectedImageCount
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (ArrayOfListEntryPointers);
  }

  // All image regions will now have nonzero attributes
  //
  // Set the bits in the bitmap to mark that the corresponding memory descriptor
  // has been set based on the memory protection policy
  Status = SyncBitmap (MemoryMapSize, *MemoryMap, DescriptorSize, Bitmap);
  ASSERT_EFI_ERROR (Status);

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Memory Map With Separated Image Descriptors---\n"));
    DumpMemoryMap (MemoryMapSize, *MemoryMap, DescriptorSize);
    DEBUG ((DEBUG_INFO, "---------------------------------------\n"));
    );

  // Remove the access attributes from descriptors which correspond with nonprotected images
  if (mNonProtectedImageRangesPrivate.NonProtectedImageCount > 0) {
    Status = RemoveAttributesOfNonProtectedImageRanges (
               MemoryMapSize,
               *MemoryMap,
               DescriptorSize,
               &mNonProtectedImageRangesPrivate.NonProtectedImageList
               );
    ASSERT_EFI_ERROR (Status);
  }

  // Set the access attributes of descriptor ranges which have not been checked
  // against our memory protection policy
  Status = SetAccessAttributesInMemoryMap (MemoryMapSize, *MemoryMap, DescriptorSize, Bitmap);
  ASSERT_EFI_ERROR (Status);

  DEBUG_CODE (
    DEBUG ((DEBUG_INFO, "---Final Bitmap---\n"));
    DumpBitmap (Bitmap, NumberOfBitmapEntries);
    DEBUG ((DEBUG_INFO, "---------------------------------------\n"));
    );

  // Merge contiguous entries with the type and attributes
  MergeMemoryMap (*MemoryMap, MemoryMapSize, *DescriptorSize);

  return Status;

OutOfResourcesCleanup:
  if (*MemoryMap != NULL) {
    FreePool (*MemoryMap);
  }

  if (ArrayOfListEntryPointers != NULL) {
    Status = OrderedInsertImagePropertiesRecordArray (
               &mNonProtectedImageRangesPrivate.NonProtectedImageList,
               ArrayOfListEntryPointers,
               mNonProtectedImageRangesPrivate.NonProtectedImageCount
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (ArrayOfListEntryPointers);
  }

  *MemoryMapSize  = 0;
  *DescriptorSize = 0;

  return EFI_OUT_OF_RESOURCES;
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
      Status = OrderedInsertImageRecordListEntry (&mImagePropertiesPrivate.ImageRecordList, &ImageRecord->Link);
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
  UINT64                     Attributes;
  LIST_ENTRY                 *Link;
  EFI_GCD_MAP_ENTRY          *Entry;
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

  // Apply the policy for RAM regions that we know are present and
  // accessible, but have not been added to the UEFI memory map (yet).
  if (GetDxeMemoryTypeSettingFromBitfield (EfiConventionalMemory, gDxeMps.NxProtectionPolicy)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: applying strict permissions to inactive memory regions\n",
      __FUNCTION__
      ));

    CoreAcquireGcdMemoryLock ();

    Link = mGcdMemorySpaceMap.ForwardLink;
    while (Link != &mGcdMemorySpaceMap) {
      Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);

      if ((Entry->GcdMemoryType == EfiGcdMemoryTypeReserved) &&
          (Entry->EndAddress < MAX_ADDRESS) &&
          ((Entry->Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
           (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED)))
      {
        Attributes = (GetDxeMemoryTypeSettingFromBitfield (EfiConventionalMemory, gDxeMps.NxProtectionPolicy) ? EFI_MEMORY_XP : 0) |
                     (Entry->Attributes & EFI_CACHE_ATTRIBUTE_MASK);

        DEBUG ((
          DEBUG_INFO,
          "Untested GCD memory space region: - 0x%016lx - 0x%016lx (0x%016lx)\n",
          Entry->BaseAddress,
          Entry->EndAddress - Entry->BaseAddress + 1,
          Attributes
          ));

        ASSERT (gCpu != NULL);
        gCpu->SetMemoryAttributes (
                gCpu,
                Entry->BaseAddress,
                Entry->EndAddress - Entry->BaseAddress + 1,
                Attributes
                );
      }

      Link = Link->ForwardLink;
    }

    CoreReleaseGcdMemoryLock ();
  }
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

  if (gDxeMps.NxProtectionPolicy.Data || gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromFv) {
    InitializePageAttributesForMemoryProtectionPolicy ();
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

  Status = gBS->LocateProtocol (&gEfiMemoryAttributeProtocolGuid, NULL, (VOID **)&MemoryAttributeProtocol);

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
