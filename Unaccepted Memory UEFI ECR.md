# Unified Extensible Firmware Interface Engineering Change Request (ECR)
SPDX-License-Identifier: CC-BY-4.0
# ***Draft for Review***

## Title: Unaccepted Memory Type Addition to EFI_MEMORY_TYPE
Document: UEFI V2.9

Sponsors:<br>
***Chris Oo, Microsoft***<br>
***Jon Lange, Microsoft***<br>
***Sean Brogan, Microsoft***<br>
***Brijesh Singh, AMD***<br>
***Thomas Lendacky, AMD***<br>
***Jiewen Yao, Intel***<br>
<br>
**Submission for Review Date: x/xx/2020**<br>
**Review Approval Date: x/xx/2020**<br>
**Submission for Technical Editing Date: x/xx/2020**<br>
**Submission for Draft Review Date: xx/xx/2020**<br>
**Verification Date: x/xx/2020**<br>
**Verifiers: TBD**<br>

## Summary
### Summary of Change
This ECR introduces a new memory type to the EFI_MEMORY_TYPE that is to be used to represent unaccepted guest memory to OS Loaders.

### Benefits of the Change
Newer virtual machine architectures such as AMD SEV-SNP introduce the concept of memory acceptance to the guest address space, offering additional protection against virtual machine host components. In order for the guest to utilize any memory offered by the host, the guest must first indicate to the underlying architecture that it wishes to accept the memory allowing the architecture to help prevent different classes of attacks.<br>

This change enables the firmware running within the guest to indicate to the OS loader which portions of memory have been already accepted by the firmware, and which regions of the physical address space have not been accepted. Additionally, this change allows the firmware to be flexible on how much of the guest memory space it must accept before handing off control to the OS loader. Without this change, firmware would be required to accept all of the usable physical address space on behalf of the OS loader, as the expectation by the OS loader for existing memory types is that the memory described in the EFI memory map are accepted and usable.<br>

Instead of the normal feedback channels, we ask that all feedback on this ECR be given on this Github pull request, https://github.com/microsoft/mu_basecore/pull/66. The most up to date version of this ECR can be found there.<br>

An example of a platform that uses this change to report unaccepted memory to an OS can be found in the following OVMF commit, https://github.com/AMDESE/ovmf/commit/793a755795a3580b7ee96d9478992767da331fb3.<br>

### References

AMD APM Volume 2, Section 15.36 https://www.amd.com/system/files/TechDocs/24593.pdf <br>
SEV Firmware Specification https://www.amd.com/system/files/TechDocs/56860.pdf <br>
Github PR for comments https://github.com/microsoft/mu_basecore/pull/66 <br>

## Detailed Description of the Change and Special Instructions

### Summary of Changes:
- Update Section 2.3.4 x64 Platforms to clarify unaccepted memory pagetable mappings are implementation defined
- Update Table 29 & 30 to add new type
- Update Section 7.2 AllocatePages Related Definitions
- Add entry describing unaccepted memory to Appendix R - Glossary

~~Text Removed~~,  Text Added ***Text Added*** ,  New Sections ***New Section***, Discussion/Mode Edits needed ***DME***

### 2.3.4 x64 Platforms
All functions are called with the C language calling convention. See Section 2.3.4.2 for more detail.<br>

During boot services time the processor is in the following execution mode:
- Uniprocessor, as described in chapter 8.4 of:
    - Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3, System Programming Guide, Part 1, Order Number: 253668-033US, December 2009
    - See “Links to UEFI-Related Documents” (http://uefi.org/uefi) under the heading “Intel Processor Manuals”.
- Long mode, in 64-bit mode
- Paging mode is enabled and any memory space defined by the UEFI memory map is identity mapped (virtual address equals physical address), although the attributes of certain regions may not have all read, write, and execute attributes or be unmarked for purposes of platform protection. The mappings to other regions ***Text Added***,  such as those for unaccepted memory,***Text Added*** are undefined and may vary from implementation to implementation.
- ...

#### 7.2 Memory Allocation Services
**Table 29. Memory Type Usage before ExitBootServices()**<br>
| Mnemonic                | Description                                                                                                                                                                                                                                                          |
|-------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| EfiReservedMemoryType   | Not usable.                                                                                                                                                                                                                                                          |
| ...                     | ...                                                                                                                                                                                                                                                                  |
| EfiUnacceptedMemoryType ***Text Added*** | A memory region that represents unaccepted memory, that must be accepted by the guest before it can be used. Unless otherwise noted, all other EFI memory types are accepted. For platforms that support unaccepted memory, all unaccepted valid memory will be reported as unaccepted in the memory map. Unreported physical address ranges must be treated as not-present memory. ***Text Added*** |

**Table 30. Memory Type Usage after ExitBootServices()**<br>
| Mnemonic                | Description                                                                                                                                                                                                                                                          |
|-------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| EfiReservedMemoryType   | Not usable.                                                                                                                                                                                                                                                          |
| ...                     | ...                                                                                                                                                                                                                                                                  |
| EfiUnacceptedMemoryType ***Text Added*** | A memory region that represents unaccepted memory, that must be accepted by the guest before it can be used. Unless otherwise noted, all other EFI memory types are accepted. For platforms that support unaccepted memory, all unaccepted valid memory will be reported as unaccepted in the memory map. Unreported physical address ranges must be treated as not-present memory. ***Text Added*** |
<br>

**EFI_BOOT_SERVICES.AllocatePages()**<br>
**Summary**<br>
Allocates memory pages from the system.<br>
**Prototype**<br>
``` c
typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_PAGES) ( IN EFI_ALLOCATE_TYPE        Type,
    IN EFI_MEMORY_TYPE          MemoryType,
    IN UINTN                    Pages,
    IN OUT EFI_PHYSICAL_ADDRESS *Memory
    );
```
**Parameters**<br>

**Type**<br>
The type of allocation to perform. See “Related Definitions.”<br>

**MemoryType**<br>
The type of memory to allocate. The type EFI_MEMORY_TYPE is defined in “Related Definitions” below. These memory types are also described in more detail in Table 28 and Table 29. Normal  allocations (that is, allocations by any UEFI application) are of type  EfiLoaderData. MemoryType values in the range  0x70000000..0x7FFFFFFF are reserved for OEM use. MemoryType values in the range 0x80000000..0xFFFFFFFF are reserved for use by UEFI OS loaders that are provided by operating system vendors.<br>

**Pages**<br>
The number of contiguous 4 KiB pages to allocate.<br>

**Memory**<br>
Pointer to a physical address. On input, the way in which the address is used depends on the value of Type. See “Description” for more information. On output the address is set to the base of the page range that was allocated. See “Related Definitions.”<br>

Note: UEFI Applications, UEFI Drivers, and UEFI OS Loaders must not allocate memory of types EfiReservedMemoryType ~~and~~ , EfiMemoryMappedIO, and EfiUnacceptedMemoryType. ***Text Added***<br>

**Related Definitions**<br>
``` c
//*******************************************************
//EFI_ALLOCATE_TYPE
//*******************************************************
// These types are discussed in the “Description” section below.
typedef enum {
AllocateAnyPages,
AllocateMaxAddress,
AllocateAddress,
MaxAllocateType
 } EFI_ALLOCATE_TYPE;

//*******************************************************
//EFI_MEMORY_TYPE
//*******************************************************
// These type values are discussed in Table 28 and Table 29.
typedef enum {
  EfiReservedMemoryType,
  EfiLoaderCode,
  EfiLoaderData,
  EfiBootServicesCode,
  EfiBootServicesData,
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiConventionalMemory,
  EfiUnusableMemory,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS,
  EfiMemoryMappedIO,
  EfiMemoryMappedIOPortSpace,
  EfiPalCode,
  EfiPersistentMemory,
  EfiUnacceptedMemory, // Text Added
  EfiMaxMemoryType
} EFI_MEMORY_TYPE;
//*******************************************************
//EFI_PHYSICAL_ADDRESS
//*******************************************************
typedef UINT64 EFI_PHYSICAL_ADDRESS;
```
...

**Status Codes Returned**<br>
| EFI_SUCCESS            | The requested pages were allocated.                                        |
|------------------------|----------------------------------------------------------------------------|
| EFI_OUT_OF_RESOURCES   | The pages could not be allocated.                                          |
| EFI_INVALID_PARAMETER  | Type is not AllocateAnyPages or AllocateMaxAddress or AllocateAddress.     |
| EFI_INVALID_PARAMETER  | MemoryType is in the range EfiMaxMemoryType..0x6FFFFFFF.                   |
| EFI_INVALID_PARAMETER  | MemoryType is EfiPersistentMemory or EfiUnacceptedMemory. ***Text Added*** |
| EFI_INVALID_PARAMETER  | Memory is NULL.                                                            |
| EFI_NOT_FOUND          | The requested pages could not be found.                                    |

#### Appendix R - Glossary<br>
...<br>
***Text Added***<br>
**Unaccepted Memory** <br>

Some Virtual Machine platforms, such as AMD SEV-SNP, introduce the concept of memory acceptance, requiring memory to be accepted before it can be used by the guest. This protects against a class of attacks by the virtual machine platform.<br>
***Text Added***

### Special Instructions:
#### Discussions / Opens