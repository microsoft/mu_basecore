## @file
#  Cryptographic Library Package for UEFI Security Implementation.
#  PEIM, DXE Driver, and SMM Driver with all crypto services enabled.
#
#  Copyright (c) 2009 - 2021, Intel Corporation. All rights reserved.<BR>
#  Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
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
  SUPPORTED_ARCHITECTURES        = IA32|X64|ARM|AARCH64|RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

  #
  # Flavor of PEI, DXE, SMM modules to build.
  # Must be one of ALL, NONE, MIN_PEI, MIN_DXE_MIN_SMM, TARGET_UINT_TESTS.
  # Default is ALL that is used for package build verification.
  #   ALL             - Build PEIM, DXE, and SMM drivers.  Protocols and PPIs
  #                     publish all services.
  #   NONE            - Build PEIM, DXE, and SMM drivers.  Protocols and PPIs
  #                     publish no services.  Used to verify compiler/linker
  #                     optimizations are working correctly.
  #   MIN_PEI         - Build PEIM with PPI that publishes minimum required
  #                     services.
  #   MIN_DXE_MIN_SMM - Build DXE and SMM drivers with Protocols that publish
  #                     minimum required services.
  #   TARGET_UNIT_TESTS - Build target-based unit tests
  #
  DEFINE CRYPTO_SERVICES = ALL
!if $(CRYPTO_SERVICES) IN "ALL NONE MIN_PEI MIN_DXE_MIN_SMM TARGET_UNIT_TESTS"
!else
  !error CRYPTO_SERVICES must be set to one of ALL NONE MIN_PEI MIN_DXE_MIN_SMM TARGET_UNIT_TESTS.
!endif

#
# Define different OUTPUT_DIRECTORY for each CRYPTO_SERVICES profile
#
!if $(CRYPTO_SERVICES) == ALL
  OUTPUT_DIRECTORY             = Build/CryptoPkg/All
!endif
!if $(CRYPTO_SERVICES) == NONE
  OUTPUT_DIRECTORY              = Build/CryptoPkg/None
!endif
!if $(CRYPTO_SERVICES) == MIN_PEI
  OUTPUT_DIRECTORY              = Build/CryptoPkg/MinPei
!endif
!if $(CRYPTO_SERVICES) == MIN_DXE_MIN_SMM
  OUTPUT_DIRECTORY              = Build/CryptoPkg/MinDxeMinSmm
!endif
!if $(CRYPTO_SERVICES) == TARGET_UNIT_TESTS
  OUTPUT_DIRECTORY              = Build/CryptoPkg/TagetUnitTests
!endif

#
# Define FILE_GUID names/values for CryptoPei, CryptopDxe, and CryptoSmm
# drivers that are linked with different OpensslLib instances
#
  DEFINE  PEI_CRYPTO_GUID     = C693A250-6B36-49B9-B7F3-7283F8136A72
  DEFINE  PEI_STD_GUID        = EBD49F5C-6D8B-40D1-A56D-9AFA485A8661
  DEFINE  PEI_FULL_GUID       = D51FCE59-6860-49C0-9B35-984470735D17
  DEFINE  PEI_STD_ACCEL_GUID  = DCC9CB49-7BE2-47C6-864E-6DCC932360F9
  DEFINE  PEI_FULL_ACCEL_GUID = A10827AD-7598-4955-B661-52EE2B62B057
  DEFINE  DXE_CRYPTO_GUID     = 31C17C54-325D-47D5-8622-888098F10E44
  DEFINE  DXE_STD_GUID        = ADD6D05A-52A2-437B-98E7-DBFDA89352CD
  DEFINE  DXE_FULL_GUID       = AA83B296-F6EA-447F-B013-E80E98629CF8
  DEFINE  DXE_STD_ACCEL_GUID  = 9FBDAD27-910C-4229-9EFF-A93BB5FE18C6
  DEFINE  DXE_FULL_ACCEL_GUID = 41A491D1-A972-468B-A299-DABF415A43B7
  DEFINE  SMM_CRYPTO_GUID     = 1A1C9E13-5722-4636-AB73-31328EDE8BAF
  DEFINE  SMM_STD_GUID        = E4D7D1E3-E886-4412-A442-EFD6F2502DD3
  DEFINE  SMM_FULL_GUID       = 1930CE7E-6598-48ED-8AB1-EBE7E85EC254
  DEFINE  SMM_STD_ACCEL_GUID  = 828959D3-CEA6-4B79-B1FC-5AFA0D7F2144
  DEFINE  SMM_FULL_ACCEL_GUID = C1760694-AB3A-4532-8C6D-52D8F86EB1AA

!if $(CRYPTO_SERVICES) == TARGET_UNIT_TESTS
!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc
!endif

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
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf

##MSCHANGE Begin
  FltUsedLib|MdePkg/Library/FltUsedLib/FltUsedLib.inf
  BaseBinSecurityLibRng|MdePkg/Library/BaseBinSecurityLibNull/BaseBinSecurityLibNull.inf
  UnitTestLib|UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLib.inf
  UnitTestPersistenceLib|UnitTestFrameworkPkg/Library/UnitTestPersistenceLibNull/UnitTestPersistenceLibNull.inf
  UnitTestBootLib|UnitTestFrameworkPkg/Library/UnitTestBootLibNull/UnitTestBootLibNull.inf
  UnitTestResultReportLib|UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibDebugLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
##MSCHANGE End
[LibraryClasses.IA32, LibraryClasses.X64, LibraryClasses.AARCH64]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.AARCH64.DXE_DRIVER, LibraryClasses.ARM.DXE_DRIVER, LibraryClasses.AARCH64.UEFI_APPLICATION, LibraryClasses.ARM.UEFI_APPLICATION]
  RngLib|SecurityPkg/RandomNumberGenerator/RngDxeLib/RngDxeLib.inf

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  RngLib|MdePkg/Library/DxeRngLib/DxeRngLib.inf

!if $(TOOLCHAIN) == VS2017 or $(TOOLCHAIN) == VS2019 or $(TOOLCHAIN) == VS2022 ## MU_CHANGE
[LibraryClasses.IA32]
  NULL|MdePkg/Library/VsIntrinsicLib/VsIntrinsicLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
[LibraryClasses.X64, LibraryClasses.IA32]
  NULL|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
  BaseBinSecurityLib|MdePkg/Library/BaseBinSecurityLibRng/BaseBinSecurityLibRng.inf
[LibraryClasses.X64.DXE_CORE, LibraryClasses.X64.UEFI_DRIVER, LibraryClasses.X64.DXE_DRIVER, LibraryClasses.X64.UEFI_APPLICATION]
  # this is currently X64 only because MSVC doesn't support BaseMemoryLibOptDxe for AARCH64
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
!endif ##MSCHANGE
# MU_CHANGE [END]

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  #
  # It is not possible to prevent the ARM compiler for generic intrinsic functions.
  # This library provides the instrinsic functions generate by a given compiler.
  # [LibraryClasses.ARM, LibraryClasses.AARCH64] and NULL mean link this library
  # into all ARM and AARCH64 images.
  #
  #NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf
  NULL|MdePkg/Library/CompilerIntrinsicsLib/ArmCompilerIntrinsicsLib.inf

  # Add support for stack protector
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf


[LibraryClasses.ARM]
  ArmSoftFloatLib|ArmPkg/Library/ArmSoftFloatLib/ArmSoftFloatLib.inf
[LibraryClasses.common.SEC]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SecCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf

[LibraryClasses.IA32.PEIM, LibraryClasses.X64.PEIM]
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf


[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_APPLICATION] # MU_CHANGE add UEFI Application for UEFI TEsts
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibDebugPortProtocol/UefiDebugLibDebugPortProtocol.inf # MU_CHANGE add debug lib

[LibraryClasses.common.DXE_SMM_DRIVER]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf # MU_CHANGE add debug lib
[LibraryClasses.common.UEFI_APPLICATION]
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
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
# If profile is TARGET_UNIT_TESTS, then build target-based unit tests
# using the OpensslLib, BaseCryptLib, and TlsLib with the largest set of
# available services.
#
!if $(CRYPTO_SERVICES) == TARGET_UNIT_TESTS
[Components.IA32, Components.X64, Components.ARM, Components.AARCH64]
  #
  # Target based unit tests
  #
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
    <BuildOptions>
      MSFT:*_*_*_DLINK_FLAGS     = /ALIGN:4096 /FILEALIGN:4096 /SUBSYSTEM:CONSOLE
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:NOOPT_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
  }

[Components.IA32, Components.X64]
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {
    <Defines>
      FILE_GUID = B91B9A95-4D52-4501-A98F-A1711C14ED93
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
    <BuildOptions>
      MSFT:*_*_*_DLINK_FLAGS     = /ALIGN:4096 /FILEALIGN:4096 /SUBSYSTEM:CONSOLE
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:NOOPT_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
  }

[Components.RISCV64]
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  }
!endif

#
# If profile is ALL, then do verification build of all library instances.
#
!if $(CRYPTO_SERVICES) == ALL
[Components]
  #
  # Build verification of all library instances
  #
  CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/SecCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/BaseCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/SecCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/PeiCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/SmmCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/RuntimeCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/TestBaseCryptLib.inf
  CryptoPkg/Library/MbedTlsLib/MbedTlsLib.inf
  CryptoPkg/Library/MbedTlsLib/MbedTlsLibFull.inf
  CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
  CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  CryptoPkg/Library/TlsLib/TlsLib.inf
  CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  CryptoPkg/Library/OpensslLib/OpensslLib.inf
  CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf

  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf
  # MU_CHANGE [BEGIN] The prebuilt versions of CryptoDriver
!if $(CRYPTO_BINARY_EXTDEP_PATH) != FALSE
  !include CryptoPkg/Driver/Bin/CryptoPkg.ci.inc.dsc
!endif
  # MU_CHANGE [END] The prebuilt versions of CryptoDriver

# MU_CHANGE START
[Components.X64, Components.IA32]
  CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf
  #
  # Build verification of target-based unit tests
  #
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {
    <LibraryClasses>
      UnitTestLib|UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLib.inf
      UnitTestPersistenceLib|UnitTestFrameworkPkg/Library/UnitTestPersistenceLibNull/UnitTestPersistenceLibNull.inf
      UnitTestResultReportLib|UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibConOut.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  }

[Components.IA32, Components.X64]
  #
  # Build verification of IA32/X64 specific libraries
  #
  CryptoPkg/Library/OpensslLib/OpensslLibAccel.inf
  CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
!endif

[Components.IA32, Components.X64] # MU_CHANGE remove ARM and AARCH64
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_CRYPTO_DRIVER_FILE_GUID)  # MU_CHANGE updated File GUID
  }
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
!endif

[Components.IA32, Components.X64, Components.AARCH64]
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_CRYPTO_DRIVER_FILE_GUID)  # MU_CHANGE updated File GUID
  }

[Components.IA32, Components.X64]
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_CRYPTO_DRIVER_FILE_GUID)# MU_CHANGE updated File GUID
  }
## MU_CHANGE TCBZ_3799 - can't compile for ARM as it depends on ArmSoftFloatLib
[Components.IA32, Components.X64, Components.AARCH64]
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {  ## Add unit-test application for the crypto tests.
    ## MU_CHANGE [START] add library classes to allow crypto tests to run in uefi shell correctly
    <LibraryClasses>
      DebugLib|MdePkg/Library/UefiDebugLibDebugPortProtocol/UefiDebugLibDebugPortProtocol.inf # MU_CHANGE add debug lib
      DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf # MU_CHANGE add debug lib
      UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
      ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
      MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
    <PcdsFixedAtBuild>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }
  ## MU_CHANGE [END]
## MU_CHANGE [END]

[BuildOptions]
  RELEASE_*_*_CC_FLAGS = -DMDEPKG_NDEBUG
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
!if $(CRYPTO_SERVICES) IN "PACKAGE ALL"
  MSFT:*_*_*_CC_FLAGS = /D ENABLE_MD5_DEPRECATED_INTERFACES
  INTEL:*_*_*_CC_FLAGS = /D ENABLE_MD5_DEPRECATED_INTERFACES
  GCC:*_*_*_CC_FLAGS = -D ENABLE_MD5_DEPRECATED_INTERFACES

!endif
