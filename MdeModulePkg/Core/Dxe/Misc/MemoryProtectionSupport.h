/** @file
  Functionality supporting the updated Project Mu memory protections

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEMORY_PROTECTION_SUPPORT_H_
#define _MEMORY_PROTECTION_SUPPORT_H_

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
  );

/**
  Unprotect UEFI image (Project Mu Version).

  @param[in]  LoadedImage              The loaded image protocol
  @param[in]  LoadedImageDevicePath    The loaded image device path protocol
**/
VOID
UnprotectUefiImageMu (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage,
  IN EFI_DEVICE_PATH_PROTOCOL   *LoadedImageDevicePath
  );

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
  );

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
  );

/**
  Sets the NX compatibility global to FALSE so future checks to
  IsSystemNxCompatible() will return FALSE.
**/
VOID
EFIAPI
TurnOffNxCompatibility (
  VOID
  );

/**
  Returns TRUE if TurnOffNxCompatibility() has never been called.
**/
BOOLEAN
EFIAPI
IsSystemNxCompatible (
  VOID
  );

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
  );

#endif
