/** @file
  UEFI Memory Protection support.

  If the UEFI image is page aligned, the image code section is set to read only
  and the image data section is set to non-executable.

  1) This policy is applied for all UEFI image including boot service driver,
     runtime driver or application.
  2) This policy is applied only if the UEFI image meets the page alignment
     requirement.
  3) This policy is applied only if the Source UEFI image matches the
     Image Protection Policy definition. // MU_CHANGE
  4) This policy is not applied to the non-PE image region.

  The DxeCore calls CpuArchProtocol->SetMemoryAttributes() to protect
  the image. If the CpuArch protocol is not installed yet, the DxeCore
  enqueues the protection request. Once the CpuArch is installed, the
  DxeCore dequeues the protection request and applies policy.

  Once the image is unloaded, the protection is removed automatically.

Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <Guid/EventGroup.h>
#include <Guid/MemoryAttributesTable.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/HeapGuardDebug.h> // MS_CHANGE

#include "DxeMain.h"
#include "Mem/HeapGuard.h"
#include "MemoryProtectionSupport.h"

//
// Image type definitions
//
#define IMAGE_UNKNOWN  0x00000001
#define IMAGE_FROM_FV  0x00000002

//
// Protection policy bit definition
//
// MU_CHANGE START: Moved to MemoryProtectionSupport.h
// #define DO_NOT_PROTECT                 0x00000000
// #define PROTECT_IF_ALIGNED_ELSE_ALLOW  0x00000001
// MU_CHANG END

#define MEMORY_TYPE_OS_RESERVED_MIN   0x80000000
#define MEMORY_TYPE_OEM_RESERVED_MIN  0x70000000

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

// UINT32   mImageProtectionPolicy; // MU_CHANGE

extern LIST_ENTRY  mGcdMemorySpaceMap;

STATIC LIST_ENTRY  mProtectedImageRecordList;
// MS_CHANGE - START
STATIC HEAP_GUARD_DEBUG_PROTOCOL  mHeapGuardDebug = {
  IsGuardPage
};
// MS_CHANGE - END

/**
  Sort code section in image record, based upon CodeSegmentBase from low to high.

  @param  ImageRecord    image record to be sorted
**/
VOID
SortImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
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
  Get the image type.

  @param[in]    File       This is a pointer to the device path of the file that is
                           being dispatched.

  @return UINT32           Image Type
**/
UINT32
GetImageType (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;

  if (File == NULL) {
    return IMAGE_UNKNOWN;
  }

  //
  // First check to see if File is from a Firmware Volume
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)File;
  Status         = gBS->LocateDevicePath (
                          &gEfiFirmwareVolume2ProtocolGuid,
                          &TempDevicePath,
                          &DeviceHandle
                          );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      return IMAGE_FROM_FV;
    }
  }

  return IMAGE_UNKNOWN;
}

/**
  Get UEFI image protection policy based upon image type.

  @param[in]  ImageType    The UEFI image type

  @return UEFI image protection policy
**/
UINT32
GetProtectionPolicyFromImageType (
  IN UINT32  ImageType
  )
{
  // MU_CHANGE START Use ImageProtectionPolicy Bitfield
  // if ((ImageType & mImageProtectionPolicy) == 0) {
  //   return DO_NOT_PROTECT;
  // } else {
  //   return PROTECT_IF_ALIGNED_ELSE_ALLOW;
  // }
  if (((ImageType == IMAGE_UNKNOWN) && gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromUnknown) ||
      ((ImageType == IMAGE_FROM_FV) && gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromFv))
  {
    if (gDxeMps.ImageProtectionPolicy.Fields.RaiseErrorIfProtectionFails) {
      return PROTECT_ELSE_RAISE_ERROR;
    }

    return PROTECT_IF_ALIGNED_ELSE_ALLOW;
  } else {
    return DO_NOT_PROTECT;
  }

  // MU_CHANGE END
}

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
  )
{
  BOOLEAN                         InSmm;
  UINT32                          ImageType;
  UINT32                          ProtectionPolicy;
  DXE_MEMORY_PROTECTION_SETTINGS  *Settings = NULL;

  //
  // Check SMM
  //
  InSmm = FALSE;
  if (gSmmBase2 != NULL) {
    gSmmBase2->InSmm (gSmmBase2, &InSmm);
  }

  if (InSmm) {
    return FALSE;
  }

  //
  // Check DevicePath
  //
  if (LoadedImage == gDxeCoreLoadedImage) {
    // MU_CHANGE START
    // If the image is DxeCore, DxeMemoryProtectionHobLib entry point has not
    // yet executed and so gDxeMps is not yet valid. Get the memory protection
    // HOB directly and check if DxeCore should be protected.
    Settings = GetDxeMemoryProtectionSettings ();

    if (Settings != NULL) {
      if (Settings->ImageProtectionPolicy.Fields.ProtectImageFromFv == 1) {
        if (Settings->ImageProtectionPolicy.Fields.RaiseErrorIfProtectionFails == 1) {
          return PROTECT_ELSE_RAISE_ERROR;
        }

        return PROTECT_IF_ALIGNED_ELSE_ALLOW;
      }
    }

    // MU_CHANGE END
    ImageType = IMAGE_FROM_FV;
  } else {
    ImageType = GetImageType (LoadedImageDevicePath);
  }

  ProtectionPolicy = GetProtectionPolicyFromImageType (ImageType);
  return ProtectionPolicy;
}

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
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;
  UINT64                           FinalAttributes;

  Status = CoreGetMemorySpaceDescriptor (BaseAddress, &Descriptor);
  ASSERT_EFI_ERROR (Status);

  FinalAttributes = (Descriptor.Attributes & EFI_CACHE_ATTRIBUTE_MASK) | (Attributes & EFI_MEMORY_ATTRIBUTE_MASK);

  DEBUG ((DEBUG_INFO, "SetUefiImageMemoryAttributes - 0x%016lx - 0x%016lx (0x%016lx)\n", BaseAddress, Length, FinalAttributes));

  ASSERT (gCpu != NULL);
  gCpu->SetMemoryAttributes (gCpu, BaseAddress, Length, FinalAttributes);
}

/**
  Set UEFI image protection attributes.

  @param[in]  ImageRecord    A UEFI image record
**/
VOID
SetUefiImageProtectionAttributes (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  LIST_ENTRY                            *ImageRecordCodeSectionLink;
  LIST_ENTRY                            *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                            *ImageRecordCodeSectionList;
  UINT64                                CurrentBase;
  UINT64                                ImageEnd;

  ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

  CurrentBase = ImageRecord->ImageBase;
  ImageEnd    = ImageRecord->ImageBase + ImageRecord->ImageSize;

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

    ASSERT (CurrentBase <= ImageRecordCodeSection->CodeSegmentBase);
    if (CurrentBase < ImageRecordCodeSection->CodeSegmentBase) {
      //
      // DATA
      //
      SetUefiImageMemoryAttributes (
        CurrentBase,
        ImageRecordCodeSection->CodeSegmentBase - CurrentBase,
        EFI_MEMORY_XP
        );
    }

    //
    // CODE
    //
    SetUefiImageMemoryAttributes (
      ImageRecordCodeSection->CodeSegmentBase,
      ImageRecordCodeSection->CodeSegmentSize,
      EFI_MEMORY_RO
      );
    CurrentBase = ImageRecordCodeSection->CodeSegmentBase + ImageRecordCodeSection->CodeSegmentSize;
  }

  //
  // Last DATA
  //
  ASSERT (CurrentBase <= ImageEnd);
  if (CurrentBase < ImageEnd) {
    //
    // DATA
    //
    SetUefiImageMemoryAttributes (
      CurrentBase,
      ImageEnd - CurrentBase,
      EFI_MEMORY_XP
      );
  }

  return;
}

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
  )
{
  UINT32  PageAlignment;

  switch (MemoryType) {
    case EfiRuntimeServicesCode:
    case EfiACPIMemoryNVS:
      PageAlignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      break;
    case EfiRuntimeServicesData:
    case EfiACPIReclaimMemory:
      ASSERT (FALSE);
      PageAlignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      break;
    case EfiBootServicesCode:
    case EfiLoaderCode:
    case EfiReservedMemoryType:
      PageAlignment = EFI_PAGE_SIZE;
      break;
    default:
      ASSERT (FALSE);
      PageAlignment = EFI_PAGE_SIZE;
      break;
  }

  if ((SectionAlignment & (PageAlignment - 1)) != 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Free Image record.

  @param[in]  ImageRecord    A UEFI image record
**/
VOID
FreeImageRecord (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  )
{
  LIST_ENTRY                            *CodeSegmentListHead;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;

  CodeSegmentListHead = &ImageRecord->CodeSegmentList;
  while (!IsListEmpty (CodeSegmentListHead)) {
    ImageRecordCodeSection = CR (
                               CodeSegmentListHead->ForwardLink,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                               Link,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                               );
    RemoveEntryList (&ImageRecordCodeSection->Link);
    FreePool (ImageRecordCodeSection);
  }

  if (ImageRecord->Link.ForwardLink != NULL) {
    RemoveEntryList (&ImageRecord->Link);
  }

  FreePool (ImageRecord);
}

/**
  Protect UEFI PE/COFF image.

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol
**/
VOID
ProtectUefiImage (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath
  )
{
  VOID                                  *ImageAddress;
  EFI_IMAGE_DOS_HEADER                  *DosHdr;
  UINT32                                PeCoffHeaderOffset;
  UINT32                                SectionAlignment;
  EFI_IMAGE_SECTION_HEADER              *Section;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  UINT8                                 *Name;
  UINTN                                 Index;
  IMAGE_PROPERTIES_RECORD               *ImageRecord;
  CHAR8                                 *PdbPointer;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  BOOLEAN                               IsAligned;
  UINT32                                ProtectionPolicy;

  DEBUG ((DEBUG_INFO, "ProtectUefiImageCommon - 0x%x\n", LoadedImage));
  DEBUG ((DEBUG_INFO, "  - 0x%016lx - 0x%016lx\n", (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase, LoadedImage->ImageSize));

  if (gCpu == NULL) {
    return;
  }

  ProtectionPolicy = GetUefiImageProtectionPolicy (LoadedImage, LoadedImageDevicePath);
  switch (ProtectionPolicy) {
    case DO_NOT_PROTECT:
      return;
    case PROTECT_IF_ALIGNED_ELSE_ALLOW:
      break;
    default:
      ASSERT (FALSE);
      return;
  }

  ImageRecord = AllocateZeroPool (sizeof (*ImageRecord));
  if (ImageRecord == NULL) {
    return;
  }

  ImageRecord->Signature = IMAGE_PROPERTIES_RECORD_SIGNATURE;

  //
  // Step 1: record whole region
  //
  ImageRecord->ImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase;
  ImageRecord->ImageSize = LoadedImage->ImageSize;

  ImageAddress = LoadedImage->ImageBase;

  PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
  if (PdbPointer != NULL) {
    DEBUG ((DEBUG_VERBOSE, "  Image - %a\n", PdbPointer));
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
    goto Finish;
  }

  //
  // Get SectionAlignment
  //
  if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    SectionAlignment = Hdr.Pe32->OptionalHeader.SectionAlignment;
  } else {
    SectionAlignment = Hdr.Pe32Plus->OptionalHeader.SectionAlignment;
  }

  IsAligned = IsMemoryProtectionSectionAligned (SectionAlignment, LoadedImage->ImageCodeType);
  if (!IsAligned) {
    DEBUG ((
      DEBUG_VERBOSE,
      "!!!!!!!!  ProtectUefiImageCommon - Section Alignment(0x%x) is incorrect  !!!!!!!!\n",
      SectionAlignment
      ));
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_VERBOSE, "!!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
    }

    goto Finish;
  }

  Section = (EFI_IMAGE_SECTION_HEADER *)(
                                         (UINT8 *)(UINTN)ImageAddress +
                                         PeCoffHeaderOffset +
                                         sizeof (UINT32) +
                                         sizeof (EFI_IMAGE_FILE_HEADER) +
                                         Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                                         );
  ImageRecord->CodeSegmentCount = 0;
  InitializeListHead (&ImageRecord->CodeSegmentList);
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
      ImageRecordCodeSection = AllocatePool (sizeof (*ImageRecordCodeSection));
      if (ImageRecordCodeSection == NULL) {
        return;
      }

      ImageRecordCodeSection->Signature = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;

      ImageRecordCodeSection->CodeSegmentBase = (UINTN)ImageAddress + Section[Index].VirtualAddress;
      ImageRecordCodeSection->CodeSegmentSize = ALIGN_VALUE (Section[Index].SizeOfRawData, SectionAlignment);

      DEBUG ((DEBUG_VERBOSE, "ImageCode: 0x%016lx - 0x%016lx\n", ImageRecordCodeSection->CodeSegmentBase, ImageRecordCodeSection->CodeSegmentSize));

      InsertTailList (&ImageRecord->CodeSegmentList, &ImageRecordCodeSection->Link);
      ImageRecord->CodeSegmentCount++;
    }
  }

  if (ImageRecord->CodeSegmentCount == 0) {
    //
    // If a UEFI executable consists of a single read+write+exec PE/COFF
    // section, that isn't actually an error. The image can be launched
    // alright, only image protection cannot be applied to it fully.
    //
    // One example that elicits this is (some) Linux kernels (with the EFI stub
    // of course).
    //
    DEBUG ((DEBUG_WARN, "!!!!!!!!  ProtectUefiImageCommon - CodeSegmentCount is 0  !!!!!!!!\n"));
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_WARN, "!!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
    }

    goto Finish;
  }

  //
  // Final
  //
  SortImageRecordCodeSection (ImageRecord);
  //
  // Check overlap all section in ImageBase/Size
  //
  if (!IsImageRecordCodeSectionValid (ImageRecord)) {
    DEBUG ((DEBUG_ERROR, "IsImageRecordCodeSectionValid - FAIL\n"));
    goto Finish;
  }

  //
  // Round up the ImageSize, some CPU arch may return EFI_UNSUPPORTED if ImageSize is not aligned.
  // Given that the loader always allocates full pages, we know the space after the image is not used.
  //
  ImageRecord->ImageSize = ALIGN_VALUE (LoadedImage->ImageSize, EFI_PAGE_SIZE);

  //
  // CPU ARCH present. Update memory attribute directly.
  //
  SetUefiImageProtectionAttributes (ImageRecord);

  //
  // Record the image record in the list so we can undo the protections later
  //
  InsertTailList (&mProtectedImageRecordList, &ImageRecord->Link);

Finish:
  return;
}

/**
  Unprotect UEFI image.

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol
**/
VOID
UnprotectUefiImage (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  LIST_ENTRY               *ImageRecordLink;

  // // MU_CHANGE START Update to use memory protection settings HOB
  // if (PcdGet32(PcdImageProtectionPolicy) != 0) {
  if (gDxeMps.ImageProtectionPolicy.Data) {
    // MU_CHANGE END
    for (ImageRecordLink = mProtectedImageRecordList.ForwardLink;
         ImageRecordLink != &mProtectedImageRecordList;
         ImageRecordLink = ImageRecordLink->ForwardLink)
    {
      ImageRecord = CR (
                      ImageRecordLink,
                      IMAGE_PROPERTIES_RECORD,
                      Link,
                      IMAGE_PROPERTIES_RECORD_SIGNATURE
                      );

      if (ImageRecord->ImageBase == (EFI_PHYSICAL_ADDRESS)(UINTN)LoadedImage->ImageBase) {
        SetUefiImageMemoryAttributes (
          ImageRecord->ImageBase,
          ImageRecord->ImageSize,
          0
          );
        FreeImageRecord (ImageRecord);
        return;
      }
    }
  }
}

/**
  Return the EFI memory permission attribute associated with memory
  type 'MemoryType' under the configured DXE memory protection policy.

  @param MemoryType       Memory type.
**/
// STATIC // MU_CHANGE
UINT64
GetPermissionAttributeForMemoryType (
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  // MU_CHANGE START Update to use memory protection settings HOB
  // UINT64 TestBit;

  // if ((UINT32)MemoryType >= MEMORY_TYPE_OS_RESERVED_MIN) {
  //   TestBit = BIT63;
  // } else if ((UINT32)MemoryType >= MEMORY_TYPE_OEM_RESERVED_MIN) {
  //   TestBit = BIT62;
  // } else {
  //   TestBit = LShiftU64 (1, MemoryType);
  // }

  // if ((gDxeMps.NxProtectionPolicy & TestBit) != 0) { // MU_CHANGE
  //   return EFI_MEMORY_XP;
  // } else {
  //   return 0;
  // }

  // Handle code allocations according to the NX_COMPAT DLL flag. If the flag is
  // set, the image should update the attributes of code type allocates when it's ready to execute them.
  if (IsCodeType (MemoryType) && !IsSystemNxCompatible ()) {
    return 0;
  } else if (GetDxeMemoryTypeSettingFromBitfield (MemoryType, gDxeMps.NxProtectionPolicy)) {
    return EFI_MEMORY_XP;
  }

  return 0;

  // MU_CHANGE END
}

/**
  Sort memory map entries based upon PhysicalStart, from low to high.

  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MemoryMapSize          Size, in bytes, of the MemoryMap buffer.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
// STATIC // MU_CHANGE
VOID
SortMemoryMap (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      MemoryMapSize,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *NextMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR  TempMemoryMap;

  MemoryMapEntry     = MemoryMap;
  NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  MemoryMapEnd       = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while (MemoryMapEntry < MemoryMapEnd) {
    while (NextMemoryMapEntry < MemoryMapEnd) {
      if (MemoryMapEntry->PhysicalStart > NextMemoryMapEntry->PhysicalStart) {
        CopyMem (&TempMemoryMap, MemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (MemoryMapEntry, NextMemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (NextMemoryMapEntry, &TempMemoryMap, sizeof (EFI_MEMORY_DESCRIPTOR));
      }

      NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
    }

    MemoryMapEntry     = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }
}

/**
  Merge adjacent memory map entries if they use the same memory protection policy

  @param[in, out]  MemoryMap              A pointer to the buffer in which firmware places
                                          the current memory map.
  @param[in, out]  MemoryMapSize          A pointer to the size, in bytes, of the
                                          MemoryMap buffer. On input, this is the size of
                                          the current memory map.  On output,
                                          it is the size of new memory map after merge.
  @param[in]       DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
// STATIC // MU_CHANGE
VOID
MergeMemoryMapForProtectionPolicy (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN OUT UINTN                  *MemoryMapSize,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  UINT64                 MemoryBlockLength;
  EFI_MEMORY_DESCRIPTOR  *NewMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *NextMemoryMapEntry;
  UINT64                 Attributes;

  SortMemoryMap (MemoryMap, *MemoryMapSize, DescriptorSize);

  MemoryMapEntry    = MemoryMap;
  NewMemoryMapEntry = MemoryMap;
  MemoryMapEnd      = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    CopyMem (NewMemoryMapEntry, MemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);

    do {
      MemoryBlockLength = (UINT64)(EFI_PAGES_TO_SIZE ((UINTN)MemoryMapEntry->NumberOfPages));
      Attributes        = GetPermissionAttributeForMemoryType (MemoryMapEntry->Type);

      if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) &&
          (Attributes == GetPermissionAttributeForMemoryType (NextMemoryMapEntry->Type)) &&
          ((MemoryMapEntry->PhysicalStart + MemoryBlockLength) == NextMemoryMapEntry->PhysicalStart))
      {
        MemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
        if (NewMemoryMapEntry != MemoryMapEntry) {
          NewMemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
        }

        NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
        continue;
      } else {
        MemoryMapEntry = PREVIOUS_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
        break;
      }
    } while (TRUE);

    MemoryMapEntry    = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapEntry, DescriptorSize);
  }

  *MemoryMapSize = (UINTN)NewMemoryMapEntry - (UINTN)MemoryMap;

  return;
}

/**
  Remove exec permissions from all regions whose type is identified by
  the Dxe NX Protection Policy. // MU_CHANGE
**/
STATIC
VOID
InitializeDxeNxMemoryProtectionPolicy (
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

  //
  // Get the EFI memory map.
  //
  MemoryMapSize = 0;
  MemoryMap     = NULL;

  Status = gBS->GetMemoryMap (
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
    Status = gBS->GetMemoryMap (
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
  // MU_CHANGE START Update to use memory protection settings HOB
  // if (PcdGetBool (PcdCpuStackGuard)) {
  if (gDxeMps.CpuStackGuard) {
    // MU_CHANGE END
    //
    // Get the base of stack from Hob.
    //
    Hob.Raw = GetHobList ();
    while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
      MemoryHob = Hob.MemoryAllocation;
      if (CompareGuid (&gEfiHobMemoryAllocStackGuid, &MemoryHob->AllocDescriptor.Name)) {
        DEBUG ((
          DEBUG_INFO,
          "%a: StackBase = 0x%016lx  StackSize = 0x%016lx\n",
          __func__,
          MemoryHob->AllocDescriptor.MemoryBaseAddress,
          MemoryHob->AllocDescriptor.MemoryLength
          ));

        StackBase = MemoryHob->AllocDescriptor.MemoryBaseAddress;
        //
        // Ensure the base of the stack is page-size aligned.
        //
        ASSERT ((StackBase & EFI_PAGE_MASK) == 0);
        break;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
    }

    //
    // Ensure the base of stack can be found from Hob when stack guard is
    // enabled.
    //
    ASSERT (StackBase != 0);
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: applying strict permissions to active memory regions\n",
    __func__
    ));

  MergeMemoryMapForProtectionPolicy (MemoryMap, &MemoryMapSize, DescriptorSize);

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    Attributes = GetPermissionAttributeForMemoryType (MemoryMapEntry->Type);
    if (Attributes != 0) {
      SetUefiImageMemoryAttributes (
        MemoryMapEntry->PhysicalStart,
        LShiftU64 (MemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT),
        Attributes
        );

      //
      // Add EFI_MEMORY_RP attribute for page 0 if NULL pointer detection is
      // enabled.
      //
      // MU_CHANGE START: We Enable NULL detection via the function EnableNullDetection
      // if (MemoryMapEntry->PhysicalStart == 0 &&
      //     // PcdGet8 (PcdNullPointerDetectionPropertyMask) != 0) {
      //   ASSERT (MemoryMapEntry->NumberOfPages > 0);
      //   SetUefiImageMemoryAttributes (
      //     0,
      //     EFI_PAGES_TO_SIZE (1),
      //     EFI_MEMORY_RP | Attributes);
      // }
      // MU_CHANGE END

      //
      // Add EFI_MEMORY_RP attribute for the first page of the stack if stack
      // guard is enabled.
      //
      if ((StackBase != 0) &&
          ((StackBase >= MemoryMapEntry->PhysicalStart) &&
           (StackBase <  MemoryMapEntry->PhysicalStart +
            LShiftU64 (MemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT))) &&
          // MU_CHANGE START Update to use memory protection settings HOB
          // PcdGetBool (PcdCpuStackGuard)) {
          gDxeMps.CpuStackGuard)
      {
        // MU_CHANGE END
        SetUefiImageMemoryAttributes (
          StackBase,
          EFI_PAGES_TO_SIZE (1),
          EFI_MEMORY_RP | Attributes
          );
      }
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  FreePool (MemoryMap);

  //
  // Apply the policy for RAM regions that we know are present and
  // accessible, but have not been added to the UEFI memory map (yet).
  //
  if (GetPermissionAttributeForMemoryType (EfiConventionalMemory) != 0) {
    DEBUG ((
      DEBUG_INFO,
      "%a: applying strict permissions to inactive memory regions\n",
      __func__
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
        Attributes = GetPermissionAttributeForMemoryType (EfiConventionalMemory) |
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
MemoryProtectionCpuArchProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath;
  UINTN                      NoHandles;
  EFI_HANDLE                 *HandleBuffer;
  UINTN                      Index;

  DEBUG ((DEBUG_INFO, "MemoryProtectionCpuArchProtocolNotify:\n"));
  Status = CoreLocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&gCpu);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Apply the memory protection policy on non-BScode/RTcode regions.
  //
  // MU_CHANGE START Update to use memory protection settings HOB
  // if (PcdGet64 (PcdDxeNxMemoryProtectionPolicy) != 0) {
  if (gDxeMps.NxProtectionPolicy.Data) {
    // MU_CHANGE END
    InitializeDxeNxMemoryProtectionPolicy ();
  }

  //
  // Call notify function meant for Heap Guard.
  //
  HeapGuardCpuArchProtocolNotify ();

  // MU_CHANGE START Update to use memory protection settings HOB
  // if (mImageProtectionPolicy == 0) {
  if (!gDxeMps.ImageProtectionPolicy.Data) {
    // MU_CHANGE END
    goto Done;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadedImageProtocolGuid,
                  NULL,
                  &NoHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status) && (NoHandles == 0)) {
    goto Done;
  }

  for (Index = 0; Index < NoHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageDevicePathProtocolGuid,
                    (VOID **)&LoadedImageDevicePath
                    );
    if (EFI_ERROR (Status)) {
      LoadedImageDevicePath = NULL;
    }

    // MU_CHANGE START Use Project Mu ProtectUefiImage()
    // ProtectUefiImage (LoadedImage, LoadedImageDevicePath);
    Status = ProtectUefiImageMu (LoadedImage, LoadedImageDevicePath);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Unable to protect Image Handle: 0x%p... Unloading Image.\n", LoadedImage->DeviceHandle));
      Status = CoreUnloadImage (LoadedImage->DeviceHandle);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Unable to unload Image... Status: %r.\n", Status));
      }
    }

    // MU_CHANGE END
  }

  FreePool (HandleBuffer);

Done:
  CoreCloseEvent (Event);
}

/**
  ExitBootServices Callback function for memory protection.
**/
VOID
MemoryProtectionExitBootServicesCallback (
  VOID
  )
{
  EFI_RUNTIME_IMAGE_ENTRY  *RuntimeImage;
  LIST_ENTRY               *Link;

  //
  // We need remove the RT protection, because RT relocation need write code segment
  // at SetVirtualAddressMap(). We cannot assume OS/Loader has taken over page table at that time.
  //
  // Firmware does not own page tables after ExitBootServices(), so the OS would
  // have to relax protection of RT code pages across SetVirtualAddressMap(), or
  // delay setting protections on RT code pages until after SetVirtualAddressMap().
  // OS may set protection on RT based upon EFI_MEMORY_ATTRIBUTES_TABLE later.
  //
  // MU_CHANGE START Update to use memory protection settings HOB
  // if (mImageProtectionPolicy != 0) {
  if (gDxeMps.ImageProtectionPolicy.Data) {
    // MU_CHANGE END
    for (Link = gRuntime->ImageHead.ForwardLink; Link != &gRuntime->ImageHead; Link = Link->ForwardLink) {
      RuntimeImage = BASE_CR (Link, EFI_RUNTIME_IMAGE_ENTRY, Link);
      SetUefiImageMemoryAttributes ((UINT64)(UINTN)RuntimeImage->ImageBase, ALIGN_VALUE (RuntimeImage->ImageSize, EFI_PAGE_SIZE), 0);
    }
  }
}

/**
  Disable NULL pointer detection after EndOfDxe. This is a workaround resort in
  order to skip unfixable NULL pointer access issues detected in OptionROM or
  boot loaders.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the Event.
**/
// MU_CHANGE START Update naming due to addition of ability to disable at readytoboot
VOID
EFIAPI
DisableNullDetection (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Desc;

  DEBUG ((DEBUG_INFO, "%a - start\r\n", __FUNCTION__));
  //
  // Disable NULL pointer detection by enabling first 4K page
  //
  Status = CoreGetMemorySpaceDescriptor (0, &Desc);
  ASSERT_EFI_ERROR (Status);

  if ((Desc.Capabilities & EFI_MEMORY_RP) == 0) {
    Status = CoreSetMemorySpaceCapabilities (
               0,
               EFI_PAGE_SIZE,
               Desc.Capabilities | EFI_MEMORY_RP
               );
    ASSERT_EFI_ERROR (Status);
  }

  Status = CoreSetMemorySpaceAttributes (
             0,
             EFI_PAGE_SIZE,
             Desc.Attributes & ~EFI_MEMORY_RP
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Page 0 might have be allocated to avoid misuses. Free it here anyway.
  //
  CoreFreePages (0, 1);

  CoreCloseEvent (Event);
  DEBUG ((DEBUG_INFO, "%a - end\r\n", __FUNCTION__));

  return;
}

// MU_CHANGE END

// MU_CHANGE START: Add function to enable null detection as it is now done in DXE instead of PEI

/**
  Enable NULL pointer detection by changing the attributes of page 0. The assumption is that PEI
  has set page zero to allocated so this operation can be done safely.

  @retval EFI_SUCCESS       Page zero successfully marked as read protected
  @retval Other             Page zero could not be marked as read protected

**/
VOID
EFIAPI
EnableNullDetection (
  EFI_EVENT  Event,
  VOID       *Context
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
      goto Done;
    }
  }

  Status = CoreSetMemorySpaceAttributes (
             0,
             EFI_PAGES_TO_SIZE (1),
             Desc.Attributes | EFI_MEMORY_RP
             );
  ASSERT_EFI_ERROR (Status);

Done:
  CoreCloseEvent (Event);
}

// MU_CHANGE END

/**
  Initialize Memory Protection support.
**/
VOID
EFIAPI
CoreInitializeMemoryProtection (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  EFI_EVENT   DisableNullDetectionEvent;
  EFI_EVENT   EnableNullDetectionEvent;     // MU_CHANGE
  VOID        *Registration;

  // mImageProtectionPolicy = gDxeMps.ImageProtectionPolicy; // MU_CHANGE

  InitializeListHead (&mProtectedImageRecordList);

  //
  // Sanity check the Image Protection Policy setting: // MU_CHANGE
  // - code regions should have no EFI_MEMORY_XP attribute
  // - EfiConventionalMemory and EfiBootServicesData should use the
  //   same attribute
  //
  // MU_CHANGE START: We allow code types to have NX
  // ASSERT ((GetPermissionAttributeForMemoryType (EfiBootServicesCode) & EFI_MEMORY_XP) == 0);
  // ASSERT ((GetPermissionAttributeForMemoryType (EfiRuntimeServicesCode) & EFI_MEMORY_XP) == 0);
  // ASSERT ((GetPermissionAttributeForMemoryType (EfiLoaderCode) & EFI_MEMORY_XP) == 0);
  // MU_CHANGE END
  ASSERT (
    GetPermissionAttributeForMemoryType (EfiBootServicesData) ==
    GetPermissionAttributeForMemoryType (EfiConventionalMemory)
    );

  Status = CoreCreateEvent (
             EVT_NOTIFY_SIGNAL,
             // MU_CHANGE START: Use Project Mu Arch Protocol Notify
             TPL_CALLBACK - 1,
             //  MemoryProtectionCpuArchProtocolNotify,
             MemoryProtectionCpuArchProtocolNotifyMu,
             // MU_CHANGE END
             NULL,
             &Event
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifactions on this event
  //
  Status = CoreRegisterProtocolNotify (
             &gEfiCpuArchProtocolGuid,
             Event,
             &Registration
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Register a callback to disable NULL pointer detection at EndOfDxe
  //
  // MU_CHANGE START Update to use memory protection settings HOB, added
  //                 disable functionality at ReadyToBoot, create NULL
  //                 detection enable event
  // if ((PcdGet8 (PcdNullPointerDetectionPropertyMask) & (BIT0|BIT7))
  //      == (BIT0|BIT7)) {
  if (gDxeMps.NullPointerDetectionPolicy.Fields.UefiNullDetection) {
    // PEI phase has been updated to always set page zero as allocated
    // so it can be safely set as RP
    Status = CoreCreateEvent (
               EVT_NOTIFY_SIGNAL,
               TPL_CALLBACK - 1,
               EnableNullDetection,
               NULL,
               &EnableNullDetectionEvent
               );
    ASSERT_EFI_ERROR (Status);

    Status = CoreRegisterProtocolNotify (
               &gEfiCpuArchProtocolGuid,
               EnableNullDetectionEvent,
               &Registration
               );
    ASSERT_EFI_ERROR (Status);

    if (!EFI_ERROR (Status)) {
      // If both DisableEndOfDxe and DisableReadyToBoot are enabled, just
      // create the event to disable at EndOfDxe because that event is sooner
      if (gDxeMps.NullPointerDetectionPolicy.Fields.DisableEndOfDxe) {
        Status = CoreCreateEventEx (
                   EVT_NOTIFY_SIGNAL,
                   TPL_NOTIFY,
                   DisableNullDetection,
                   NULL,
                   &gEfiEndOfDxeEventGroupGuid,
                   &DisableNullDetectionEvent
                   );
      } else if (gDxeMps.NullPointerDetectionPolicy.Fields.DisableReadyToBoot) {
        Status = CoreCreateEventEx (
                   EVT_NOTIFY_SIGNAL,
                   TPL_NOTIFY,
                   DisableNullDetection,
                   NULL,
                   &gEfiEventReadyToBootGuid,
                   &DisableNullDetectionEvent
                   );
      }
    }

    ASSERT_EFI_ERROR (Status);
  }

  // MU_CHANGE END

  //
  // MSCHANGE START
  // Install protocol for validating Heap Guard if Heap Guard is turned on
  // Update to use memory protection settings HOB
  // if (PcdGet8(PcdHeapGuardPropertyMask)) {
  if (gDxeMps.HeapGuardPolicy.Data ||
      gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromFv ||
      gDxeMps.ImageProtectionPolicy.Fields.ProtectImageFromUnknown)
  {
    EFI_HANDLE  HgBmHandle = NULL;
    Status = CoreInstallMultipleProtocolInterfaces (
               &HgBmHandle,
               &gHeapGuardDebugProtocolGuid,
               &mHeapGuardDebug,
               NULL
               );
    DEBUG ((DEBUG_INFO, "Installed gHeapGuardDebugProtocolGuid - %r\n", Status));
  }

  // MSCHANGE END

  return;
}

/**
  Returns whether we are currently executing in SMM mode.
**/
STATIC
BOOLEAN
IsInSmm (
  VOID
  )
{
  BOOLEAN  InSmm;

  InSmm = FALSE;
  if (gSmmBase2 != NULL) {
    gSmmBase2->InSmm (gSmmBase2, &InSmm);
  }

  return InSmm;
}

/**
  Manage memory permission attributes on a memory range, according to the
  configured DXE memory protection policy.

  @param  OldType           The old memory type of the range
  @param  NewType           The new memory type of the range
  @param  Memory            The base address of the range
  @param  Length            The size of the range (in bytes)

  @return EFI_SUCCESS       If we are executing in SMM mode. No permission attributes
                            are updated in this case
  @return EFI_SUCCESS       If the the CPU arch protocol is not installed yet
  @return EFI_SUCCESS       If no DXE memory protection policy has been configured
  @return EFI_SUCCESS       If OldType and NewType use the same permission attributes
  @return other             Return value of gCpu->SetMemoryAttributes()

**/
EFI_STATUS
EFIAPI
ApplyMemoryProtectionPolicy (
  IN  EFI_MEMORY_TYPE       OldType,
  IN  EFI_MEMORY_TYPE       NewType,
  IN  EFI_PHYSICAL_ADDRESS  Memory,
  IN  UINT64                Length
  )
{
  UINT64  OldAttributes;
  UINT64  NewAttributes;

  //
  // The policy configured in Dxe NX Protection Policy // MU_CHANGE
  // does not apply to allocations performed in SMM mode.
  //
  if (IsInSmm ()) {
    return EFI_SUCCESS;
  }

  //
  // If the CPU arch protocol is not installed yet, we cannot manage memory
  // permission attributes, and it is the job of the driver that installs this
  // protocol to set the permissions on existing allocations.
  //
  if (gCpu == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Check if a DXE memory protection policy has been configured
  //
  // MU_CHANGE START Update to use memory protection settings HOB
  // if (PcdGet64 (PcdDxeNxMemoryProtectionPolicy) == 0) {
  if (!gDxeMps.NxProtectionPolicy.Data) {
    // MU_CHANGE END
    return EFI_SUCCESS;
  }

  //
  // Don't overwrite Guard pages, which should be the first and/or last page,
  // if any.
  //
  if (IsHeapGuardEnabled (GUARD_HEAP_TYPE_PAGE|GUARD_HEAP_TYPE_POOL)) {
    if (IsGuardPage (Memory)) {
      Memory += EFI_PAGE_SIZE;
      Length -= EFI_PAGE_SIZE;
      if (Length == 0) {
        return EFI_SUCCESS;
      }
    }

    if (IsGuardPage (Memory + Length - EFI_PAGE_SIZE)) {
      Length -= EFI_PAGE_SIZE;
      if (Length == 0) {
        return EFI_SUCCESS;
      }
    }
  }

  //
  // Update the executable permissions according to the DXE memory
  // protection policy, but only if
  // - the policy is different between the old and the new type, or
  // - this is a newly added region (OldType == EfiMaxMemoryType)
  //
  NewAttributes = GetPermissionAttributeForMemoryType (NewType);

  if (OldType != EfiMaxMemoryType) {
    OldAttributes = GetPermissionAttributeForMemoryType (OldType);
    if (OldAttributes == NewAttributes) {
      // policy is the same between OldType and NewType
      return EFI_SUCCESS;
    }
  } else if (NewAttributes == 0) {
    // newly added region of a type that does not require protection
    return EFI_SUCCESS;
  }

  return gCpu->SetMemoryAttributes (gCpu, Memory, Length, NewAttributes);
}
