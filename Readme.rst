==============================
Project Mu Basecore Repository
==============================

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/mu_basecore%20PR%20gate?branchName=release/201903

|build_status_windows| Current build status for release/201903

This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu

Branch Changes - release/201903
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

1903_CIBuild Changes
--------------------

- Move LockBoxLibPei and add Null lib to satisfy modules that aren't IA32 or X64 compatible.
- Update all references to EfiResetSystem to ResetSystem to match upstream.

1903_Rebase Changes
-------------------

Source Commit from 201811: 713696c6bf

- NvmExpressHci.c - Near the bottom, is this ReportStatusCode in a bad place? Bad merge?
- SafeString.c - Dropped the custom call to AsciiToUpper(). Replaced with Tiano fix.
- String.c - Take Tiano implementation of Base64 encode/decode.
- BmBoot.c - Changed to BmReportLoadFailure() for reporting. Lost some data. Restore if still required.
- ResetSystemLib.h - Switch from EfiResetSystem to ResetSystem. Make sure everything still works.
- Trim.py - Make sure the Tiano fix works for what we need with .iii files.
- *.template.ms - When do we perform this?
- Make sure bb9faf8d02a7405a163592798a6fa3a18926b18b changes for BaseTools/Source are preserved.
- Evaluate the effects of dropping 8083ded2580ff01ceaa88fcd2c4a900b195f604e for ResetSystem.

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

Copyright (c) 2016-2018, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Upstream License (TianoCore)
============================

Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.
Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.
Copyright (c) 2011 - 2015, ARM Limited. All rights reserved.
Copyright (c) 2014 - 2015, Linaro Limited. All rights reserved.
Copyright (c) 2013 - 2015, Red Hat, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
