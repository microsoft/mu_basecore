# Project Mu Memory Protection

The Project Mu Memory Protection Settings add safety functionality such as page and pool guards,
stack guard, and null pointer detection. The settings are split between MM and DXE environments
for modularity.

## UEFI Paging Protection Attributes

There are 3 UEFI attributes which are manipulated to apply the protections described in this
document. Each UEFI attribute corresponds to some number of architecture specific bits on
either ARM or x86 silicon.

### EFI_MEMORY_RP

This EFI attribute manipulates the writeable and readable attributes of a page. When set,
the memory is read protected.

### EFI_MEMORY_RO

This EFI attribute manipulates writeable attribute of a page. When set, the
memory is read-only.

### EFI_MEMORY_XP

This EFI attribute manipulates execute attribute of a page. When set, the memory is
non-executable. In order for this attribute to work, architecture-specific register
configuration bits must be set properly. For example, on x86 the IA32_EFER.NXE bit in
the IA32_EFER MSR register must be set.

## Enhanced Memory Protection and Compatibility Mode

Microsoft has defined a set of paging protections which will be required for UEFI
distributions booting Windows (Enhanced Memory Protections). Microsoft also defined
Compatibility Mode which is a reduced security state suitable for legacy option
ROMs and older Linux distributions. The specifics of these two modes are detailed
in the
[Project Mu documentation](https://microsoft.github.io/mu/WhatAndWhy/enhancedmemoryprotection/).

## Null Pointer Detection

Pages are allocated in 4KB chunks (UEFI Spec Required). This policy sets the attributes of the
4KB page at the NULL address to [EFI_MEMORY_RP](#efi_memory_rp) to detect NULL pointer
dereferences in DXE and/or platform MM.

The **DXE environment** has the following settings available:

- **UefiNullDetection**: Enable NULL pointer detection for DXE.
- **DisableEndOfDxe**: Disable NULL pointer detection just after the end of DXE protocol
is installed.
- **DisableReadyToBoot**: Disable NULL pointer detection just after the ready to boot
protocol is installed.

DisableEndOfDxe and DisableReadyToBoot are provided to deal with problematic legacy
drivers, option ROMs, and bootloaders which may access memory below 4KB such as older
linux distros.

The **MM environment** only has a single option indicating whether NULL detection
is active or not.

## FreeMemoryReadProtected

If enabled, all EfiConventionalMemory (free memory) will be marked with the
[EFI_MEMORY_RP](#efi_memory_rp) attribute. This policy will cause accesses to uanallocated
or freed memory to trigger a page fault and target one of the most common programmer errors.
This can be used in conjunction with the NX setting for EfiConventionalMemory.

## Image Protection Policy

This policy enables a loaded EFI image to have [EFI_MEMORY_XP](#efi_memory_xp) to its
DATA sections and [EFI_MEMORY_RO](#efi_memory_ro) to its CODE sections. Loaded EFI
images must adhere to the following rules for this policy to work:

1. The PE code section and data sections are not merged.
2. The PE image sections must be page aligned.
3. A platform may not disable XN/NX in the configuration
registers in the DXE phase.
4. CODE sections must not be self-modifying
5. Modules of type DXE_RUNTIME_DRIVER must have their section alignment set to
RUNTIME_PAGE_ALLOCATION_GRANULARITY which may differ from EFI_PAGE_SIZE. File
alignment can still be EFI_PAGE_SIZE.

This policy **only applies to DXE**.

- **FromFv**: Protect images from firmware volumes.
- **FromUnknown**: Protect images not from firmware volumes.
- **RaiseErrorIfProtectionFails**: If set, images which fail to be protected will be
unloaded which will trigger an ASSERT on DEBUG builds. An image will still be loaded
if it follows the above rules but was loaded before the CPU Arch Protocol was installed.
Images loaded before the CPU Arch Protocol was installed will be protected when the
protocol is installed.
- **BlockImagesWithoutNxFlag**: Images which do not have the NX_COMPAT DLL characteristic
set will be blocked from loading and trigger an ASSERT on DEBUG builds. If this is set to
FALSE, images without the NX_COMPAT DLL characteristic will still be loaded but will trigger
Compatibility Mode. See
[Enhanced Memory Protection and Compatibility Mode](#enhanced-memory-protection-and-compatibility-mode)
for more information.

## NX Memory Protection Policy

This policy applies [EFI_MEMORY_XP](#efi_memory_xp) to memory of the
associated memory type. **This policy only applies to DXE**

The available settings match the EFI memory types as well as the OEMReserved
and OSReserved regions defined in the UEFI specification.

## Page Guards

The HeapGuardPageType policy implements guard pages on the specified memory types
to detect heap overflow. If a bit is set, a guard page will be added before and
after the corresponding type of page allocated if there's enough free pages for
all of them. Guard pages are set to [EFI_MEMORY_RP](#efi_memory_rp) so any attempt
to access them will cause a page fault. The system will do its best to ensure
that only one guard page separates two allocated pages to avoid wasted space.

The available settings match the EFI memory types as well as the OEMReserved
and OSReserved regions defined in the UEFI specification.

## Pool Guards

The HeapGuardPoolType policy is essentially the same as HeapGuardPageType policy.
For each active memory type, a guard page with the [EFI_MEMORY_RP](#efi_memory_rp) attribute
will be added just before and after the portion of memory which the
allocated pool occupies. The only added complexity comes when the allocated pool is not a
multiple of the size of a page. In this case, the pool must align with either the head or tail
guard page, meaning either overflow or underflow can be caught consistently but not both.
The head/tail alignment is set in [HeapGuardPolicy](#heapguardpolicy).

The available settings match the EFI memory types as well as the OEMReserved
and OSReserved regions defined in the UEFI specification.

## HeapGuardPolicy

While the above two policies ([Pool Guards](#pool-guards) and [Page Guards](#page-guards))
act as a switch for each memory type, this policy is an enable/disable
switch for those two policies. For example, if UefiPageGuard is unset then page guards are
inactive regardless of the individual [Page Guard](#page-guards) settings.

The only aspect of this policy which should be elaborated upon is Direction.
Direction dictates whether an allocated pool which does not fit perfectly into a
multiple of pages is aligned to the head or tail guard. The following Figure shows
examples of the two:

![Heap Guard Pool Alignment Image](alignment_mu.PNG)

On free the pool head/tail is checked to ensure it was not overwritten while the
[EFI_MEMORY_RP](#efi_memory_rp) page will trigger a page fault immediately.

The **DXE environment** has the following settings available:

- UefiPageGuard - Enable UEFI page guard
- UefiPoolGuard - Enable UEFI pool guard
- Direction - Specifies the direction of Guard Page for Pool Guard. If 0, the returned
pool is near the tail guard page. If 1, the returned pool is near the head guard page. The
default value for this is 0

The **MM environment** has the following settings available:

- SmmPageGuard - Enable SMM page guard
- SmmPoolGuard - Enable SMM pool guard
- Direction - Specifies the direction of Guard Page for Pool Guard. If 0, the returned
pool is near the tail guard page. If 1, the returned pool is near the head guard page. The
default value for this is 0

## CPU Stack Guard

CPU Stack Guard adds two additional pages to the stack base for each core. The first page is
simply a guard page with the [EFI_MEMORY_RP](#efi_memory_rp) attribute. When a page fault
occurs, the current stack address is invalid and so it is not possible to push the error
code and architecture status onto the current stack. Because of this, there is a special
"Exception Stack" or "Known Good Stack" which is the second page placed at the base
of the stack. This page is reserved for use by the exception handler and ensures that a
valid stack is always present when an exception occurs for error reporting.

**A note on SMM:** An equivalent SMM stack guard feature is contained in
UefiCpuPkg/PiSmmCpuDxeSmm and is not dictated by this policy.

**Note that the UEFI:** stack protection starts in DxeIpl, because the region is fixed,
and requires PcdDxeIplBuildPageTables to be TRUE. In Project Mu, we have hard-coded CpuStackGuard
to be TRUE in PEI phase, so we always set up a switch stack and guard page in PEI. However,
the stack switch handlers will still only be installed in DXE phase if CpuStackGuard is TRUE.
If the stack guard is disabled in DXE, the paging attributes at the stack base will be
removed during memory protection initialization.

## Stack Cookies

A stack cookie (also called stack canary) is an integer placed in memory just before the stack
return pointer. Most buffer overflows overwrite memory from lower to higher memory addresses,
so in order to overwrite the return pointer (and thus take control of the process) the canary
value must also be overwritten. This value is checked to make sure it has not changed before a
routine uses the return pointer on the stack.

The stack cookie value is specific to each loaded image and is generated at random on image load
in DXE. Stack cookies are enabled at compile time, but if this setting is FALSE the interrupts
generated by stack cookie check failures will be ignored **which is extremely unsafe**. Stack
cookie failures will trigger a warm reset if this policy is TRUE.

## How to Set the Memory Protection Policy

For DXE settings, add the following to the platform DSC file:

```text
[LibraryClasses.Common.DXE_DRIVER, LibraryClasses.Common.DXE_CORE, LibraryClasses.Common.UEFI_APPLICATION]
  DxeMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLib/DxeMemoryProtectionHobLib.inf
```

For MM settings, add the following to the platform DSC file if the platform utilizes SMM:

```text
[LibraryClasses.common.SMM_CORE, LibraryClasses.common.DXE_SMM_DRIVER]
  MmMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLib/SmmMemoryProtectionHobLib.inf
```

**or** the following if the platform utilizes Standalone MM:

```text
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

## Memory Protection Special Regions

Memory protection is not activated until the CPU Architecture Protocol has been installed because the
protocol facilitates access to the attribute manipulation functions in CpuDxe which update the translation/page tables.
Many allocations and image loads will have occurred by the time the protocol is published so careful accounting
is required to ensure appropriate attributes are applied. An event notification triggered on the CPU Architecture
Protocol installation will combine the GCD and EFI memory maps to create a full map of memory for use internally
by the memory protection initialization logic. Because image memory is allocated for the entire image and not each
section (code and data), the images are separated within
the combined map so NX can be applied to data regions and RO can be applied to code regions. After breaking
up the map so each DXE image section has its own descriptor, every non-image descriptor will have its attributes
set based on its EFI memory type. There are cases where the platform will want to apply attributes to
a region of memory which is different than what would be applied based on its EFI memory type. In this case,

platforms can utilize the Memory Protection Special Region interface to specify regions which should have specific
attributes applied during memory protection initialization.

### Example Declaration of Special Region in PEI

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

### Example Declaration of Special Region in DXE

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
