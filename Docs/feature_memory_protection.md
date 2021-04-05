# Memory Protections

The Memory Protection bitwise PCDs add safety functionality such as page and pool guards, stack guard and 
null pointer detection. The target audience for this doc has intermediate knowledge of systems programming and working 
with EDK II. 

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

### Non-eXecute/eXecute Disable Bit (NX/DX)
A bit used to mark certain areas of memory as non-executable. NX is a term usually used by AMD whereas 
DX is used by Intel. The only difference between NX and DX are their names.

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

### Page Fault Exception (AKA #PF)
An exception raised when EDK II code attempts to access memory which is not present or settings for the page make 
it invisible.

### Task State Segment (TSS)
A structure on x86-based CPUs which holds information about a unit of execution.

### Cpu Context Dump
A routine which prints to serial out the module in which the fault occurred, type of fault which occurred and 
contents of each CPU register.

### Memory Management Unit (MMU)
Hardware on a CPU which is primarily responsible for translating Virtual Memory addresses to Physical ones

### Translation Lookaside Buffer (AKA TLB)
A memory cache which is part of the CPUs [MMU](#Memory-Management-Unit-(MMU)) and stores translations of Virtual 
Memory to Physical Memory. The addresses stored in the TLB are dictated by some algorithm intended to decrease 
amount of memory accesses for which the address translation is outside the TLB. 

### Non-Stop Mode
In the case of Non-Stop mode being enabled for either [PcdHeapGuardPropertyMask](#PcdHeapGuardPropertyMask) or 
[PcdNullPointerDetectionPropertyMask](#PcdNullPointerDetectionPropertyMask), two exception handlers are registered. 
The first handler runs whenever the heap guard or null pointer page absences trigger a 
[#PF](#Page-Fault-Exception-(AKA-#PF)). If Non-Stop mode is enabled for this type of 
[#PF](#Page-Fault-Exception-(AKA-#PF)), the absent page(s) are temporarily set to be present and a 
[Cpu Context Dump](#Cpu-Context-Dump) is run after which the second exception handler registered 
(the debug handler) is run. 
The debug handler sets the page to be present and clears the [TLB](#Translation-Lookaside-Buffer-(AKA-TLB)) to 
remove the current translation for the page which caused the [#PF](#Page-Fault-Exception-(AKA-#PF)). Once these 
two handlers have run, code execution continues.

## PcdNullPointerDetectionPropertyMask

### Summary
Pages are allocated in 4Kb chunks. This PCD marks the first 4Kb page to be not present to
detect NULL pointer references in both/either UEFI and SMM.

### Implementation Details

If BIT0 is set, the present bit for the NULL page is cleared for UEFI address space in
https://github.com/tianocore/edk2/tree/master/MdeModulePkg/Core/DxeIplPeim (A PEIM module which is the last
executed in PEI phase and loads DXE Core from the Firmware Volume). Therefore, any NULL accesses prior to this 
point will not cause a [#PF](#Page-Fault-Exception-(AKA-#PF)).

If BIT1 is set, the present bit for the NULL page is cleared for SMM address space in 
https://github.com/tianocore/edk2/tree/master/UefiCpuPkg/PiSmmCpuDxeSmm (THE SMM initialization driver).

If Bit 7 is set, the present bits for the NULL page in UEFI and SMM will be set once execution reaches 
[EndOfDxe](#EndOfDxe). This is a workaround in order to skip unfixable NULL pointer access issues detected in 
legacy [Option ROM](#Option-ROM) or [boot loaders](#boot-loader).

### Overhead
**O(1)** time and space overhead because there is a constant number of NULL pages.

**Available Bits:**

- BIT0  - Enable NULL pointer detection for UEFI
- BIT1  - Enable NULL pointer detection for SMM
- BIT6  - Enable [Non-Stop Mode](#Non-Stop-Mode)
- BIT7  - Disable NULL pointer detection just after [EndOfDxe](#EndOfDxe)

## PcdImageProtectionPolicy

### Summary

If a bit is set, the image will be protected by DxeCore if it is page-aligned.
The code section becomes read-only, and the data section becomes non-executable. If a bit (BIT1 and/or BIT2) 
is clear, nothing will be done to image code/data sections.

### Implementation Details in UEFI

There are 3 environment assumptions for enabling image protection:

1. The PE code section and data sections are not merged. If those 2 sections are merged, a 
[#PF](#Page-Fault-Exception-(AKA-#PF)) exception might be generated because the CPU may try to write read-only 
data in data section or execute a [NX](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) instruction in the code section.

2. The PE image can be protected if it is page aligned. This feature should **NOT** be used if there is any 
self-modifying code in the code region.

3. A platform may not disable the XD in the DXE phase. If a platform disables the 
[XD](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) in the DXE phase, the x86 page table will become invalid because 
the [XD](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) bit in the page table becomes a RESERVED bit and a 
[#PF](#Page-Fault-Exception-(AKA-#PF)) exception will be generated. If a platform wants to disable the 
[XD](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) bit, it must happen in the PEI phase.

In EDK II, the DXE core image services calls 
[ProtectUefiImage()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c) on 
image load 
and [UnprotectUefiImage()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Image/Image.c) on image 
unload. On load, 
[ProtectUefiImage()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c) 
calls 
[GetUefiImageProtectionPolicy()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c) 
to check the image source and protection policy, and parses PE alignment. If all checks pass, 
[SetUefiImageProtectionAttributes()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c) 
calls 
[SetUefiImageMemoryAttributes()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c). 
Finally, gCpu->[CpuSetMemoryAttributes()](https://github.com/tianocore/edk2/blob/master/UefiCpuPkg/CpuDxe/CpuDxe.c) 
sets EFI_MEMORY_XP (eXecution Protection) or EFI_MEMORY_RO (Read Only) for the new loaded image , or clears the 
protection in the case of an unloaded image. When the CPU driver gets the memory attribute setting request, it 
updates page table.

The X86 CPU driver calls 
[CpuSetMemoryAttributes()](https://github.com/tianocore/edk2/blob/master/UefiCpuPkg/CpuDxe/CpuDxe.c) followed by 
[AssignMemoryPageAttributes()](https://github.com/tianocore/edk2/blob/master/UefiCpuPkg/CpuDxe/CpuPageTable.c) to 
setup page table.
The ARM CPU driver call ( 
[CpuSetMemoryAttributes()](https://github.com/tianocore/edk2/blob/master/ArmPkg/Drivers/CpuDxe/CpuMmuCommon.c) ) 
also has similar capability.

If an image is loaded before CPU_ARCH protocol is ready, the DXE core skips the setting until the CPU_ARCH notify 
function 
[MemoryProtectionCpuArchProtocolNotify()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c) 
is invoked at which point the protection settings are applied to the image. When the ExitBootServices event raised, 
[MemoryProtectionExitBootServicesCallback()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c) 
is invoked to unprotect the runtime image to accomodate virtual address mapping.

### Implementation Details in SMM
In UEFI/PI firmware, the SMM image is a normal PE/COFF image loaded by the SmmCore. However, image protection in 
SMM is completely separate from this PCD and is controlled by the static variable 
[mMemoryProtectionAttribute](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/PiSmmCore/MemoryAttributesTable.c)
. To change image protection in SMM, change that variable.

### Overhead
**O(n)** time and space overhead. Each image requires a 6K attributes header, so if there are **n** images the 
space overhead will be 6K\*n and thus O(n) time to populate the headers. Of course, in most cases the number of 
images is fairly low, and so enabling this feature is relatively inexpensive.

**Available Bits:**

- BIT0 - Images from Unknown devices
- BIT1 - Images from firmware volumes

## PcdDxeNxMemoryProtectionPolicy

Sets DXE memory protection policy. If a bit is set, memory regions of the associated type
will be mapped non-executable. **Note** that a portion of memory will only be marked as
non-executable once gEfiCpuArchProtocolGuid has been published. **Also note** that in order
to enable Data Execution Protection, the operating system needs to set the 
[IA32_EFER.NXE](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) bit in the IA32_EFER [MSR](#-Model-specific-Register-(MSR)), 
and then set the [XD](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) bit in the CPU [PAE](#Physical/Page-Address-Extension) 
page table. **Finally**, [NX](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) settings cannot be applied while in SMM.

This PCD is consumed by DXE Core through 
[ApplyMemoryProtectionPolicy()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c) 
which sets the NX attribute for allocated memory using the CPU_ARCH protocol (hence why gEfiCpuArchProtocolGuid 
must be published for this to work). Once gEfiCpuArchProtocolGuid is published, 
[MemoryProtectionCpuArchProtocolNotify()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c) 
is called and 
[InitializeDxeNxMemoryProtectionPolicy()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Misc/MemoryProtection.c) 
will get the current memory map and setup [NX](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) protection. Just 
prior to applying protection, memory mapped regions will be "merged" such that adjacent entries with the same memory 
protection policy will become one entry.

### Overhead
**O(n)** time where n is the number of memory mapped regions. The number of actual set bits beyond one is 
inconsequential because every memory region must be checked if at least one bit is set. There is no extra space 
complexity due to using the already present [NX](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) bit.

**Available Bits:**

- BIT0  - EfiReservedMemoryType
- BIT1  - EfiLoaderCode
- BIT2  - EfiLoaderData
- BIT3  - EfiBootServicesCode
- BIT4  - EfiBootServicesData
- BIT5  - EfiRuntimeServicesCode
- BIT6  - EfiRuntimeServicesData
- BIT7  - EfiConventionalMemory
- BIT8  - EfiUnusableMemory
- BIT9  - EfiACPIReclaimMemory  
- BIT10 - EfiACPIMemoryNVS  
- BIT11 - EfiMemoryMappedIO
- BIT12 - EfiMemoryMappedIOPortSpace
- BIT13 - EfiPalCode
- BIT14 - EfiPersistentMemory
- BIT15 - OEM Reserved
- BIT16 - OS Reserved

## PcdHeapGuardPageType

This PCD implements guard pages on the specified regions to detect heap overflow. If a bit
is set, a guard page will be added before and after the
corresponding type of page allocated if there's enough free pages for all of them. On the 
implementation side, the tail and guard pages are simply set to NOT PRESENT so any attempt 
to access them will cause a [#PF](#Page-Fault-Exception-(AKA-#PF)).

### Implementation Details
Whenever there is a call to allocate a page, if the 
[IsPageTypeToGuard()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/HeapGuard.c) call 
returns TRUE, 
[CoreInternalAllocatePages()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/Page.c) uses 
[CoreConvertPagesWithGuard()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/HeapGuard.c) 
to allocate 2 extra pages and calls 
[SetGuardForMemory()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/HeapGuard.c) 
which calls [SetGuardPage()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/HeapGuard.c) 
twice to set the guard page before and after. 
[SetGuardPage()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/HeapGuard.c) calls 
[CpuSetMemoryAttributes()](https://github.com/tianocore/edk2/blob/master/UefiCpuPkg/CpuDxe/CpuDxe.c) to clear the 
PRESENT flag. Finally, 
[SetGuardForMemory()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/HeapGuard.c) 
calls [SetGuardedMemoryBits()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/HeapGuard.c) 
to mark the memory range as guarded. This bitmask will be checked in 
[UnsetGuardForMemory()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/HeapGuard.c) when 
[CoreInternalFreePages()](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/Core/Dxe/Mem/Page.c) is called.

Pages can be freed partially while maintaining guard structure as shown in the following figure.

![Heap Guard Image](guardpages_mu.PNG)

### Overhead
**O(n)** time where n is the number of page allocations/frees. Because there are 2 extra pages allocated for 
every call to AllocatePages(), **O(n)** space is also required.

**Available Bits:**

- BIT0  - EfiReservedMemoryType
- BIT1  - EfiLoaderCode
- BIT2  - EfiLoaderData
- BIT3  - EfiBootServicesCode
- BIT4  - EfiBootServicesData
- BIT5  - EfiRuntimeServicesCode
- BIT6  - EfiRuntimeServicesData
- BIT7  - EfiConventionalMemory
- BIT8  - EfiUnusableMemory
- BIT9  - EfiACPIReclaimMemory
- BIT10 - EfiACPIMemoryNVS
- BIT11 - EfiMemoryMappedIO
- BIT12 - EfiMemoryMappedIOPortSpace
- BIT13 - EfiPalCode
- BIT14 - EfiPersistentMemory
- BIT15 - OEM Reserved
- BIT16 - OS Reserved

## PcdHeapGuardPoolType

This is essentially the same as PcdHeapGuardPageType. If a bit is set, a head guard page and 
a tail guard page will be added just before and after the portion of memory which the 
allocated pool occupies. For brevity, I will not trace the calls here as well, but it's essentially the same as 
above with the word "Pool" instead of "Page". The only added complexity comes when the allocated pool is not a 
multiple of the size of a page. In this case, the pool must align with either the head or tail guard page. This 
behavior is defined by BIT7 in [PcdHeapGuardPropertyMask](#PcdHeapGuardPropertyMask) - look there for additional 
details.

### Overhead
Same as above: **O(n)** time and space for same reasons as [PcdHeapGuardPageType](#PcdHeapGuardPageType). **Note** 
that this functionality requires creating guard pages, meaning that for n allocations, 4k \* (n + 1) (assuming 
each of the n pools is adjacent to another pool) additional space is required.

**Available Bits:**

- BIT0  - EfiReservedMemoryType
- BIT1  - EfiLoaderCode
- BIT2  - EfiLoaderData
- BIT3  - EfiBootServicesCode
- BIT4  - EfiBootServicesData
- BIT5  - EfiRuntimeServicesCode
- BIT6  - EfiRuntimeServicesData
- BIT7  - EfiConventionalMemory
- BIT8  - EfiUnusableMemory
- BIT9  - EfiACPIReclaimMemory
- BIT10 - EfiACPIMemoryNVS
- BIT11 - EfiMemoryMappedIO
- BIT12 - EfiMemoryMappedIOPortSpace
- BIT13 - EfiPalCode
- BIT14 - EfiPersistentMemory
- BIT15 - OEM Reserved
- BIT16 - OS Reserved

## PcdHeapGuardPropertyMask

While the above two Pcd Masks ([PcdHeapGuardPoolType](#PcdHeapGuardPoolType) and 
[PcdHeapGuardPageType](#PcdHeapGuardPageType)) act as a 
switch for each protectable type of memory, this PCD is an enable/disable switch for those 
two PCDs (ex. if BIT0 == 0, UEFI page guards are inactive regardless of the bitmask for 
[PcdHeapGuardPageType](#PcdHeapGuardPageType)).

The only aspect of this PCD which should be elaborated upon is BIT7. BIT7 dictates whether an allocated pool 
which does not fit perfectly into a multiple of pages is aligned to the head or tail guard. The following Figure 
shows examples of the two.

![Heap Guard Pool Alignment Image](alignment_mu.PNG)

**Available Bits:**

- BIT0 - Enable UEFI page guard
- BIT1 - Enable UEFI pool guard
- BIT2 - Enable SMM page guard
- BIT3 - Enable SMM pool guard
- BIT6 - Enable [Non-Stop Mode](#Non-Stop-Mode)
- BIT7 - Specifies the direction of Guard Page for Pool Guard. If 0, the returned
pool is near the tail guard page. If 1, the returned pool is near the head guard page. The
default value for this is 0

### Overhead
Overhead is same as [PcdHeapGuardPoolType](#PcdHeapGuardPoolType) and 
[PcdHeapGuardPageType](#PcdHeapGuardPageType) if any of BIT0 - BIT3 are enabled.

## PcdCpuStackGuard and PcdCpuSmmStackGuard

PcdCpuStackGuard indicates if UEFI Stack Guard will be enabled and an equivalent SMM 
stack guard feature is contained in 
[PiSmmCpuDxeSmm](https://github.com/tianocore/edk2/tree/master/UefiCpuPkg/PiSmmCpuDxeSmm). 

### Implementation Details
The stack guards add two additional pages to the bottom of the stack(s). The first page is simply the guard page 
which is set to not present. However, a complexity arises when a page fault occurs due to the guard page being 
accessed. In this case, the current stack address is invalid and so it is not possible to push the error code and 
architecture status onto the current stack. For this reason, there is a special "Exception Stack" (described as a 
"Known Good Stack" in the actual codebase) which is the second page placed at the bottom of the stack. This page is 
reserved for use by the exception handler and ensures that a valid stack is always present when an exception occurs 
for error reporting.

**Note** that the UEFI
stack protection starts in DxeIpl, because the region is fixed, and requires 
[PcdDxeIplBuildPageTables](https://github.com/tianocore/edk2/blob/master/MdeModulePkg/MdeModulePkg.dec) to be 
TRUE. If the PcdCpuStackGuard is TRUE, the DxeIpl clears the PRESENT bit in the page table for the guard page of 
the Boot Strap Processor stack. The guard page of the Application Processor stack is initialized in CpuDxe driver 
by using the DXE service 
[CpuSetMemoryAttributes()](https://github.com/tianocore/edk2/blob/master/UefiCpuPkg/CpuDxe/CpuDxe.c). 
[PcdCpuSmmStackGuard](https://github.com/tianocore/edk2/blob/master/UefiCpuPkg/UefiCpuPkg.dec).

### Overhead
**O(1)** time and space.

**Setting:**

- If TRUE, UEFI Stack Guard will be enabled.

## PcdSetNxForStack

This TRUE/FALSE PCD indicates if the stack will have [NX](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) protection.

### Overhead
**O(1)** time and space.

**Setting:**

- If TRUE, Stack will have [NX](#Non-eXecute/eXecute-Disable-Bit-(NX/DX)) bit set.
