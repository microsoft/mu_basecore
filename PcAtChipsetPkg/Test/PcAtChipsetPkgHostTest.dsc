# MU_CHANGE [BEGIN]

## @file
# MdePkg DSC file used to build host-based unit tests.
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = PcAtChipsetPkgHostTest
  PLATFORM_GUID           = B055AEEA-1797-4C67-9914-F54E64ECC1EB
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/PcAtChipsetPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]

[Components]
  #
  # TODO: why is there no HOST_APPLICATION that tests the libraries in this package?
  #
  PcAtChipsetPkg/Test/Library/MockResetSystemLib/MockResetSystemLib.inf
  
  # MU_CHANGE [END]
