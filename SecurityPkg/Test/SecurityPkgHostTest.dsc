## @file
# SecurityPkg DSC file used to build host-based unit tests.
#
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = SecurityPkgHostTest
  PLATFORM_GUID           = 9D78A9B4-00CD-477E-A5BF-90CC793EEFB0
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/SecurityPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64|AARCH64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.AARCH64]
  RngLib|MdePkg/Library/BaseRngLibNull/BaseRngLibNull.inf

[Components]
  # MU_CHANGE [BEGIN] - ECIT host tests.
  SecurityPkg/CryptoIndicatorTable/GoogleTest/CryptoIndicatorTableDxeGoogleTest.inf {
    <LibraryClasses>
      HobLib|MdePkg/Test/Mock/Library/GoogleTest/MockHobLib/MockHobLib.inf
      UefiBootServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiBootServicesTableLib/MockUefiBootServicesTableLib.inf
      UefiLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.inf
    <PcdsFixedAtBuild>
      gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemId|{0x4D, 0x53, 0x46, 0x54, 0x20, 0x20}
      gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemTableId|0x545345544943454D
      gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemRevision|1
      gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorId|0x5453464D
      gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultCreatorRevision|1
  }
  SecurityPkg/Library/CryptoIndicatorRegistrationLib/GoogleTest/CryptoIndicatorRegistrationLibPeiGoogleTest.inf {
    <LibraryClasses>
      CryptoIndicatorRegistrationLib|SecurityPkg/Library/CryptoIndicatorRegistrationLib/CryptoIndicatorRegistrationLibPei.inf
      HobLib|MdePkg/Test/Mock/Library/GoogleTest/MockHobLib/MockHobLib.inf
  }
  SecurityPkg/Library/EcitEncodingLib/UnitTest/EcitEncodingLibUnitTest.inf {
    <LibraryClasses>
      EcitEncodingLib|SecurityPkg/Library/EcitEncodingLib/EcitEncodingLib.inf
  }
  SecurityPkg/Library/CryptoIndicatorRegistrationLib/GoogleTest/CryptoIndicatorRegistrationLibStandaloneMmGoogleTest.inf {
    <LibraryClasses>
      MmServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockMmServicesTableLib/MockMmServicesTableLib.inf
  }
  # MU_CHANGE [END]

  SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiRuntimeServicesTableLib.inf
  SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockPlatformPKProtectionLib.inf
  SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiLib.inf
  SecurityPkg/Test/Mock/Library/GoogleTest/MockPlatformPKProtectionLib/MockPlatformPKProtectionLib.inf
  SecurityPkg/Library/DxeTpm2MeasureBootLib/InternalUnitTest/DxeTpm2MeasureBootLibSanitizationTestHost.inf
  SecurityPkg/Library/DxeTpmMeasureBootLib/InternalUnitTest/DxeTpmMeasureBootLibSanitizationTestHost.inf
  SecurityPkg/Test/Mock/Library/GoogleTest/MockSecureBootVariableLib/MockSecureBootVariableLib.inf

  #
  # Build SecurityPkg HOST_APPLICATION Tests
  #
  SecurityPkg/Library/SecureBootVariableLib/UnitTest/SecureBootVariableLibUnitTest.inf {
    <LibraryClasses>
      SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
      UefiRuntimeServicesTableLib|SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiRuntimeServicesTableLib.inf
      PlatformPKProtectionLib|SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockPlatformPKProtectionLib.inf
      UefiLib|SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiLib.inf
  }
  SecurityPkg/Library/SecureBootVariableLib/GoogleTest/SecureBootVariableLibGoogleTest.inf {
    <LibraryClasses>
      SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
      UefiRuntimeServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
      PlatformPKProtectionLib|SecurityPkg/Test/Mock/Library/GoogleTest/MockPlatformPKProtectionLib/MockPlatformPKProtectionLib.inf
      UefiLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.inf
  }

  # MU_CHANGE [BEGIN]
  SecurityPkg/Library/BaseAuthenticodeLib/GoogleTest/AuthenticodeLibGoogleTest.inf {
    <LibraryClasses>
      AuthenticodeLib|SecurityPkg/Library/BaseAuthenticodeLib/BaseAuthenticodeLib.inf
      PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
      PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  }

  SecurityPkg/Library/BaseAuthenticodeLib/GoogleTest/AuthenticodeLibFaultGoogleTest.inf {
    <LibraryClasses>
      MemoryAllocationLib|MdePkg/Test/Mock/Library/GoogleTest/MockMemoryAllocationLib/MockMemoryAllocationLib.inf
      PeCoffLib|MdePkg/Test/Mock/Library/GoogleTest/MockPeCoffLib/MockPeCoffLib.inf
      SafeIntLib|MdePkg/Test/Mock/Library/GoogleTest/MockSafeIntLib/MockSafeIntLib.inf
  }
  # MU_CHANGE [END]

  SecurityPkg/Library/ImageSecureBootVerificationResultTableLib/GoogleTest/ImageSecureBootVerificationResultTableLibGoogleTest.inf {
    <LibraryClasses>
      ImageSecureBootVerificationResultTableLib|SecurityPkg/Library/ImageSecureBootVerificationResultTableLib/ImageSecureBootVerificationResultTableLib.inf
  }

  SecurityPkg/Library/AuthVariableLib/GoogleTest/AuthVariableLibGoogleTest.inf {
    <LibraryClasses>
      AuthVariableLib|SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
      BaseCryptLib|CryptoPkg/Test/Mock/Library/GoogleTest/MockBaseCryptLib/MockBaseCryptLib.inf
      PlatformSecureLib|SecurityPkg/Library/PlatformSecureLibNull/PlatformSecureLibNull.inf
      VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
        SecurityLockAuditLib|MdeModulePkg/Library/SecurityLockAuditLibNull/SecurityLockAuditLibNull.inf
  }

[PcdsPatchableInModule]
  gEfiSecurityPkgTokenSpaceGuid.PcdOptionRomImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdRemovableMediaImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdFixedMediaImageVerificationPolicy|0x04
