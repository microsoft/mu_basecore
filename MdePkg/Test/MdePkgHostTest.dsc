## @file
# MdePkg DSC file used to build host-based unit tests.
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = MdePkgHostTest
  PLATFORM_GUID           = 50652B4C-88CB-4481-96E8-37F2D0034440
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/MdePkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLibBase.inf

[Components]
  #
  # Build HOST_APPLICATION that tests the SafeIntLib
  #
  MdePkg/Test/UnitTest/Library/BaseSafeIntLib/TestBaseSafeIntLibHost.inf
  MdePkg/Test/UnitTest/Library/BaseLib/BaseLibUnitTestsHost.inf
  MdePkg/Test/GoogleTest/Library/BaseSafeIntLib/GoogleTestBaseSafeIntLib.inf
  MdePkg/Test/UnitTest/Library/DevicePathLib/TestDevicePathLibHost.inf
  #
  # BaseLib tests
  #
  MdePkg/Test/GoogleTest/Library/BaseLib/GoogleTestBaseLib.inf
  # MU_CHANGE [BEGIN]
  MdePkg/Test/Library/SynchronizationLibHostUnitTest/SynchronizationLibHostUnitTest.inf
  MdePkg/Test/Library/RngLibHostTestLfsr/RngLibHostTestLfsr.inf
  # MU_CHANGE [END]

  #
  # Build HOST_APPLICATION Libraries
  #
  MdePkg/Library/BaseLib/UnitTestHostBaseLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockUefiBootServicesTableLib/MockUefiBootServicesTableLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPeiServicesLib/MockPeiServicesLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockHobLib/MockHobLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockFdtLib/MockFdtLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockIoLib/MockIoLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockSmmServicesTableLib/MockSmmServicesTableLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockMmServicesTableLib/MockMmServicesTableLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeLib/MockUefiRuntimeLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockMemoryAllocationLib/MockMemoryAllocationLib.inf
  # MU_CHANGE [BEGIN]
  MdePkg/Test/Mock/Library/Cmocka/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
  MdePkg/Test/Mock/Library/Cmocka/MockUefiBootServicesTableLib/MockUefiBootServicesTableLib.inf
  MdePkg/Test/Mock/Library/Stub/StubHobLib/StubHobLib.inf
  MdePkg/Test/Mock/Library/Stub/StubUefiLib/StubUefiLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPciExpressLib/MockPciExpressLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockUefiDevicePathLib/MockUefiDevicePathLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPerformanceLib/MockPerformanceLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockDxeServicesTableLib/MockDxeServicesTableLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPciLib/MockPciLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPcdLib/MockPcdLib.inf
  # MU_CHANGE [END]
