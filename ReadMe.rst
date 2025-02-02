==============================
Project Mu Basecore Repository
==============================

============================= ================= =============== ===================
 Host Type & Toolchain        Build Status      Test Status     Code Coverage
============================= ================= =============== ===================
Windows_VS2022_               |WindowsCiBuild|  |WindowsCiTest| |WindowsCiCoverage|
Ubuntu_GCC5_                  |UbuntuCiBuild|   |UbuntuCiTest|  |UbuntuCiCoverage|
============================= ================= =============== ===================

This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu.

For more details about the repository, refer to `RepoDetails.md`_.

.. _`RepoDetails.md`: https://github.com/microsoft/mu_basecore/blob/HEAD/RepoDetails.md

Branch Status - release/202608
==============================

:Status:
  In Development

:Entered Development:
  2026/08/21 (Date EDK2 started accepting changes which were not in a previous release)

:Anticipated Stabilization:
  Feb 2027

Branch Changes - release/202608
===============================

Breaking Changes-dev
--------------------

- Header include guards

  - Many headers replaced named ``#ifndef``/``#define`` include guards with ``#pragma once``. Normal inclusion is
    unchanged, but consumers must not test or define another header's former guard macro to detect or suppress it.

- ArmPkg

  - ``ArmExceptionLib`` and ``DefaultExceptionHandlerLib`` were moved from ``ArmPkg`` and merged into
    ``UefiCpuPkg/CpuExceptionHandlerLib``. Replace calls to ``DefaultExceptionHandler()`` with ``DumpCpuContext()``,
    add ``UefiCpuPkg.dec`` as a package dependency, and update platform DSC library mappings.
  - ``Library/MmuLib/BaseMmuLib.inf`` was removed. Standalone MM consumers should migrate to
    ``Library/StandaloneMmMmuLib/ArmMmuStandaloneMmLib.inf`` and update their library class usage as needed.

- BaseTools

  - ``GenFv`` supports selective rebasing through the new ``Xip=TRUE/FALSE`` FDF rule keyword. When
    ``FvForceRebase=TRUE`` and any file in an FV uses ``Xip=TRUE``, only files marked ``Xip=TRUE`` are rebased;
    platforms adopting selective rebasing must update the relevant FDF rules.
  - Visual Studio 2015 and 2017 toolchain support was removed. Windows builds and CI must use VS2019, VS2022, or
    VS2026.
  - The deprecated ``GCC48``, ``GCC49``, and ``GCC5`` toolchain definitions were removed. Platforms using these
    toolchain tags must migrate to ``GCC``.
  - X64 images built with GCC, CLANGDWARF, CLANGPDB, VS2022, or VS2026 now use 4 KiB section alignment by default.
    SEC, PEI_CORE, and PEIM XIP images retain their previous alignment through module-specific linker flags. Platforms
    with firmware-size constraints or loaders that assume smaller alignment must review their generated images.

- CryptoPkg

  - ``Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf``, ``SmmCryptLib.inf``, and
    ``StandaloneMmCryptLib.inf`` were removed as part of the OneCrypto transition. Replace them with the
    corresponding instances under ``Library/BaseCryptLibOnOneCrypto`` and add the required OneCrypto binary drivers
    and loaders to the platform DSC.
  - ``Library/BaseCryptLibOnProtocolPpi/RuntimeDxeCryptLib.inf`` and
    ``Library/BaseCryptLibMbedTls/UnitTestHostBaseCryptLib.inf`` were removed with no direct OneCrypto replacement.
    Runtime and host-test consumers must remove these mappings and select a crypto instance supported by their
    execution environment.
  - Projects requiring PEI hashing support through the CryptoLib should use ``Library\BaseCryptLibOnProtocolPpi\PeiCryptLib.inf``
    and include the 

- EmbeddedPkg

  - The ``PrePiLib`` APIs changed: ``FfsFindSectionDataWithHook()`` now takes an ``AuthenticationStatus`` output
    parameter and ``FfsProcessFvFile()`` now takes a ``ParentVolumeHandle`` parameter.

- MdeModulePkg

  - ``PartitionDxe``, ``DxeTpm2MeasureBootLib``, and ``DxeTpmMeasureBootLib`` now depend on the new ``GptLib``
    library class. Platforms that build these modules must map ``GptLib`` to
    ``MdeModulePkg/Library/GptLib/GptLib.inf`` in their DSC files.
  - ``Library/RealTimeClockLibNull/RealTimeClockLibNull.inf`` was removed with no direct replacement. Platforms that
    mapped ``RealTimeClockLib`` to this null instance must use a platform RTC implementation or provide their own
    null implementation if unsupported RTC behavior is still required.
  - The ``InternalEventServices`` protocol and ``PcdInternalEventServicesEnabled`` were removed, including
    ``INTERNAL_EVENT_SERVICES_PROTOCOL_GUID``, ``WAIT_FOR_EVENT_INTERNAL``, ``WaitForEventInternal``, and
    ``gInternalEventServicesProtocolGuid``. The related private DXE Core interfaces ``CoreWaitForEventInternal()``
    and ``InternalEventServicesInit()`` were also removed. Consumers must use the standard boot-services
    ``WaitForEvent()`` interface at ``TPL_APPLICATION`` and refactor waits at higher TPLs into event-driven callbacks.
  - The ``NonDiscoverableDeviceUniqueId`` protocol was removed, including
    ``EDKII_NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL_GUID``,
    ``NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL_REVISION``, ``MAX_NON_DISCOVERABLE_PCI_DEVICE_ID``,
    ``NON_DISCOVERABLE_DEVICE_UNIQUE_ID_PROTOCOL``, and ``gEdkiiNonDiscoverableDeviceUniqueIdProtocolGuid``. Stop
    installing the protocol; non-discoverable PCI devices now receive IDs from discovery order. Consumers that
    require stable device identity must use a platform-owned identification mechanism instead of the ID returned by
    ``EFI_PCI_IO_PROTOCOL.GetLocation()``.
  - The private DXE Core ``GetBucketMemoryType()`` interface was removed from ``Core/Dxe/Mem/Imem.h``. Out-of-tree
    DXE Core changes must use the current memory-map and memory-protection infrastructure instead of calling this
    removed helper.
  - ``Universal/Variable/RuntimeDxe/RuntimeDxeUnitTest/VariableRuntimeDxeUnitTest.inf`` was removed. Test DSC files
    must remove this module. Its private test headers, assertion GUID ``#define`` values, SCT shim macros, and test
    function declarations were also removed; use the remaining focused variable unit tests or platform-owned
    conformance tests for equivalent coverage.

- MdePkg

  - ``Library/BaseMmuLibNull/BaseMmuLibNull.inf`` and the ``MmuLib`` library class were removed. Platforms must
    remove null ``MmuLib`` mappings. The header and its ``MmuSetAttributes()``, ``MmuClearAttributes()``, and
    ``MmuGetAttributes()`` interfaces were also removed; consumers must migrate to the architecture-specific MMU
    library or the CPU, memory-attribute, or MM memory-attribute protocol appropriate to their execution environment.
  - The ``Cpu2`` protocol and the BaseLib ``EnableInterruptsAndSleep()`` API were removed, including
    ``EFI_CPU2_PROTOCOL_GUID``, ``EFI_CPU2_PROTOCOL``, ``EFI_CPU_ENABLE_AND_WAIT_FOR_INTERRUPT``,
    ``EnableAndForWaitInterrupt``, and ``gEfiCpu2ProtocolGuid``. CPU drivers must stop producing this protocol. For
    normal DXE idle handling, use ``gIdleLoopEventGuid`` and perform the architecture-specific sleep operation from
    the CPU driver's idle-loop callback, as in ``UefiCpuPkg/CpuDxe``.
  - ``EFI_SW_DXE_CORE_EC_IMAGE_LOAD_FAILURE`` was removed from ``Pi/PiStatusCode.h``. Code emitting this MU-specific
    status code must remove the report or define and use a platform-owned status-code value; there is no direct
    replacement in ``PiStatusCode.h``.

- SecurityPkg

  - TPM2 helper functions were moved from ``Tpm2CommandLib`` to the new ``Tpm2HelpLib`` library class. Platforms
    that build affected modules must map ``Tpm2HelpLib`` to ``SecurityPkg/Library/Tpm2HelpLib/Tpm2HelpLib.inf`` in
    their DSC files.

- StandaloneMmPkg

  - ``Library/FvLib/FvLib.inf`` was removed. Consumers should map the ``FvLib`` library class to
    ``MdePkg/Library/FvLib/FvLib.inf`` and add ``MdePkg/MdePkg.dec`` as a package dependency if needed.
  - ``Library/StandaloneMmCoreEntryPointNull/StandaloneMmCoreEntryPointNull.inf`` was removed. Map
    ``StandaloneMmCoreEntryPoint`` to ``MdePkg/Library/StandaloneMmCoreEntryPoint/StandaloneMmCoreEntryPoint.inf``
    for X64 or ``ArmPkg/Library/ArmStandaloneMmCoreEntryPoint/ArmStandaloneMmCoreEntryPoint.inf`` for AARCH64.

- ManageabilityPkg

  - ManageabilityPkg now exists in mu_basecore. Platforms are encouraged to switch to this over mu_feature_ipmi.

See `EDK II Breaking Changes <BREAKING-CHANGES.md>`_ for complete descriptions, affected consumers, and migration
guidance.


MU Overrides on EDK2
--------------------

- At the start of 202608, mu_basecore contains 267 commits on top of edk2-stable202608. Full MU changes list can be viewed `in the changelog <https://github.com/microsoft/mu_basecore/compare/2970e5699ba6267f3384ffab20f96647578aebc8...release/202608>`_.

Platform Integration Reference
------------------------------
Reference platforms which consume release/202608 are available in `mu_tiano_platforms <https://github.com/microsoft/mu_tiano_platforms>`_.

Please note that this version of EDK2 has specific requirements when it comes to TF-A support. 
Platforms that consume this version of EDK2 must ensure their TF-A `contains this set of patches <https://review.trustedfirmware.org/q/topic:%22hob_creation_in_tf_a%22>`_.
Failure to contain the appropriate patches will result in a failure to boot.

Code of Conduct
===============

This project has adopted the Microsoft Open Source Code of Conduct https://opensource.microsoft.com/codeofconduct/

For more information see the Code of Conduct FAQ https://opensource.microsoft.com/codeofconduct/faq/
or contact `opencode@microsoft.com <mailto:opencode@microsoft.com>`_ with any additional questions or comments.

Contributions
=============

Contributions are always welcome and encouraged!
Please open any issues in the Project Mu GitHub tracker and read https://microsoft.github.io/mu/How/contributing/

For documentation:

Copyright & License
===================

| Copyright (c) Microsoft Corporation
| SPDX-License-Identifier: BSD-2-Clause-Patent

Upstream License (TianoCore)
============================

Copyright (c) 2019, TianoCore and contributors.  All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

Subject to the terms and conditions of this license, each copyright holder
and contributor hereby grants to those receiving rights under this license
a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable
(except for failure to satisfy the conditions of this license) patent
license to make, have made, use, offer to sell, sell, import, and otherwise
transfer this software, where such license applies only to those patent
claims, already acquired or hereafter acquired, licensable by such copyright
holder or contributor that are necessarily infringed by:

(a) their Contribution(s) (the licensed copyrights of copyright holders and
    non-copyrightable additions of contributors, in source or binary form)
    alone; or

(b) combination of their Contribution(s) with the work of authorship to
    which such Contribution(s) was added by such copyright holder or
    contributor, if, at the time the Contribution is added, such addition
    causes such combination to be necessarily infringed. The patent license
    shall not apply to any other combinations which include the
    Contribution.

Except as expressly stated above, no rights or licenses from any copyright
holder or contributor is granted under this license, whether expressly, by
implication, estoppel or otherwise.

DISCLAIMER

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

.. ===================================================================
.. This is a bunch of directives to make the README file more readable
.. ===================================================================

.. CoreCI

.. _Windows_VS2022: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=39&branchName=release%2F202608
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202608
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/39.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=40&branchName=release%2F202608
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20Ubuntu%20GCC5?branchName=release%2F202608
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/40.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202608
