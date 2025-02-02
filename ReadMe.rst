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

Branch Status - release/202411
==============================

:Status:
  In Development

:Entered Development:
  2024/11/24 (Date Edk2 started accepting changes which were not in a previous release)

:Anticipated Stabilization:
  May 2025

Branch Changes - release/202411
===============================

Breaking Changes-dev
--------------------
- MemoryBinOverrideLib and associated changes have been deprecated and dropped. Platforms should remove this library instance. Platforms which need this functionality should switch to using standard Edk method of creating gEfiMemoryTypeInformationGuid HOBS.
- NetworkPkg/NetworkPcds.dsc.inc has been removed. NetworkPkg/NetworkFixedPcds.dsc.inc and NetworkPkg/NetworkDynamicPcds.dsc.inc should be used instead.
- Sec type modules have been changed to USER_DEFINED. Platforms use a custom rules `Rules.Common.SEC`` will need to update with the new type.
- Stack Cookie support has been upstreamed into EDK2. Projects should define `CUSTOM_STACK_CHECK_LIB`` in their DSC file if they plan to use StackCheckLibStaticInit or StackCheckLibDynamicInit.

Main Changes-dev
----------------

MU Overrides on EDK2
--------------------
- At the end of 202405, mu_basecore contained 470 commits on top of edk2-stable202405.
- At the start of 202411, mu_basecore contains 286 commits on top of edk2-stable202411.
Full MU changes list can be viewed `in the changelog <https://github.com/microsoft/mu_basecore/compare/83b58641933d55d6efe5ff3b71a85e9868e6abd9...dev/202411>`_.

Platform Integration Reference
------------------------------
Reference platforms which consume release/202411 are available in `mu_tiano_platforms <https://github.com/microsoft/mu_tiano_platforms>`_.

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

.. _Windows_VS2022: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=39&&branchName=release%2F202411
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202411
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/39.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=40&branchName=release%2F202411
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20Ubuntu%20GCC5?branchName=release%2F202411
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/40.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202411
