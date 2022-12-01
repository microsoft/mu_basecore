## @file
# MdeModulePkg DSC file used to build host-based unit tests.
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = MdeModulePkgHostTest
  PLATFORM_GUID           = F74AF7C6-698C-4EBA-BA49-FF6816916354
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/MdeModulePkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  SecurityLockAuditLib|MdeModulePkg/Library/SecurityLockAuditLibNull/SecurityLockAuditLibNull.inf    # MU_CHANGE

[Components]
  # MdeModulePkg/Library/DxeResetSystemLib/UnitTest/MockUefiRuntimeServicesTableLib.inf # MU_CHANGE - Move lib to correct home

  #
  # Build MdeModulePkg HOST_APPLICATION Tests
  #
  MdeModulePkg/Library/DxeResetSystemLib/UnitTest/DxeResetSystemLibUnitTestHost.inf {
    <LibraryClasses>
      ResetSystemLib|MdeModulePkg/Library/DxeResetSystemLib/DxeResetSystemLib.inf
      UefiRuntimeServicesTableLib|MdePkg/Test/Library/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
  }

  MdeModulePkg/Universal/Variable/RuntimeDxe/RuntimeDxeUnitTest/VariableLockRequestToLockUnitTest.inf {
    <LibraryClasses>
      VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
      VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdAllowVariablePolicyEnforcementDisable|TRUE
  }

  # MU_CHANGE [BEGIN] - Add a host-based unit test for common variable services code.
  MdeModulePkg/Universal/Variable/RuntimeDxe/RuntimeDxeUnitTest/VariableRuntimeDxeUnitTest.inf {
    <LibraryClasses>
      UefiLib|MdePkg/Test/Library/StubUefiLib/StubUefiLib.inf
      VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
      VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
      TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
      AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
      DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
      UefiRuntimeServicesTableLib|MdePkg/Test/Library/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
      UefiBootServicesTableLib|MdePkg/Test/Library/MockUefiBootServicesTableLib/MockUefiBootServicesTableLib.inf
      SynchronizationLib|MdePkg/Test/Library/SynchronizationLibHostUnitTest/SynchronizationLibHostUnitTest.inf
      VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
      HobLib|MdePkg/Test/Library/StubHobLib/StubHobLib.inf

      VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|MdeModulePkg/Library/VarCheckPolicyLib/VarCheckPolicyLibVariableDxe.inf

    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdAllowVariablePolicyEnforcementDisable|TRUE
      gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
      # SCT tests are noisy, so disable VERBOSE.
      gUnitTestFrameworkPkgTokenSpaceGuid.PcdUnitTestLogLevel|0x00000007
  }
  # MU_CHANGE [END] - Add a host-based unit test for common variable services code.

  MdeModulePkg/Library/UefiSortLib/UnitTest/UefiSortLibUnitTest.inf {
    <LibraryClasses>
      UefiSortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      # MU_CHANGE
      # The UefiBootServicesTableLib cannot be used in a generic way, but is
      # safe for this module because it only needs the symbol defined. gBS is
      # never actually used.
      UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  }

  MdeModulePkg/Bus/Pci/NvmExpressDxe/UnitTest/MediaSanitizeUnitTestHost.inf {
    <LibraryClasses>
      NvmExpressDxe|MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf
  }

  # MU_CHANGE [BEGIN]
  MdeModulePkg/Library/VariablePolicyLib/VariablePolicyUnitTest/VariablePolicyUnitTest.inf {
    <LibraryClasses>
      VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf

    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdAllowVariablePolicyEnforcementDisable|TRUE
  }

  MdeModulePkg/Core/Dxe/UnitTest/MemoryProtectionUnitTestHost.inf {
    <LibraryClasses>
      HobLib|MdeModulePkg/Library/BaseHobLibNull/BaseHobLibNull.inf
      PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
      DxeMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLibNull/DxeMemoryProtectionHobLibNull.inf
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
      UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
      UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
      PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
      UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
      PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
      PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
      ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
      DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
      DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
      CpuExceptionHandlerLib|MdeModulePkg/Library/CpuExceptionHandlerLibNull/CpuExceptionHandlerLibNull.inf
      BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibNull/BaseBinSecurityLibNull.inf
      UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
      MemoryBinOverrideLib|MdeModulePkg/Library/MemoryBinOverrideLibNull/MemoryBinOverrideLibNull.inf  # MU_CHANGE

    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdLoadModuleAtFixAddressEnable|0
      gEfiMdeModulePkgTokenSpaceGuid.PcdLoadFixAddressRuntimeCodePageNumber|0
      gEfiMdeModulePkgTokenSpaceGuid.PcdLoadFixAddressBootTimeCodePageNumber|0
  }
  # MU_CHANGE [END]
  
  MdeModulePkg/Library/UefiSortLib/GoogleTest/UefiSortLibGoogleTest.inf {
    <LibraryClasses>
      UefiSortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  }

  #
  # Build HOST_APPLICATION Libraries
  #
  MdeModulePkg/Test/Mock/Library/GoogleTest/MockPciHostBridgeLib/MockPciHostBridgeLib.inf
