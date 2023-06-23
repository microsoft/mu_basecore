## @file
# PolicyServicePkg DSC file used to build host-based unit tests.
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = PolicyServicePkgHostTest
  PLATFORM_GUID           = 94003CBE-B829-4A54-90C5-A622EE550181
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/PolicyServicePkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  HobLib|MdeModulePkg/Library/BaseHobLibNull/BaseHobLibNull.inf

[Components]
  PolicyServicePkg/PolicyService/DxeMm/UnitTest/DxeMmPolicyUnitTest.inf
  PolicyServicePkg/PolicyService/Pei/UnitTest/PeiPolicyUnitTest.inf

