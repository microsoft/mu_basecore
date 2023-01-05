# Memory Protections

The Memory Protection Settings add safety functionality such as page and pool guards, stack guard and
null pointer detection. The settings are split between MM and DXE environments for modularity.
The target audience for this doc has intermediate knowledge of systems programming and working with EDK II.

## Useful Terms and Concepts (Linked in Text if Used)

### Option ROM
A driver that interfaces between BIOS services and hardware.

### Boot Strap Processor (BSP)
The bootstrap processor (BSP) handles initialization procedures for the system as a whole. These procedures include
checking the integrity of memory, identifying properties of the system logic and starting the remaining processors.

### Application Processor (AP)
A system processor used for processing signals in embedded systems.

### Boot Loader
Places into working memory the required resources for runtime.

### Read Only (RO)
A bit used to mark certain areas of memory as non-writeable.

### No eXecute/eXecute Never/eXecute Disable Attribute (NX/XN/XD)
A bit used to mark certain areas of memory as non-executable. NX is a term usually used by AMD whereas
XD is used by Intel and XN by Qualcomm. The only difference between NX, XD, and XN are their names.

### Physical/Page Address Extension
A memory management feature in x86 architecture which defines a page table heirarchy with table entries of 64 bits
allowing CPUs to directly address physical address spaces larger than 32 bits (4 GB).

### Model-specific Register (MSR)
Any of the various control registers in the x86 instruction set used for debugging, execution tracing, performance
monitoring and CPU feature toggling.

### EndOfDxe
The point at which the driver execution (DXE) phase has ended and all drivers provided by the mfg (as part of the
built-in ROM or loaded directly from another driver) should be loaded now, or else they have failed their dependency
expressions. UEFI drivers and OpROMs have not yet been started.

### Page Fault Exception (#PF)
An exception raised when EDK II code attempts to access memory which is not present or settings for the page make
it invisible.

### Task State Segment (TSS)
A structure on x86-based CPUs which holds information about a unit of execution.

### Cpu Context Dump
A routine which prints to serial out the module in which the fault occurred, type of fault which occurred and
contents of each CPU register.

### Memory Management Unit (MMU)
Hardware on a CPU which is primarily responsible for translating Virtual Memory addresses to Physical ones.

### Translation Lookaside Buffer (TLB)
A memory cache which is part of the CPUs [MMU](#memory-management-unit-mmu) and stores translations of Virtual
Memory to Physical Memory. The addresses stored in the TLB are dictated by some algorithm intended to decrease
amount of memory accesses for which the address translation is outside the TLB.

### NXCOMPAT
NXCOMPAT is a DLL flag which indicates that the loaded binary expects memory allocations to have the
[NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd)
attribute applied which will be removed when the code is copied into the memory. NXCOMPAT images
should also apply [RO](#read-only-ro) to the memory before execution to ensure that, at any point in time,
all memory is executable or read-only but not both.

### Nonstop Mode
In the case of Non-Stop mode being enabled for either [HeapGuardPolicy](#heapguardpolicy) or
[NullPointerDetectionPolicy](#nullpointerdetectionpolicy), two exception handlers are registered.
The first handler runs whenever the heap guard or null pointer page absences trigger a
[#PF](#page-fault-exception-pf). If Non-Stop mode is enabled for this type of
[#PF](#page-fault-exception-pf), the absent page(s) are temporarily set to be present and a
[Cpu Context Dump](#cpu-context-dump) is run after which the second exception handler registered
(the debug handler) is run.
The debug handler sets the page to be present and clears the [TLB](#translation-lookaside-buffer-tlb) to
remove the current translation for the page which caused the [#PF](#page-fault-exception-pf). Once these
two handlers have run, code execution continues.

### Stack Cookies
A stack cookie (also called stack canary) is an integer placed in memory just before the stack return pointer. Most buffer overflows overwrite memory from lower to higher memory addresses, so in order to overwrite the return pointer (and thus take control of the process) the canary value must also be overwritten. This value is checked to make sure it has not changed before a routine uses the return pointer on the stack.

## **Null Pointer Detection**

### Summary
Pages are allocated in 4KB chunks. This policy marks the 4KB page at the NULL address to be not present to
detect NULL pointer references in Dxe and/or platform MM.

### Dxe Available Settings

- UefiNullDetection  - Enable NULL pointer detection for DXE
- DisableEndOfDxe    - Disable NULL pointer detection just after [EndOfDxe](#endofdxe)
- DisableReadyToBoot - Disable NULL pointer detection just after ReadyToBoot

### MM Available Settings
If NullPointerDetectionPolicy is TRUE, the present bit for the NULL page is cleared for SMM address space.

## **Image Protection Policy**

### Summary

This policy enables an image to be protected by DxeCore if it is page-aligned, meaning
the code sections become read-only, and the data sections become non-executable. **This policy is only**
**available in the DXE environment.**

There are 3 environment assumptions for enabling image protection:

1. The PE code section and data sections are not merged. If those 2 sections are merged, a
[#PF](#page-fault-exception-(aka-#pf) exception might be generated because the CPU may try to write read-only
data in data section or execute an [NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) instruction
in the code section.

2. The PE image can be protected if it is page aligned. This feature should **NOT** be used if there is any
self-modifying code in the code region.

3. A platform may not disable [NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) in the DXE phase.
If a platform disables [NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) in the DXE
phase, the x86 page table will become invalid because the
[NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) bit in the page table entry becomes a RESERVED bit and a
[#PF](#page-fault-exception-pf) exception will be generated. If a platform wants to disable the
[NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) bit, it must occur in the PEI phase.

### Overhead
**O(n)** time and space overhead. Each image requires a 6K attributes header, so if there are **n** images the
space overhead will be 6K\*n and thus O(n) time to populate the headers. In most cases the number of
images is in the order of hundreds making this feature relatively inexpensive.

Because this feature requires aligned images, there will likely be an increased size footprint for each image.

### Available Settings

- FromUnknown                  - Protect images from unknown devices
- FromFv                       - Protect images from firmware volume
- RaiseErrorIfProtectionFails  - If set, images which fail to be protected will be unloaded. This excludes failure
because CPU Arch Protocol has not yet been installed
- BlockImagesWithoutNxFlag     - [NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) may be set on
EfiLoaderCode, EfiBootServicesCode, and EfiRuntimeServicesCode if the setting for each is active in the
[NX Memory Protection Policy](#nx-memory-protection-policy). However, if the image does not indicate support for
[NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) via the [NXCOMPAT](#nxcompat) DLL flag
in the header, the logic will cease to set the [NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd)
attribute on allocations of memory of type EfiLoaderCode, EfiBootServicesCode, and/or EfiRuntimeServicesCode. Using
the BlockImagesWithoutNxFlag setting in this policy will prevent images which don't support
[NXCOMPAT](#nxcompat) from loading and thus cause
[NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) to continue to be applied to allocations of a
code memory type based on their respective setting in the [NX Memory Protection Policy](#nx-memory-protection-policy).

## **NX Memory Protection Policy**

### Summary

This policy sets the [NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) attribute to memory of the
associated memory type. **This setting does not apply to MM.**

Every active memory type will be mapped as non-executable.
**Note** that a portion of memory will only be marked as
non-executable once the CPU Architectural Protocol is available. **Also note** that in order
to enable Data Execution Protection, the operating system needs to set the
[IA32_EFER.NXE](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) bit in the IA32_EFER
[MSR](#model-specific-register-msr),
and then set the [XD](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) bit in the CPU
[PAE](#physicalpage-address-extension)
page table.

### Overhead
**O(n)** time where n is the number of memory mapped regions. The number of actual set bits beyond one is
inconsequential because every memory region must be checked if at least one bit is set. There is no extra space
complexity due to using the already present [NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) bit.

### Available Settings

- EfiReservedMemoryType
- EfiLoaderCode - If an image does not indicate support for
[NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) via the [NXCOMPAT](#nxcompat) DLL flag, the logic will
cease to set the [NX](#no-executeexecute-neverexecute-disable-attribute-nxxnxd) attribute on allocations of memory
this type. Using the BlockImagesWithoutNxFlag in the [Image Protection Policy](image-protection-policy) will prevent
images which don't support [NXCOMPAT](#nxcompat).
- EfiLoaderData
- EfiBootServicesCode - Same note as EfiLoaderCode.
- EfiBootServicesData
- EfiRuntimeServicesCode - Same note as EfiLoaderCode.
- EfiRuntimeServicesData
- EfiConventionalMemory
- EfiUnusableMemory
- EfiACPIReclaimMemory
- EfiACPIMemoryNVS
- EfiMemoryMappedIO
- EfiMemoryMappedIOPortSpace
- EfiPalCode
- EfiPersistentMemory
- OEMReserved
- OSReserved

## **Page Guards**

### Summary

The HeapGuardPageType policy implements guard pages on the specified memory types to detect heap overflow. If a bit
is set, a guard page will be added before and after the
corresponding type of page allocated if there's enough free pages for all of them. Guard pages are set to NOT PRESENT so any attempt
to access them will cause a [#PF](#page-fault-exception-pf). The system will do its best to ensure that only one guard page separates two allocated pages to avoid wasted space.

### Overhead
**O(n)** time where n is the number of page allocations/frees. Because there are 2 extra pages allocated for
every call to AllocatePages(), **O(n)** space is also required.

### Available Settings for DXE and MM

- EfiReservedMemoryType
- EfiLoaderCode
- EfiLoaderData
- EfiBootServicesCode
- EfiBootServicesData
- EfiRuntimeServicesCode
- EfiRuntimeServicesData
- EfiConventionalMemory
- EfiUnusableMemory
- EfiACPIReclaimMemory
- EfiACPIMemoryNVS
- EfiMemoryMappedIO
- EfiMemoryMappedIOPortSpace
- EfiPalCode
- EfiPersistentMemory
- OEMReserved
- OSReserved

## **Pool Guards**

### Summary

The HeapGuardPoolType policy is essentially the same as HeapGuardPageType policy.
For each active memory type, a guard page will be added just before and after the portion of memory which the
allocated pool occupies. The only added complexity comes when the allocated pool is not a
multiple of the size of a page. In this case, the pool must align with either the head or tail guard page, meaning
either overflow or underflow can be caught consistently but not both. The head/tail alignment is set in
[HeapGuardPolicy](#heapguardpolicy) - look there for additional details.

### Overhead
Same as above: **O(n)** time and space for same reasons as [HeapGuardPageType](#heapguardpagetype). **Note**
that this functionality requires creating guard pages, meaning that for n allocations, 4k \* (n + 1) (assuming
each of the n pools is adjacent to another pool) additional space is required.

### Available Settings for DXE and MM

- EfiReservedMemoryType
- EfiLoaderCode
- EfiLoaderData
- EfiBootServicesCode
- EfiBootServicesData
- EfiRuntimeServicesCode
- EfiRuntimeServicesData
- EfiConventionalMemory
- EfiUnusableMemory
- EfiACPIReclaimMemory
- EfiACPIMemoryNVS
- EfiMemoryMappedIO
- EfiMemoryMappedIOPortSpace
- EfiPalCode
- EfiPersistentMemory
- OEMReserved
- OSReserved

## **HeapGuardPolicy**

### Summary

While the above two policies ([Pool Guards](#pool-guards) and
[Page Guards](#page-guards) act as a
switch for each protectable memory type, this policy is an enable/disable switch for those
two policies (ex. if UefiPageGuard is unset, page guards in DXE are inactive regardless
of the [Page Guard](#page-guards) settings).

The only aspect of this policy which should be elaborated upon is Direction. Direction dictates whether
an allocated pool which does not fit perfectly into a multiple of pages is aligned to the head or tail guard.
The following Figure shows examples of the two:

![Heap Guard Pool Alignment Image](alignment_mu.PNG)

On free the pool head/tail is checked to ensure it was not overwritten while the not-present page will trigger a page fault immediately.

### Overhead
Overhead is same as [Page Guards](#page-guards) and [Pool Guards](#pool-guards).

### DXE Available Settings

- UefiPageGuard - Enable UEFI page guard
- UefiPoolGuard - Enable UEFI pool guard
- UefiFreedMemoryGuard - Enable Use-After-Free memory detection
- Direction - Specifies the direction of Guard Page for Pool Guard. If 0, the returned
pool is near the tail guard page. If 1, the returned pool is near the head guard page. The
default value for this is 0

### MM Available Settings

- SmmPageGuard - Enable SMM page guard
- SmmPoolGuard - Enable SMM pool guard
- Direction - Specifies the direction of Guard Page for Pool Guard. If 0, the returned
pool is near the tail guard page. If 1, the returned pool is near the head guard page. The
default value for this is 0

## **CPU Stack Guard**

The CpuStackGuard policy indicates if UEFI Stack Guard will be enabled.

The stack guards add two additional pages to the bottom of the stack(s). The first page is simply the guard page
which is set to not present. When a page fault occurs, the current stack address is invalid and so it is not possible to push the error code and
architecture status onto the current stack. Because of this, there is a special "Exception Stack" or "Known Good Stack" which is the second page placed at the bottom of the stack. This page is
reserved for use by the exception handler and ensures that a valid stack is always present when an exception occurs
for error reporting.

### A note on SMM
An equivalent SMM stack guard feature is contained in
[PiSmmCpuDxeSmm](https://github.com/tianocore/edk2/tree/master/UefiCpuPkg/PiSmmCpuDxeSmm) and is not dictated
by this policy.

**Note** that the UEFI
stack protection starts in DxeIpl, because the region is fixed, and requires
[PcdDxeIplBuildPageTables](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/MdeModulePkg.dec) to be
TRUE. In Project Mu, we have hard-coded CpuStackGuard to be TRUE in PEI phase, so we always set up a switch
stack, clear the PRESENT bit in the page table for the guard page of
the Boot Strap Processor stack, and build the page tables. However, the stack switch handlers will still only be
installed in DXE phase if CpuStackGuard is TRUE.

### Overhead
**O(1)** time and space.

**Setting:**

- If TRUE, UEFI Stack Guard will be enabled.

## **Stack Cookies**
[Stack Cookies](#stack-cookies) enable protection of the stack return pointer. The stack cookie value is specific to each loaded image and is generated at random on image load. Stack cookies are enabled at compile time, but if this
setting is FALSE the interrupts generated by stack cookie check failures should be ignored.

**Setting:**

- If TRUE, stack cookie failures will cause a warm reset. If FALSE, stack cookie failure interrupts will be ignored.

## **How to Set the Memory Protection Policy**

For DXE settings, add the following to the platform DSC file:

```C
[LibraryClasses.Common.DXE_DRIVER, LibraryClasses.Common.DXE_CORE, LibraryClasses.Common.UEFI_APPLICATION]
  DxeMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLib/DxeMemoryProtectionHobLib.inf
```

For MM settings, add the following to the platform DSC file if the platform utilizes SMM:

```C
[LibraryClasses.common.SMM_CORE, LibraryClasses.common.DXE_SMM_DRIVER]
  MmMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLib/SmmMemoryProtectionHobLib.inf
```

**or** the following if the platform utilizes Standalone MM:

```C
[LibraryClasses.common.MM_CORE_STANDALONE, LibraryClasses.common.MM_STANDALONE]
  MmMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLib/StandaloneMmMemoryProtectionHobLib.inf
```

Create the HOB entry in any PEI module by adding the include:

```C
#include <Guid/DxeMemoryProtectionSettings.h>
#include <Guid/MmMemoryProtectionSettings.h>
```

and somewhere within the code doing something like:

```C
  DXE_MEMORY_PROTECTION_SETTINGS  DxeSettings;
  MM_MEMORY_PROTECTION_SETTINGS   MmSettings;

  DxeSettings = (DXE_MEMORY_PROTECTION_SETTINGS)DXE_MEMORY_PROTECTION_SETTINGS_DEBUG;
  MmSettings  = (MM_MEMORY_PROTECTION_SETTINGS)MM_MEMORY_PROTECTION_SETTINGS_DEBUG;

  BuildGuidDataHob (
    &gDxeMemoryProtectionSettingsGuid,
    &DxeSettings,
    sizeof (DxeSettings)
    );

  BuildGuidDataHob (
    &gMmMemoryProtectionSettingsGuid,
    &MmSettings,
    sizeof (MmSettings)
    );
```

This will also require you to add gMemoryProtectionSettingsGuid under the Guids section in the relevant INF.

If you want to deviate from one of the settings profile definitions in DxeMemoryProtectionSettings.h
and/or MmMemoryProtectionSettings, it is recommended
that you start with the one which most closely aligns with your desired settings and update from there in a
manner similar to below:

```C
  MmSettings.HeapGuardPolicy.Fields.MmPageGuard                    = 0;
  MmSettings.HeapGuardPolicy.Fields.MmPoolGuard                    = 0;
  DxeSettings.ImageProtectionPolicy.Fields.ProtectImageFromUnknown = 1;
```

before building the HOB.

## **Memory Protection Special Regions**

Memory protection is not activated until the CPU Architecture Protocol has been installed because the
protocol facilitates access to the attribute manipulation functions in CpuDxe which update the translation/page tables.
Many allocations and image loads will have occurred by the time the protocol is published so careful accounting
is required to ensure appropriate attributes are applied. An event notification triggered on the CPU Architecture
Protocol installation will combine the GCD and EFI memory maps to create a full map of memory for use internally
by the memory protection initialization logic. Because image memory is allocated for the entire image and not each
section (code and data), the images are separated within
the combined map so NX can be applied to data regions and RO can be applied to code regions. After breaking
up the map so each DXE image section has its own descriptor, every non-image descriptor will have its attributes
set based on its EFI memory type. There are cases where the platform will want to attributes applied to
a region of memory which is different than what would be applied based on its EFI memory type. In this case,
platforms can utilize the Memory Protection Special Region interface to specify regions which should have specific
attributes applied during memory protection initialization.

### In PEI:

```C
#include <Guid/MemoryProtectionSpecialRegionGuid.h>

MEMORY_PROTECTION_SPECIAL_REGION SpecialRegion;
SpecialRegion.Start       = 0x1000;
SpecialRegion.Length      = 0x1000;
SpecialRegion.Attributes  = EFI_MEMORY_RO;

BuildGuidDataHob (
    &gMemoryProtectionSpecialRegionHobGuid,
    &SpecialRegion,
    sizeof (SpecialRegion)
    );

```


### In DXE:

```C
#include <Protocol/MemoryProtectionSpecialRegionProtocol.h>

MEMORY_PROTECTION_SPECIAL_REGION_PROTOCOL *SpecialRegionProtocol = NULL;
EFI_PHYSICAL_ADDRESS  BufferStart       = 0x1000;
UINT64                BufferLength      = 0x1000;
UINT64                BufferAttributes  = EFI_MEMORY_RO;

Status = gBS->LocateProtocol (&gMemoryProtectionSpecialRegionProtocolGuid, NULL, (VOID **)&SpecialRegionProtocol);
ASSERT_EFI_ERROR (Status);
if (SpecialRegionProtocol != NULL) {
  Status = SpecialRegionProtocol->AddSpecialRegion (
                                    BufferStart,
                                    BufferLength,
                                    BufferAttributes
                                    );

  ASSERT_EFI_ERROR (Status);
}
```

These special regions also may be used during paging audit tests which check if the page table has secure
attributes. For example, an existing test checks to see if there are any Read/Write/Execute memory regions and fail
if true. During this test, if a Read/Write/Execute region is found, it will be checked against the special regions
and a test failure will not be emitted if the page attributes are consistent with the attributes identified in
the overlapping special region.