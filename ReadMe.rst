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

Branch Status - release/202511
==============================

:Status:
  In Development

:Entered Development:
  2025/11/23 (Date EDK2 started accepting changes which were not in a previous release)

:Anticipated Stabilization:
  May 2025

Branch Changes - release/202511
===============================

Breaking Changes-dev
--------------------

- General

  - EDK2 packages that were split into mu_tiano_plus, mu_silicon_intel_tiano, mu_silicon_arm_tiano have been kept in mu_basecore. 
  - mu_tiano_plus, mu_silicon_arm_tiano are deprecated, and projects should be updated to no longer consume them.
  - MSVC AARCH64 build support has been removed. Please use ClangPDB or GCC for AARCH64 compilation.
  - Itanium support cleanup. DXE_SAL_DRIVER is no longer recognized as a keyword by build tools. Remove any usage of this term.
  - ARM support (arm 32) has been removed by EDK2.
  - mu_silicon_intel_tiano only contains IntelSiliconPkg from edk2-platforms.
  - EDK2 is moving towards depreciating TOOL_CHAIN_TAG GCC5, and platforms should start planning on using the GCC replacement. NOTE: This has implications for build overrides from dsc and inf files. 

- MdePkg

  - gEfiEventPostReadyToBootGuid has been dropped in favor of EDK2's gEfiEventAfterReadyToBootGuid. Events should be switched to use gEfiEventAfterReadyToBootGuid.

- MdeModulePkg

  - UgaSupport has been removed from EDK2. This includes PCDs associated with it such as gEfiMdeModulePkgTokenSpaceGuid.PcdConOutUgaSupport
  - UefiDevicePathLibStandaloneMm has been removed by EDK2. Platforms should replace this with instance UefiDevicePathLibBase.
  - UhciPei.inf has been removed by EDK2. Remove any reliance on the driver.
  - PPI gPeiUsbHostControllerPpiGuid has been removed by EDK2. Remove any references to it and associated files. 
  - BaseUefiBootManagerLibNull has been removed. It was intended to speed up CI builds, but never to be used in platforms. Remove references to it.
  - MemoryProtectionSpecialRegionGuid support has been depreciated. Remove any references to the includes.
  - gMemoryProtectionNonstopModeProtocolGuid has been removed. Code relying upon it should be changed to expect a system reboot upon exceptions.
  - BaseUefiBootManagerLibNull instance of UefiBootManagerLib has been dropped.
  - Memory Protection Read Protect on Freed memory has been dropped. The implementation resulted in false reports of issues.
  - gEfiEventPostReadyToBootGuid has been dropped in favor of EDK2's gEfiEventAfterReadyToBootGuid

- StandaloneMmPkg

  - MmCommunicationNotifyDxe.inf has been created by splitting the functionality out of MmCommunicationDxe.inf. Platforms will need to update to include the additional driver.

- UefiCpuPkg

  - AmdSysCallLib has been introduced and dependencies taken. AMD based platform may need to include this additional library class.
  - PcdCpuApWakeupBufferReserved has been removed. WakeupBuffers will be allocated as EfiBootServicesData. Please remove pcds from any components.

- EmbeddedPkg

  - FdtLib has been dropped. Please use the BaseFdtLib from MdePkg. (Will require code to be touched because interfaces are not 1:1 compatible).

- ArmPkg

  - ArmMmuLib has been promoted from ArmPkg into UefiCpuPkg. Please update the paths for instances.
  - gArmTokenSpaceGuid.PcdVFPEnabled has been dropped. New implemenation should not require this. 
  - PcdFdMemoryType has been deprecated. Fd memory will be reported as EfiBootServicesData.

MU Overrides on EDK2
--------------------

- At the start of 202511, mu_basecore contains 307 commits on top of edk2-stable202511. Full MU changes list can be viewed `in the changelog <https://github.com/microsoft/mu_basecore/compare/fbe0805b2091393406952e84724188f8c1941837...release/202511>`_.

Platform Integration Reference
------------------------------
Reference platforms which consume release/202511 are available in `mu_tiano_platforms <https://github.com/microsoft/mu_tiano_platforms>`_.

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

.. _Windows_VS2022: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=39&branchName=release%2F202511
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202511
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/39.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=40&branchName=release%2F202511
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20Ubuntu%20GCC5?branchName=release%2F202511
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/40.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202511
