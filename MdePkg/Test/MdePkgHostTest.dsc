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

[PcdsFeatureFlag]
  !if "$(FAMILY)" == "MSFT" || "$(TOOL_CHAIN_TAG)" == "CLANGPDB"
    gEfiMdePkgTokenSpaceGuid.PcdEnableMsvcStyleStackChecking|TRUE
  !endif

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
  MdePkg/Test/Library/RngLibHostTestLfsr/RngLibHostTestLfsr.inf
  # MU_CHANGE [END]
  # MU_CHANGE [BEGIN]
  MdePkg/Test/Library/SynchronizationLibHostUnitTest/SynchronizationLibHostUnitTest.inf
  # MU_CHANGE [END]

  #
  # StackCheckLib unit test. This is only run on GCC because the Windows C runtime provides
  # its own stack cookie mechanism that cannot be overridden and GCC is the only toolchain
  # that produces ELF binaries that also has stack cookies enabled.
  #
  !if "$(TOOL_CHAIN_TAG)" == "GCC"
  MdePkg/Test/GoogleTest/Library/StackCheckLib/GoogleTestStackCheckLib.inf {
    <LibraryClasses>
      StackCheckLib|MdePkg/Library/StackCheckLib/StackCheckLib.inf
    <BuildOptions>
      #
      # Disable ASAN so only the stack cookie check is tested.
      # ASAN catches stack buffer overflows before the stack cookie and would mask the test.
      # Enable stack cookie checking for all functions to guarantee we should expect the stack cookie
      # to be corrupted.
      #
      GCC:*_GCC_*_CC_FLAGS  = -fno-sanitize=address -fstack-protector-all
  }
  !endif

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
  MdePkg/Test/Mock/Library/GoogleTest/MockPostCodeLib/MockPostCodeLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockSmmServicesTableLib/MockSmmServicesTableLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockCpuLib/MockCpuLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPciSegmentLib/MockPciSegmentLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockReportStatusCodeLib/MockReportStatusCodeLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockSafeIntLib/MockSafeIntLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockDevicePathLib/MockDevicePathLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockDxeServicesLib/MockDxeServicesLib.inf

  MdePkg/Library/StackCheckLibNull/StackCheckLibNullHostApplication.inf

  MdePkg/Test/Mock/Library/GoogleTest/MockIoLib/MockIoLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockMmServicesTableLib/MockMmServicesTableLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeLib/MockUefiRuntimeLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockMemoryAllocationLib/MockMemoryAllocationLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPciExpressLib/MockPciExpressLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockUefiDevicePathLib/MockUefiDevicePathLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPciLib/MockPciLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockDxeServicesTableLib/MockDxeServicesTableLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPcdLib/MockPcdLib.inf
  MdePkg/Test/Mock/Library/Cmocka/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
  MdePkg/Test/Mock/Library/Cmocka/MockUefiBootServicesTableLib/MockUefiBootServicesTableLib.inf
  MdePkg/Test/Mock/Library/Stub/StubHobLib/StubHobLib.inf
  MdePkg/Test/Mock/Library/Stub/StubUefiLib/StubUefiLib.inf

  MdePkg/Test/Mock/Library/GoogleTest/MockSafeIntLib/MockSafeIntLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockSmmMemLib/MockSmmMemLib.inf
  MdePkg/Test/Mock/Library/GoogleTest/MockPerformanceLib/MockPerformanceLib.inf # MU_CHANGE
  MdePkg/Test/Mock/Library/GoogleTest/MockSynchronizationLib/MockSynchronizationLib.inf # MU_CHANGE
  MdePkg/Test/Mock/Library/GoogleTest/MockPciCf8Lib/MockPciCf8Lib.inf
 
