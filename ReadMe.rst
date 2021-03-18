==============================
Project Mu Basecore Repository
==============================

============================= ================= =============== ===================
 Host Type & Toolchain        Build Status      Test Status     Code Coverage
============================= ================= =============== ===================
Windows_VS2019_               |WindowsCiBuild|  |WindowsCiTest| |WindowsCiCoverage|
Ubuntu_GCC5_                  |UbuntuCiBuild|   |UbuntuCiTest|  |UbuntuCiCoverage|
============================= ================= =============== ===================

This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu

Branch Status - release/202102
==============================

:Status:
  In Development

:Entered Development:
  2021/03/12

:Anticipated Stabilization:
  May 2021

Branch Changes - release/202102
===============================

Breaking Changes-dev
--------------------

- None

Main Changes-dev
----------------

- None

Bug Fixes-dev
-------------

- None

2102_RefBoot Changes
--------------------

- VariablePolicy has changed significantly since release/202008
    - The previously Mu-only infrastructure has been ported upstream, and the changes made with the community
      require attention on the platform side
    - MdeModulePkg/Library/UefiVariablePolicyLib/UefiVariablePolicyLib.inf becomes
      MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf or MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf
      and the class changes
    - MdeModulePkg/Library/MuVariablePolicyHelperLib/MuVariablePolicyHelperLib.inf becomes
      MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf and the
      class changes
    - MdeModulePkg/Universal/Variable/UefiVariablePolicy/Library/VarCheckPolicyLib/VarCheckPolicyLib.inf moved to
      MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLib.inf
    - MdeModulePkg/Universal/Variable/UefiVariablePolicy/Dxe/VariablePolicyDxe.inf is no longer needed. It is
      now built-in to Variable Services
- An instance of MmUnblockMemoryLib must be added to support the StandaloneMm changes. If your platform does not
  already support StandaloneMm, you should be safe to use MdePkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLibNull.inf

2102_CIBuild Changes
--------------------

- MdeModulePkg/Universal/Variable/RuntimeDxe/PropertyBasedVarLockLib.inf is gone
    - This was not kept in the EDK2 integration. It is not required (and not supported).
- SecurityPkg/RandomNumberGenerator/RngDxeLib/RngDxeLib.inf is dropped in favor of EDK2 version.
    - Please use MdePkg/Library/DxeRngLib/DxeRngLib.inf.

2102_Rebase Changes
-------------------

| Starting commit: 150d5933348478cc97ea44221a1764f372fa61a6 (revised 202008 commit)
| Destination Commit from upstream edk2: edd46cd407ea4a0adaa8d6ca86f550c2a4d5c507 (tag: NONE - commit post 202102 tag)

* Add a AcquireLockOnlyAtBootTime() to VariablePolicySmmDxe.c
    - Open a bug for this.
* Make sure that it still makes sense to keep the SMM_VARIABLE_FUNCTION_LOCK_VARIABLE interface open.
    - Is there still a late-lock whitelist?
    - Probably should keep this change since, ultimately, the VarPolicy will disable locking.
    - Upstream it as a Bugzilla that will go away when VarLock is dropped, anyway.
* 542d38fdeb ("Revert "CHERRY-PICK: MdeModulePkg/SetupBrowserDxe: Fix IsZeroGuid() ASSERT."", 2020-03-23)
    - Dropping this commit. Believe it's a bad vestige of a bad CHERRY-PICK that was reverted.

Remaining to address from 202008:

* 8238a66de4 ("Update for VS2017", 2018-01-22)
    - Might try to drop this and see if it's still a problem for VS2017, otherwise upstream.
* 164b09676a ("REBASE: Revert "MdePkg: Added header file for Delayed Dispatch PPI"", 2020-06-17)
    - This should go away within this integration


Code of Conduct
===============

This project has adopted the Microsoft Open Source Code of Conduct https://opensource.microsoft.com/codeofconduct/

For more information see the Code of Conduct FAQ https://opensource.microsoft.com/codeofconduct/faq/
or contact `opencode@microsoft.com <mailto:opencode@microsoft.com>`_. with any additional questions or comments.

Contributions
=============

Contributions are always welcome and encouraged!
Please open any issues in the Project Mu GitHub tracker and read https://microsoft.github.io/mu/How/contributing/


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

.. _Windows_VS2019: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=39&&branchName=release%2F202102
.. |WindowsCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202102
.. |WindowsCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/39.svg
.. |WindowsCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. _Ubuntu_GCC5: https://dev.azure.com/projectmu/mu/_build/latest?definitionId=40&branchName=release%2F202102
.. |UbuntuCiBuild| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20Ubuntu%20GCC5?branchName=release%2F202102
.. |UbuntuCiTest| image:: https://img.shields.io/azure-devops/tests/projectmu/mu/40.svg
.. |UbuntuCiCoverage| image:: https://img.shields.io/badge/coverage-coming_soon-blue

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/CI/Mu%20Basecore%20CI%20VS2019?branchName=release%2F202102
