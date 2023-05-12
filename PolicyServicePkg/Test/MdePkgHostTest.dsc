## @file
# PolicyServicePkg DSC file used to build host-based unit tests.
#
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = PolicyServicePkgHostTest
  PLATFORM_GUID           = 77C72A3D-89BE-4521-BC99-33E25DC59F6D
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/PolicyServicePkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64|AARCH64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

#[LibraryClasses]
#  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf

[Components]
  PolicyServicePkg/Test/Mock/Library/GoogleTest/MockPolicyLib/MockPolicyLib.inf
# #
# # Build HOST_APPLICATION that tests the SafeIntLib
# #
# MdePkg/Test/UnitTest/Library/BaseSafeIntLib/TestBaseSafeIntLibHost.inf
# MdePkg/Test/UnitTest/Library/BaseLib/BaseLibUnitTestsHost.inf
# MdePkg/Test/GoogleTest/Library/BaseSafeIntLib/GoogleTestBaseSafeIntLib.inf
# # MU_CHANGE [BEGIN]
# MdePkg/Test/Library/MockUefiBootServicesTableLib/MockUefiBootServicesTableLib.inf
# MdePkg/Test/Library/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
# MdePkg/Test/Library/RngLibHostTestLfsr/RngLibHostTestLfsr.inf
# MdePkg/Test/Library/StubHobLib/StubHobLib.inf
# MdePkg/Test/Library/StubUefiLib/StubUefiLib.inf
# MdePkg/Test/Library/SynchronizationLibHostUnitTest/SynchronizationLibHostUnitTest.inf
# # MU_CHANGE [END]
#
# #
# # Build HOST_APPLICATION Libraries
# #
# MdePkg/Library/BaseLib/UnitTestHostBaseLib.inf
# MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.inf
# MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
