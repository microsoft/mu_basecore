## @file
# UnitTestFrameworkPkg DSC include file for host and target based test DSC
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[LibraryClasses]
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UnitTestPersistenceLib|UnitTestFrameworkPkg/Library/UnitTestPersistenceLibNull/UnitTestPersistenceLibNull.inf
  UnitTestResultReportLib|UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibDebugLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  NULL|UnitTestFrameworkPkg/Library/UnitTestDebugAssertLib/UnitTestDebugAssertLib.inf

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x17
