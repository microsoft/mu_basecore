## @file
# Mde DSC include file for [LibraryClasses*] section of all Architectures.
#
# This file can be included to the [LibraryClasses*] section(s) of a platform DSC file
# by using "!include MdePkg/MdeLibs.dsc.inc" to specify the library instances
# of some EDKII basic/common library classes.
#
# Copyright (c) 2021 - 2022, Intel Corporation. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[LibraryClasses]
  ArmTrngLib|MdePkg/Library/BaseArmTrngLibNull/BaseArmTrngLibNull.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  SmmCpuRendezvousLib|MdePkg/Library/SmmCpuRendezvousLibNull/SmmCpuRendezvousLibNull.inf

# MU_CHANGE [BEGIN] - Add Stack Cookie Support
[LibraryClasses.X64.SEC, LibraryClasses.X64.PEIM, LibraryClasses.X64.PEI_CORE, LibraryClasses.X64.SMM_CORE, LibraryClasses.X64.DXE_SMM_DRIVER, LibraryClasses.X64.MM_CORE_STANDALONE, LibraryClasses.X64.MM_STANDALONE, LibraryClasses.X64.DXE_CORE, LibraryClasses.X64.DXE_DRIVER, LibraryClasses.X64.DXE_RUNTIME_DRIVER, LibraryClasses.X64.DXE_SAL_DRIVER, LibraryClasses.X64.UEFI_DRIVER, LibraryClasses.X64.UEFI_APPLICATION]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  NULL|MdePkg/Library/StackCheckLib/StackCheckLib.inf
  StackCheckFailureLib|MdePkg/Library/StackCheckFailureLibNull/StackCheckFailureLibNull.inf
# MU_CHANGE [END] - Add Stack Cookie Support