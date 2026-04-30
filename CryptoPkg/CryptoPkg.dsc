## @file
#  Cryptographic Library Package for UEFI Security Implementation.
#  PEIM, DXE Driver, and SMM Driver with all crypto services enabled.
#
#  Copyright (c) 2009 - 2022, Intel Corporation. All rights reserved.<BR>
#  Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#  Copyright (c) 2022, Loongson Technology Corporation Limited. All rights reserved.<BR>
#  Copyright (c) 2023, Arm Limited. All rights reserved.<BR>
#  Copyright (c) 2024, American Megatrends International LLC. All rights reserved.<BR>
#  Copyright (c) Microsoft Corporation.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = CryptoPkg
  PLATFORM_GUID                  = E1063286-6C8C-4c25-AEF0-67A9A5B6E6B6
  PLATFORM_VERSION               = 0.98
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/CryptoPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64|RISCV64|LOONGARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  RngLib|MdePkg/Library/BaseRngLibNull/BaseRngLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  #OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf # MU_CHANGE Remove OpenSslLib
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  HmacSha1Lib|CryptoPkg/Library/HmacSha1Lib/HmacSha1LibNull.inf # MU_CHANGE add HmacSha1Lib
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf # MU_CHANGE

[LibraryClasses.IA32, LibraryClasses.X64, LibraryClasses.AARCH64]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.AARCH64]
  # MU_CHANGE [BEGIN]: Remove ArmLib (will be updated in future shared crypto changes)
  # ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  # MU_CHANGE [END]: Remove ArmLib (will be updated in future shared crypto changes)

  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

!include CryptoPkg/CryptoPkgFeatureFlagPcds.dsc.inc

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06

## MU_CHANGE Remove PCD definitions

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

#
[Components.IA32, Components.X64, Components.AARCH64]

[Components]
  #
  # Build verification of all library instances
  #

  CryptoPkg/Library/HmacSha1Lib/HmacSha1LibNull.inf
  CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
  CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
#  CryptoPkg/Library/OpensslLib/OpensslLibSm3.inf # MU_CHANGE
  CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/RuntimeDxeCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/StandaloneMmCryptLib.inf  # MU_CHANGE: Add StandaloneMmCryptLib
  CryptoPkg/Library/BaseCryptLibOnOneCrypto/DxeCryptLib.inf             # MU_CHANGE: Add OneCrypto
# MU_CHANGE START
[Components.X64, Components.IA32]
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/StandaloneMmCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnOneCrypto/SmmCryptLib.inf # MU_CHANGE: Add OneCrypto
  CryptoPkg/Library/BaseCryptLibOnOneCrypto/StandaloneMmCryptLib.inf # MU_CHANGE: Add OneCrypto


[Components.IA32, Components.X64, Components.AARCH64]
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/BaseCryptLibUnitTestApp.inf {  ## Add unit-test application for the crypto tests.
    ## MU_CHANGE [START] add library classes to allow crypto tests to run in uefi shell correctly
    <LibraryClasses>
      DebugLib|MdePkg/Library/UefiDebugLibDebugPortProtocol/UefiDebugLibDebugPortProtocol.inf # MU_CHANGE add debug lib
      DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf # MU_CHANGE add debug lib
      UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
      ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
      MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xFFFFFFFF
    ## MU_CHANGE [END]
  }
  CryptoPkg/Test/UnitTest/Library/TlsLib/TestTlsLibApp.inf {  ## MU_CHANGE: Add TlsLib unit test application
    <LibraryClasses>
      TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      DebugLib|MdePkg/Library/UefiDebugLibDebugPortProtocol/UefiDebugLibDebugPortProtocol.inf
      DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
      UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
      ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
      MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0xFFFFFFFF
  }
  ## MU_CHANGE [END]
## MU_CHANGE [END]
  #
[Components.IA32, Components.X64]
  # Build verification of IA32/X64 specific libraries
[Components.IA32, Components.X64]
  # CryptoPei with IA32/X64 performance optimized OpensslLib instance without EC services
  # CryptoPei with IA32/X64 performance optimized OpensslLib instance all services
[Components.IA32, Components.X64]
  # CryptoDxe with IA32/X64 performance optimized OpensslLib instance with no EC services
  # CryptoDxe with IA32/X64 performance optimized OpensslLib instance with all services.
  # CryptoSmm with IA32/X64 performance optimized OpensslLib instance with no EC services
  # CryptoSmm with IA32/X64 performance optimized OpensslLib instance with all services

[BuildOptions]
  RELEASE_*_*_CC_FLAGS = -DMDEPKG_NDEBUG
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
!if $(CRYPTO_SERVICES) IN "ALL"
  MSFT:*_*_*_CC_FLAGS = /D ENABLE_MD5_DEPRECATED_INTERFACES
  INTEL:*_*_*_CC_FLAGS = /D ENABLE_MD5_DEPRECATED_INTERFACES
  GCC:*_*_*_CC_FLAGS = -D ENABLE_MD5_DEPRECATED_INTERFACES
!endif

#MU_CHANGE START
[BuildOptions.common.EDKII.DXE_RUNTIME_DRIVER, BuildOptions.common.EDKII.DXE_SMM_DRIVER, BuildOptions.common.EDKII.SMM_CORE, BuildOptions.common.EDKII.DXE_DRIVER]
  MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:4096 # enable 4k alignment for MAT and other protections.
  MSFT:*_*_X64_DLINK_FLAGS = /ALIGN:4096 # enable 4k alignment for MAT and other protections.
#MU_CHANGE END
