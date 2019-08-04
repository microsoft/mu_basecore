==============================
Project Mu Basecore Repository
==============================

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/mu_basecore%20PR%20gate?branchName=dev/201905

|build_status_windows| Current build status for dev/201905

This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu

Branch Status - dev/201905
==============================

Status:
  In Development

Entered Development:
  2019/08/03

Anticipated Stabilization:
  September 2019

Branch Changes - dev/201905
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

1905_RefBoot Changes
--------------------

Platform Changes:

- Incomplete

1905_CIBuild Changes
--------------------

- Drop unnecessary files from MdeModulePkg.dsc
- Disable ARM compilation on NetworkPkg.

1905_Rebase Changes
-------------------

Source Commit from 201903: 1903_RC1

- DebugPrint changed in TC to match. Valist split from normal call. DebugPrintValist() turned into DebugPrintMarker()
- Might be able to drop parts of 97de7267b1. And 6a4c1c5a47.
- Change PcdAcpiS3Enable default to FALSE.
- Make sure that ResetSystemWithSubtype() still works as expected.
- There may be a TlsDxe change or PLATFORMX64_ENABLE change needed. Something about IA32 vs X64 vs AARCH64.
- All added files had their copyrights updated to match TianoCore.

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

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
