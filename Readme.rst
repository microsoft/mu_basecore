==============================
Project Mu Basecore Repository
==============================

.. |build_status_windows| image:: https://dev.azure.com/projectmu/mu/_apis/build/status/mu_basecore%20PR%20gate?branchName=release/201903

|build_status_windows| Current build status for release/201903

This repository is part of Project Mu.  Please see Project Mu for details https://microsoft.github.io/mu

Branch Status - release/201903
==============================

Status:
  In Development

Entered Development:
  2019/03/25

Anticipated Stabilization:
  May 2019

Branch Changes - release/201903
===============================

Breaking Changes-dev
--------------------

- Edk2ToolHelper no longer has logic for dependencies. This is easily replaced, but convenient to leave out while GenerateCapsule is being refactored to remove reliance on older compiled tools.
- VariablePolicy changes in VaraibleSmm and AuthService will require additional libraries in DSC files (e.g. UefiVariablePolicyLib, VarCheckPolicyLib, MuVariablePolicyHelperLib).
- gEdkiiVariableLockProtocolGuid is no longer a component of VariableServices. An alternate implementation is provided in Mu Plus repo, in the MuVarPolicyFoundationDxe driver, but it is recommended that policies be used instead.

Main Changes-dev
----------------

- Modifies DxeCapsuleProcessLib so that UpdateImageProgress() returns success even if DisplayUpdateProgress() returns EFI_ERROR. This allows capsule update to proceed even if there are errors in the progress reporting.
- Integrate VariablePolicy libraries and DXE driver into the Variable Services stack. Include unit tests, even though unit test framework isn't quite in place.
- Switched to MuLogging Compiler error/warning parser and added output to console in addition to JUNIT XML
- Modified FmpAuthenticationLibPkcs7 to utilize the EKU checking capabilities in BaseCryptLib if a PCD (gFmpDevicePkgTokenSpaceGuid.PcdFmpDxeRequiredEKU) is set.

Bug Fixes-dev
-------------

- None

1903_RefBoot Changes
--------------------

Platform Changes:

- Remove the use of PcdPeiCoreMaxFvSupported, PcdPeiCoreMaxPeimPerFv and PcdPeiCoreMaxPpiSupported in platform code as they have been removed for BZ1405.
- Add a LibraryClass for MmServicesTableLib to satisfy new library requirement in FaultTolerantWrite.
- Override ResetSystemLib with RuntimeResetSystemLib in CapsuleRuntimeDxe.
- Change "COMPONENT_TYPE = Microcode" to "MODULE_TYPE = USER_DEFINED" for ucode modules. All EDK build definitions have been dropped from EDK2.
- Remove all references to $(EFI_SOURCE). This define is no longer set anywhere.

1903_CIBuild Changes
--------------------

- Add FaultTolerantWriteStandaloneMm and VariableStandaloneMm to the CI ignore list. Cannot be built without StandaloneMmPkg.
- Move LockBoxLibPei and add Null lib to satisfy modules that aren't IA32 or X64 compatible.
- Update all references to EfiResetSystem to ResetSystem to match upstream.

1903_Rebase Changes
-------------------

Source Commit from 201811: 713696c6bfaffbf2481aa13518268725e0c4b30c

- *.template.ms - When do we perform this?
- BmBoot.c - Changed to BmReportLoadFailure() for reporting. Lost some data. Restore if still required.
- NvmExpressHci.c - Near the bottom, is this ReportStatusCode in a bad place? Bad merge?
- ResetSystemLib.h - Switch from EfiResetSystem to ResetSystem. Make sure everything still works.
- Make sure bb9faf8d02a7405a163592798a6fa3a18926b18b changes for BaseTools/Source are preserved.
- Evaluate the effects of dropping 8083ded2580ff01ceaa88fcd2c4a900b195f604e for ResetSystem.
- Trim.py - TianoCore fix slightly different than Mu fix. Confirmed compatible.
- String.c - Take Tiano implementation of Base64 encode/decode.
- SafeString.c - Dropped the custom call to AsciiToUpper(). Replaced with Tiano fix.

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
