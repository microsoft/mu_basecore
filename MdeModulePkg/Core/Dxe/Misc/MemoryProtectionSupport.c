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

BOOLEAN                        mIsSystemNxCompatible    = TRUE;
EFI_MEMORY_ATTRIBUTE_PROTOCOL  *MemoryAttributeProtocol = NULL;

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
  Applies EFI_MEMORY_ACCESS_MASK to each memory map entry

  @param[in, out] MemoryMapSize     A pointer to the size, in bytes, of the
                                    MemoryMap buffer. On input, this is the size of
                                    old MemoryMap before split. The actual buffer
                                    size of MemoryMap is MemoryMapSize +
                                    (AdditionalRecordCount * DescriptorSize) calculated
                                    below. On output, it is the size of new MemoryMap
                                    after split.
  @param[in, out] MemoryMap         A pointer to the buffer in which firmware places
                                    the current memory map.
  @param[in]      DescriptorSize    Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
FilterMemoryMapAttributes (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);

  while (MemoryMapEntry < MemoryMapEnd) {
    MemoryMapEntry->Attribute &= EFI_MEMORY_ACCESS_MASK;
    MemoryMapEntry             = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }
}

/**
  Sets NX attribute on memory map based on NxProtectionPolicy

  @param[in, out] MemoryMapSize       A pointer to the size, in bytes, of the
                                      MemoryMap buffer. On input, this is the size of
                                      old MemoryMap before split. The actual buffer
                                      size of MemoryMap is MemoryMapSize +
                                      (AdditionalRecordCount * DescriptorSize) calculated
                                      below. On output, it is the size of new MemoryMap
                                      after split.
  @param[in, out] MemoryMap           A pointer to the buffer in which firmware places
                                      the current memory map.
  @param[in]      DescriptorSize      Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
SetAccessAttributesInMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);

  while (MemoryMapEntry < MemoryMapEnd) {
    if ((MemoryMapEntry->Attribute == 0) &&
        (!IsCodeType (MemoryMapEntry->Type)))
    {
      MemoryMapEntry->Attribute = GetPermissionAttributeForMemoryType (MemoryMapEntry->Type);
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }
}

/**
  This function for GetMemoryMap() with properties table capability.

  It calls original GetMemoryMap() to get the original memory map information. Then
  plus the additional memory map entries for PE Code/Data seperation.

  @param[in, out] MemoryMapSize           A pointer to the size, in bytes, of the
                                          MemoryMap buffer. On input, this is the size of
                                          the buffer allocated by the caller.  On output,
                                          it is the size of the buffer returned by the
                                          firmware  if the buffer was large enough, or the
                                          size of the buffer needed  to contain the map if
                                          the buffer was too small.
  @param[in, out] MemoryMap               A pointer to the buffer in which firmware places
                                          the current memory map.
  @param[out]     MapKey                  A pointer to the location in which firmware
                                          returns the key for the current memory map.
  @param[out]     DescriptorSize          A pointer to the location in which firmware
                                          returns the size, in bytes, of an individual
                                          EFI_MEMORY_DESCRIPTOR.
  @param[out]     DescriptorVersion       A pointer to the location in which firmware
                                          returns the version number associated with the
                                          EFI_MEMORY_DESCRIPTOR.

  @retval         EFI_SUCCESS             The memory map was returned in the MemoryMap
                                          buffer.
  @retval         EFI_BUFFER_TOO_SMALL    The MemoryMap buffer was too small. The current
                                          buffer size needed to hold the memory map is
                                          returned in MemoryMapSize.
  @retval         EFI_INVALID_PARAMETER  One of the parameters has an invalid value.

**/
EFI_STATUS
EFIAPI
GetMemoryMapWithPopulatedAccessAttributes (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT UINTN                     *MapKey,
  OUT UINTN                     *DescriptorSize,
  OUT UINT32                    *DescriptorVersion
  )
{
  EFI_STATUS  Status;
  UINTN       OldMemoryMapSize;
  UINTN       AdditionalRecordCount;

  if (MemoryMapSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AdditionalRecordCount = (2 * mImagePropertiesPrivate.CodeSegmentCountMax + 3) * mImagePropertiesPrivate.ImageRecordCount;

  OldMemoryMapSize = *MemoryMapSize;
  Status           = CoreGetMemoryMap (MemoryMapSize, MemoryMap, MapKey, DescriptorSize, DescriptorVersion);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    *MemoryMapSize = *MemoryMapSize + (*DescriptorSize) * AdditionalRecordCount;
  } else if (Status == EFI_SUCCESS) {
    ASSERT (MemoryMap != NULL);
    if (OldMemoryMapSize - *MemoryMapSize < (*DescriptorSize) * AdditionalRecordCount) {
      *MemoryMapSize = *MemoryMapSize + (*DescriptorSize) * AdditionalRecordCount;
      Status         = EFI_BUFFER_TOO_SMALL;
    } else {
      // Filter each map entry to only contain access attributes
      FilterMemoryMapAttributes (MemoryMapSize, MemoryMap, *DescriptorSize);

      DEBUG_CODE (
        DEBUG ((DEBUG_INFO, "---Currently protected images---\n"));
        DumpImageRecords (&mImagePropertiesPrivate.ImageRecordList);
        );

      // Split PE code/data if firmware volume image protection is active
      if (gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromFv) {
        SeparateImagesInMemoryMap (MemoryMapSize, MemoryMap, *DescriptorSize, &mImagePropertiesPrivate.ImageRecordList, AdditionalRecordCount);
      }

      DEBUG_CODE (
        DEBUG ((DEBUG_INFO, "---Memory Map With Separated Image Descriptors---\n"));
        DumpMemoryMap (MemoryMapSize, MemoryMap, DescriptorSize);
        DEBUG ((DEBUG_INFO, "---------------------------------------\n"));
        );

      // Set attributes if NX protection is active
      if (gDxeMps.NxProtectionPolicy.Data) {
        SetAccessAttributesInMemoryMap (MemoryMapSize, MemoryMap, *DescriptorSize);
      }

      // Merge contiguous entries with the type and attributes
      MergeMemoryMap (MemoryMap, MemoryMapSize, *DescriptorSize);
    }
  }

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
      Status = EFI_UNSUPPORTED;
      goto Finish;
    case PROTECT_IF_ALIGNED_ELSE_ALLOW:
    case PROTECT_ELSE_RAISE_ERROR:
      break;
    default:
      ASSERT (FALSE);
      Status = EFI_INVALID_PARAMETER;
      goto Finish;
  }

  ImageRecord = AllocateZeroPool (sizeof (*ImageRecord));

  if (ImageRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  // If a record was already created for the memory attributes table, copy it
  Status = CopyExistingPropertiesRecord ((EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase, LoadedImage->ImageSize, ImageRecord);

  if (EFI_ERROR (Status)) {
    // Create a new image properties record
    Status = CreateImagePropertiesRecord (LoadedImage->ImageBase, LoadedImage->ImageSize, LoadedImage->ImageCodeType, ImageRecord);
  }

  if (!EFI_ERROR (Status)) {
    if (gCpu != NULL) {
      SetUefiImageProtectionAttributes (ImageRecord);
    } else {
      // If gCpu is not ready, still put this record into the image record
      // list so it can be protected in MemoryProtectionCpuArchProtocolNotifyMu()
      Status = EFI_NOT_READY;
    }

    // Record the image record in the list so we can undo the protections later
    Status = OrderedInsertImageRecordListEntry (&mImagePropertiesPrivate.ImageRecordList, &ImageRecord->Link);
    mImagePropertiesPrivate.ImageRecordCount++;

    // When breaking up the memory map to include image code/data ranges, we need
    // to know the maximum number of code segments a single image will have
    if (mImagePropertiesPrivate.CodeSegmentCountMax < ImageRecord->CodeSegmentCount) {
      mImagePropertiesPrivate.CodeSegmentCountMax = ImageRecord->CodeSegmentCount;
    }
  }

Finish:
  if (EFI_ERROR (Status)) {
    // If we failed to protect the image for reasons other than CPU Arch not being ready,
    // clear the access attributes from the memory and free the image record.
    if (Status != EFI_NOT_READY) {
      ClearAccessAttributesFromMemoryRange (
        (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase,
        ALIGN_VALUE ((UINTN)LoadedImage->ImageSize, EFI_PAGE_SIZE)
        );

      if (ImageRecord != NULL) {
        FreeImageRecord (ImageRecord);
      }
    }

    // If the status is EFI_NOT_READY, the CPU Arch has not been installed. We assume
    // CPU Arch will be installed later in boot, so don't return an error in that case
    // or in the case that the protection policy indicates we shouldn't return an error
    // if protection fails.
    if ((ProtectionPolicy != PROTECT_ELSE_RAISE_ERROR) || (Status == EFI_NOT_READY)) {
      Status = EFI_SUCCESS;
    }
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

  if (gDxeMps.ImageProtectionPolicy.Data) {
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
    }
  }
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
  UINTN                      MapKey;
  UINTN                      DescriptorSize;
  UINT32                     DescriptorVersion;
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
             MemoryMap,
             &MapKey,
             &DescriptorSize,
             &DescriptorVersion
             );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  do {
    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocatePool (MemoryMapSize);
    ASSERT (MemoryMap != NULL);
    Status = GetMemoryMapWithPopulatedAccessAttributes (
               &MemoryMapSize,
               MemoryMap,
               &MapKey,
               &DescriptorSize,
               &DescriptorVersion
               );
    if (EFI_ERROR (Status)) {
      FreePool (MemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

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
