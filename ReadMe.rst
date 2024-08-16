==============================
Project Mu Basecore Repository
==============================

============================= ================= =============== ===================
 Host Type & Toolchain        Build Status      Test Status     Code Coverage
============================= ================= =============== ===================
Windows_VS2022_               |WindowsCiBuild|  |WindowsCiTest| |WindowsCiCoverage|
Ubuntu_GCC5_                  |UbuntuCiBuild|   |UbuntuCiTest|  |UbuntuCiCoverage|
============================= ================= =============== ===================

This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu

For more details about the repository, refer to `RepoDetails.md`_.

.. _`RepoDetails.md`: https://github.com/microsoft/mu_basecore/blob/HEAD/RepoDetails.md

Branch Status - release/202405
==============================

:Status:
  In Development

:Entered Development:
  2023/11/24 (Date Edk2 started accepting changes which were not in a previous release)

:Anticipated Stabilization:
  Nov 2024

Branch Changes - release/202405
===============================

202405 is a larger deviation than previous releases. As part of upstreaming changes to EDK2, the commits were reviewed, squashed, and some were dropped.
Due to these changes, there may be more work required to bring an existing platform up to 202405 compatibility. 

Breaking Changes-dev
--------------------
- SourceLevelDebugPkg has been dropped. Please reevaluate usage of this Package.
- MemoryBinOverrideLib and associated changes have been deprecated and dropped. Platforms should remove this library instance. 
- MdeModulePkg/Application/MpServicesTest/MpServicesTest.inf has been dropped. Any reliance on this should move to using mu_plus's UefiTestingPkg/FunctionalSystemTests/MpManagement.
- NetworkPkg/SharedNetworkingPkg and NetworkPkg/SharedNetworkingPkg.fdf.inc have been dropped. Please include specific inf files in projects and directly build.
- PerformancePkg has been dropped. The functionality will be available under UefiTestingPkg in mu_plus.
- UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLibMu has been dropped. Please use UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.
- MdeModulePkg/Library/PeiReportStatusCodeOnlyLib/PeiReportStatusCodeLib.inf has been dropped. Please use standard PeiReportStatusCodeLib.
- MuPreExitBootServices has been removed in favor of gEfiEventBeforeExitBootServicesGuid. gEfiEventBeforeExitBootServicesGuid can be signaled multiple times, so be sure to close the events that are being signaled when this is changed. 
- AmdSvsmLib is now a required library for IA32/X64 platforms using UefiCpuPkg/Library/MpInitLib/DxeMpInitLib,UefiCpuPkg/Library/MpInitLib/PeiMpInitLib.
- MdePkg/Library/CompilerIntrinsics no longer contains strcmp functionality.
  - The strcmp functionality moved to CryptoPkg/Library/BaseCryptLib/SysCall/CrtWrapper.c where it is available to the third-party crypto code that previously used the code from the library.
- NetworkPkg requires EFI_RNG_PROTOCOL and EFI_HASH2_PROTOCOL to be available. This is part of CVE-2023-45237.
- CryptoBinPkg requires the platform to supply EFI_RNG_PPI in the Pei phase, and EFI_RNG_PROTOCOL in the Dxe phase.
  - RngPei can be used to produce the RNG PPI. See [SecurityPkg/RandomNumberGenerator/RngPei](https://github.com/microsoft/mu_tiano_plus/tree/release/202405/SecurityPkg/RandomNumberGenerator/RngPei) in Mu Tiano Plus.
- TOOL_CHAIN_TAG=GCC and TOOL_CHAIN_TAG=GCC5 are both supported. GCC will become the normal in a future release.
- ImageValidation will now run on all systems with a default configuration. This will verify generated images are ready for memory protections. Details available [Here](.pytool\Plugin\ImageValidation\ReadMe.md).

Main Changes-dev
----------------
- Add SPI bus driver stack
- NetworkPkg: Predictable TCP ISNs
- NetworkPkg: Use of a Weak PseudoRandom Number Generator
- UefiCpuPkg: Add new SmmRelocationLib library

Platform Integration Reference
----------------
Reference platforms which consume release/202405 are available in [mu_tiano_platforms](https://github.com/microsoft/mu_tiano_platforms).


Code of Conduct
===============

This project has adopted the Microsoft Open Source Code of Conduct https://opensource.microsoft.com/codeofconduct/

For more information see the Code of Conduct FAQ https://opensource.microsoft.com/codeofconduct/faq/
or contact `opencode@microsoft.com <mailto:opencode@microsoft.com>`_. with any additional questions or comments.

Contributions
=============

Contributions are always welcome and encouraged!
Please open any issues in the Project Mu GitHub tracker and read https://microsoft.github.io/mu/How/contributing/

For documentation:

Copyright & License
===================

| Copyright (C) Microsoft Corporation
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

.. _Windows_VS2022: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=39&&branchName=release%2F202405
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202405
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/39.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=40&branchName=release%2F202405
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20Ubuntu%20GCC5?branchName=release%2F202405
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/40.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202405
