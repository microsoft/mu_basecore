## @file
# CryptoPkg DSC file used to build host-based unit tests.
#
# Copyright (c) Microsoft Corporation.<BR>
# Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = CryptoPkgHostTest
  PLATFORM_GUID           = C7F97D6D-54AC-45A9-8197-CC99B20CC7EC
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/CryptoPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

# MU_CHANGE - Start - Move to OneCrypto
#!ifndef CRYPTO_TEST_TYPE
#  DEFINE CRYPTO_TEST_TYPE = OPENSSL
#!endif
#
#!if $(CRYPTO_TEST_TYPE) IN "OPENSSL MBEDTLS"
#!else
#  !error CRYPTO_TEST_TYPE must be set to one of OPENSSL MBEDTLS.
#!endif
# MU_CHANGE - End - Move to OneCrypto
!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

!include CryptoPkg/CryptoPkgFeatureFlagPcds.dsc.inc

[LibraryClasses]
# MU_CHANGE - Start - Move to OneCrypto
#  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/UnitTestHostBaseCryptLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
# MU_CHANGE - End - Move to OneCrypto
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.X64, LibraryClasses.IA32]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[Components]
  #
  # Build HOST_APPLICATION that tests the SampleUnitTest
  #
# MU_CHANGE - Start - Move to OneCrypto
#!if $(CRYPTO_TEST_TYPE) IN "OPENSSL"
#  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibHost.inf {
#    <LibraryClasses>
#      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
#
#  }
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibHost.inf
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibHost.inf {
    <Defines>
      FILE_GUID = 3604CCB8-138C-488F-8045-18704F73E734
# MU_CHANGE - Start - Move to OneCrypto
#    <LibraryClasses>
#      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
# MU_CHANGE - End - Move to OneCrypto
  }
!endif

# MU_CHANGE - Start - Move to OneCrypto
#!if $(CRYPTO_TEST_TYPE) IN "MBEDTLS"
#  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibHostMbedTls.inf {
#    <LibraryClasses>
#      BaseCryptLib|CryptoPkg/Library/BaseCryptLibMbedTls/UnitTestHostBaseCryptLib.inf
#      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibSm3.inf
#      MbedTlsLib|CryptoPkg/Library/MbedTlsLib/MbedTlsLib.inf
#  }
!endif
# MU_CHANGE - Start - Move to OneCrypto

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
