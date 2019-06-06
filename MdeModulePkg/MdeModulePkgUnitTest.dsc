## @file MdeModulePkgUnitTest.dsc
#
##
# Copyright (C) Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

[Defines]
  PLATFORM_NAME                  = MdeModulePkgUnitTest
  PLATFORM_GUID                  = 4C0252C3-C169-4619-B4A6-F4E6B914F4CC
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MdeModulePkgUnitTest
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT


[PcdsFixedAtBuild]


[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLibHost/BaseLibHost.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibHost/BaseMemoryLibHost.inf
  MemoryAllocationLib|MdePkg/Library/MemoryAllocationLibHost/MemoryAllocationLibHost.inf
  DebugLib|MdePkg/Library/DebugLibHost/DebugLibHost.inf

  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf

  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf

  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf

  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf

  OsServiceLib|UnitTestPkg/Library/OsServiceLibHost/OsServiceLibHost.inf

  UnitTestLib|CmockaHostUnitTestPkg/Library/UnitTestLibcmocka/UnitTestLibcmocka.inf
  UnitTestAssertLib|CmockaHostUnitTestPkg/Library/UnitTestAssertLibcmocka/UnitTestAssertLibcmocka.inf

  CmockaLib|CmockaHostUnitTestPkg/Library/CmockaLib/CmockaLib.inf

[LibraryClasses.common.USER_DEFINED]

[Components]
  MdeModulePkg/Library/UefiVariablePolicyLib/UefiVariablePolicyUnitTest/UefiVariablePolicyUnitTest.inf {
    <LibraryClasses>
      UefiVariablePolicyLib|MdeModulePkg/Library/UefiVariablePolicyLib/UefiVariablePolicyLib.inf
  }

!include MdePkg/UefiHostTestBuildOption.dsc
