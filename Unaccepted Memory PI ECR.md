# Unified Extensible Firmware Interface Engineering Change Request (ECR)
SPDX-License-Identifier: CC-BY-4.0
# ***Draft for Review***

## Title: Unaccepted Memory Type Addition to PI HOB Resource types and GcdMemoryType
Document: PI V1.8

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
This ECR supports the introduction of a new memory type, unaccepted memory, with updates to the corresponding PI HOB Resource types and GcdMemoryTypes. See the corresponding ECR introducing a new memory type to the EFI_MEMORY_TYPE for more information on unaccepted memory.

### Benefits of the Change
This change allows PEI to reflect the current state of accepted memory to DXE so that it can be used correctly, along with reporting it to the OS loader.

### References

TODO - How to reference other ECR?

AMD APM Volume 2, Section 15.36 https://www.amd.com/system/files/TechDocs/24593.pdf <br>
SEV Firmware Specification https://www.amd.com/system/files/TechDocs/56860.pdf <br>
Github PR for comments https://github.com/microsoft/mu_basecore/pull/66 <br>

## Detailed Description of the Change and Special Instructions

### Summary of Changes:
- Update Section 5.5 of Platform Initialization Specification, Vol. 3, HOB Code definitions
- TODO - Changes for GCD

~~Text Removed~~,  Text Added ***Text Added*** ,  New Sections ***New Section***, Discussion/Mode Edits needed ***DME***

#### 5.5  Resource Descriptor HOB
...

**Related Definitions**<br>
There can only be a single ResourceType field, characterized as follows.
``` c
//*********************************************************
// EFI_RESOURCE_TYPE
//*********************************************************

typedef UINT32 EFI_RESOURCE_TYPE;

#define EFI_RESOURCE_SYSTEM_MEMORY           0x00000000
#define EFI_RESOURCE_MEMORY_MAPPED_IO        0x00000001
#define EFI_RESOURCE_IO                      0x00000002
#define EFI_RESOURCE_FIRMWARE_DEVICE         0x00000003
#define EFI_RESOURCE_MEMORY_MAPPED_IO_PORT   0x00000004
#define EFI_RESOURCE_MEMORY_RESERVED         0x00000005
#define EFI_RESOURCE_IO_RESERVED             0x00000006
#define EFI_RESOURCE_MEMORY_UNACCEPTED       0x00000007 // Text Added
#define EFI_RESOURCE_MAX_MEMORY_TYPE         0x00000008 // Text Edited
```

The following table describes the fields listed in the above definition.
| EFI_RESOURCE_SYSTEM_MEMORY | Memory that persists out of the HOB producer phase.                    |
|------------------------|----------------------------------------------------------------------------|
| EFI_RESOURCE_MEMORY_MAPPED_IO   | Memory-mapped I/O that is programmed in the HOB producer phase.                                         |
| EFI_RESOURCE_IO  | Processor I/O space.     |
| EFI_RESOURCE_FIRMWARE_DEVICE  | Memory-mapped firmware devices.                   |
| EFI_RESOURCE_MEMORY_MAPPED_IO_PORT  | Memory that is decoded to produce I/O cycles. |
| EFI_RESOURCE_MEMORY_RESERVED  | Reserved memory address space.                                                            |
| EFI_RESOURCE_IO_RESERVED          | Reserved I/O address space.                                   |
| EFI_RESOURCE_MEMORY_UNACCEPTED          | Memory that is unaccepted. ***Text Added***                              |
| EFI_RESOURCE_MAX_MEMORY_TYPE      | Any reported HOB value of this type or greater should be deemed illegal. This value could increase with successive revisions of this specification, so the “illegality” will also be based upon the revision field of the PHIT HOB |

### Special Instructions:
#### Discussions / Opens
