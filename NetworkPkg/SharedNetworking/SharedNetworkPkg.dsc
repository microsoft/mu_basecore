## @file
# UEFI 2.4 Network Module Package for All Architectures
#
# (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
# Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
#
#    This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

[Defines]
  PLATFORM_NAME                  = SharedNetworkPkg
  PLATFORM_GUID                  = 3FD34E9B-E90C-44e1-B510-1F632A509F10
  PLATFORM_VERSION               = 0.98
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/SharedNetworkPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|EBC|ARM|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = NetworkPkg/SharedNetworking/SharedNetworkPkg.fdf
  NETWORK_ENABLE = TRUE
  NETWORK_TLS_ENABLE = TRUE
  NETWORK_ALLOW_HTTP_CONNECTIONS = TRUE
  NETWORK_SNP_ENABLE = TRUE
  !include NetworkPkg/NetworkDefines.dsc.inc


## MU_CHANGE Begin
[LibraryClasses.ARM, LibraryClasses.AARCH64]
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf
## MU_CHANGE End

[LibraryClasses]
  !include NetworkPkg/NetworkLibs.dsc.inc
  DebugLib|MdePkg/Library/UefiDebugLibDebugPortProtocol/UefiDebugLibDebugPortProtocol.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  MemoryTypeInformationChangeLib|MdeModulePkg/Library/MemoryTypeInformationChangeLibNull/MemoryTypeInformationChangeLibNull.inf  # MU_CHANGE
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  BaseBinSecurityLibRng|MdePkg/Library/BaseBinSecurityLibNull/BaseBinSecurityLibNull.inf

  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf

[LibraryClasses.common.DXE_CORE, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.UEFI_DRIVER]
    # Common\MU_TIANO\CryptoPkg\Library\BaseCryptLibOnProtocolPpi\DxeCryptLib.inf
    BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf
    TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf

[LibraryClasses.common.PEIM]
    BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf
    TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf

[LibraryClasses.x64.DXE_SMM_DRIVER, LibraryClasses.x64.SMM_CORE]
    BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf
    TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf

##MSCHANGE Begin
!if $(TOOL_CHAIN_TAG) == VS2019 or $(TOOL_CHAIN_TAG) == VS2022
[LibraryClasses.X64]
  # Provide StackCookie support lib so that we can link to /GS exports for VS builds
  NULL|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
  BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
!endif
##MSCHANGE End
[LibraryClasses.DXE_RUNTIME_DRIVER, LibraryClasses.DXE_CORE]
  DebugLib|MdePkg/Library/UefiDebugLibDebugPortProtocol/UefiDebugLibDebugPortProtocol.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

[LibraryClasses.DXE_SMM_DRIVER, LibraryClasses.X64.SMM_CORE]
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf

[LibraryClasses.common.UEFI_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_APPLICATION, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_CORE, LibraryClasses.common.UEFI_DRIVER]
  #DXE_CORE DXE_DRIVER DXE_RUNTIME_DRIVER UEFI_APPLICATION UEFI_DRIVER
  DebugLib|MdePkg/Library/UefiDebugLibDebugPortProtocol/UefiDebugLibDebugPortProtocol.inf

[LibraryClasses.common.PEIM]
  DebugLib|MsCorePkg/Library/PeiDebugLib/PeiDebugLib.inf

[LibraryClasses.IA32, LibraryClasses.X64]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  RngLib|SecurityPkg/RandomNumberGenerator/RngDxeLib/RngDxeLib.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  #
  # It is not possible to prevent ARM compiler calls to generic intrinsic functions.
  # This library provides the instrinsic functions generated by a given compiler.
  # [LibraryClasses.ARM] and NULL mean link this library into all ARM images.
  #
!if $(TOOL_CHAIN_TAG) != VS2017 or $(TOOL_CHAIN_TAG) != VS2015 or $(TOOL_CHAIN_TAG) != VS2019 or $(TOOL_CHAIN_TAG) != VS2022 ## MS_CHANGE only applies to ARM compiler
  # NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf       # MU_CHANGE
  NULL|MdePkg/Library/CompilerIntrinsicsLib/ArmCompilerIntrinsicsLib.inf  # MU_CHANGE
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf  # while building with MSVC, we can't process the s files
!endif
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf
  # ArmSoftFloatLib|ArmPkg/Library/ArmSoftFloatLib/ArmSoftFloatLib.inf    # MU_CHANGE


[PcdsFeatureFlag]
  gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable|TRUE

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################

[Components]
  #NetworkPkg/WifiConnectionManagerDxe/WifiConnectionManagerDxe.inf # we don't care about wifi connections yet
  NetworkPkg/Application/VConfig/VConfig.inf

  !include NetworkPkg/NetworkComponents.dsc.inc

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES


[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER, BuildOptions.common.EDKII.DXE_SMM_DRIVER, BuildOptions.common.EDKII.SMM_CORE, BuildOptions.common.EDKII.DXE_DRIVER]
  MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:4096 # enable 4k alignment for MAT and other protections.
  MSFT:*_*_X64_DLINK_FLAGS = /ALIGN:4096 # enable 4k alignment for MAT and other protections.