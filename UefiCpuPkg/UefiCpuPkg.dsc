## @file
#  UefiCpuPkg Package
#
#  Copyright (c) 2007 - 2021, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = UefiCpu
  PLATFORM_GUID                  = a1b7be22-78b3-4260-9569-8649e8c17d49
  PLATFORM_VERSION               = 0.90
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/UefiCpu
  # MU_CHANGE START
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64|ARM
  # MU_CHANGE END
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

#
# External libraries to build package
#

!include MdePkg/MdeLibs.dsc.inc
## MU_CHANGE Begin
[LibraryClasses.ARM, LibraryClasses.AARCH64]
  #
  # It is not possible to prevent the ARM compiler for generic intrinsic functions.
  # This library provides the instrinsic functions generate by a given compiler.
  # [LibraryClasses.ARM, LibraryClasses.AARCH64] and NULL mean link this library
  # into all ARM and AARCH64 images.
  #
  #NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf
  NULL|MdePkg/Library/CompilerIntrinsicsLib/ArmCompilerIntrinsicsLib.inf
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf
## MU_CHANGE End

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  StandaloneMmDriverEntryPoint|MdePkg/Library/StandaloneMmDriverEntryPoint/StandaloneMmDriverEntryPoint.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  SmmCpuPlatformHookLib|UefiCpuPkg/Library/SmmCpuPlatformHookLibNull/SmmCpuPlatformHookLibNull.inf
  SmmCpuFeaturesLib|UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  VmgExitLib|UefiCpuPkg/Library/VmgExitLibNull/VmgExitLibNull.inf
  MicrocodeLib|UefiCpuPkg/Library/MicrocodeLib/MicrocodeLib.inf

##MSCHANGE Begin
  DxeMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLibNull/DxeMemoryProtectionHobLibNull.inf
  MmMemoryProtectionHobLib|MdeModulePkg/Library/MemoryProtectionHobLibNull/MmMemoryProtectionHobLibNull.inf

[LibraryClasses.X64, LibraryClasses.IA32]
!if $(TARGET) == DEBUG
!if $(TOOL_CHAIN_TAG) == VS2017 or $(TOOL_CHAIN_TAG) == VS2015 or $(TOOL_CHAIN_TAG) == VS2019
  #if debug is enabled provide StackCookie support lib so that we can link to /GS exports
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  NULL|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
!endif
!endif
HwResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
##MSCHANGE End

[LibraryClasses.common.SEC]
  PlatformSecLib|UefiCpuPkg/Library/PlatformSecLibNull/PlatformSecLibNull.inf
!if $(TOOL_CHAIN_TAG) == "XCODE5"
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/Xcode5SecPeiCpuExceptionHandlerLib.inf
!else
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf
!endif
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf

[LibraryClasses.common.PEIM]
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/PeiMpInitLib.inf
  RegisterCpuFeaturesLib|UefiCpuPkg/Library/RegisterCpuFeaturesLib/PeiRegisterCpuFeaturesLib.inf
  CpuCacheInfoLib|UefiCpuPkg/Library/CpuCacheInfoLib/PeiCpuCacheInfoLib.inf

[LibraryClasses.IA32.PEIM, LibraryClasses.X64.PEIM]
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/PeiCpuExceptionHandlerLib.inf

[LibraryClasses.common.DXE_DRIVER]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
  RegisterCpuFeaturesLib|UefiCpuPkg/Library/RegisterCpuFeaturesLib/DxeRegisterCpuFeaturesLib.inf
  CpuCacheInfoLib|UefiCpuPkg/Library/CpuCacheInfoLib/DxeCpuCacheInfoLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf

[LibraryClasses.common.MM_STANDALONE]
  MmServicesTableLib|MdePkg/Library/StandaloneMmServicesTableLib/StandaloneMmServicesTableLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

#
# Drivers/Libraries within this package
#

# MU_CHANGE START
[Components.X64, Components.IA32]
  UefiCpuPkg/CpuIoPei/CpuIoPei.inf
  UefiCpuPkg/Library/CpuCommonFeaturesLib/CpuCommonFeaturesLib.inf
  UefiCpuPkg/Application/Cpuid/Cpuid.inf

[Components]
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  #UefiCpuPkg/CpuIoPei/CpuIoPei.inf                           # MU_CHANGE - Move to X64, IA32 section.
  UefiCpuPkg/Library/SecPeiDxeTimerLibUefiCpu/SecPeiDxeTimerLibUefiCpu.inf
  #UefiCpuPkg/Application/Cpuid/Cpuid.inf                     # MU_CHANGE - Move to X64, IA32 section.
[Components.IA32, Components.X64]
# MU_CHANGE END
  UefiCpuPkg/Library/CpuTimerLib/BaseCpuTimerLib.inf
  UefiCpuPkg/Library/CpuCacheInfoLib/PeiCpuCacheInfoLib.inf
  UefiCpuPkg/Library/CpuCacheInfoLib/DxeCpuCacheInfoLib.inf
  UefiCpuPkg/MicrocodeMeasurementDxe/MicrocodeMeasurementDxe.inf

[Components.IA32, Components.X64]
  UefiCpuPkg/CpuDxe/CpuDxe.inf
  UefiCpuPkg/CpuFeatures/CpuFeaturesPei.inf {
    <LibraryClasses>
      NULL|UefiCpuPkg/Library/CpuCommonFeaturesLib/CpuCommonFeaturesLib.inf
  }
  UefiCpuPkg/CpuFeatures/CpuFeaturesDxe.inf {
    <LibraryClasses>
      NULL|UefiCpuPkg/Library/CpuCommonFeaturesLib/CpuCommonFeaturesLib.inf
  }
  UefiCpuPkg/CpuIo2Smm/CpuIo2Smm.inf
  UefiCpuPkg/CpuIo2Smm/CpuIo2StandaloneMm.inf
  UefiCpuPkg/CpuMpPei/CpuMpPei.inf
  UefiCpuPkg/CpuS3DataDxe/CpuS3DataDxe.inf
  UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.inf
  UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  UefiCpuPkg/Library/CpuCommonFeaturesLib/CpuCommonFeaturesLib.inf
  UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
!if $(TOOL_CHAIN_TAG) != "XCODE5"
  UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf
!endif
  UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf
  UefiCpuPkg/Library/CpuExceptionHandlerLib/PeiCpuExceptionHandlerLib.inf
  UefiCpuPkg/Library/CpuExceptionHandlerLib/Xcode5SecPeiCpuExceptionHandlerLib.inf
  UefiCpuPkg/Library/MpInitLib/PeiMpInitLib.inf
  UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
  UefiCpuPkg/Library/MpInitLibUp/MpInitLibUp.inf
  UefiCpuPkg/Library/MicrocodeLib/MicrocodeLib.inf
  UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  UefiCpuPkg/Library/PlatformSecLibNull/PlatformSecLibNull.inf
  UefiCpuPkg/Library/RegisterCpuFeaturesLib/PeiRegisterCpuFeaturesLib.inf
  UefiCpuPkg/Library/RegisterCpuFeaturesLib/DxeRegisterCpuFeaturesLib.inf
  UefiCpuPkg/Library/SmmCpuPlatformHookLibNull/SmmCpuPlatformHookLibNull.inf
  UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLib.inf
  UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLibStm.inf
  UefiCpuPkg/Library/SmmCpuFeaturesLib/StandaloneMmCpuFeaturesLib.inf
  UefiCpuPkg/Library/VmgExitLibNull/VmgExitLibNull.inf
  UefiCpuPkg/PiSmmCommunication/PiSmmCommunicationPei.inf
  UefiCpuPkg/PiSmmCommunication/PiSmmCommunicationSmm.inf
  UefiCpuPkg/SecCore/SecCore.inf
  UefiCpuPkg/SecCore/SecCoreNative.inf
  UefiCpuPkg/SecMigrationPei/SecMigrationPei.inf
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf {
    <Defines>
      FILE_GUID = D1D74FE9-7A4E-41D3-A0B3-67F13AD34D94
    <LibraryClasses>
      SmmCpuFeaturesLib|UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLibStm.inf
  }
  UefiCpuPkg/Universal/Acpi/S3Resume2Pei/S3Resume2Pei.inf
  UefiCpuPkg/ResetVector/Vtf0/Bin/ResetVector.inf

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
