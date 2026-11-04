## @file
# Standalone host test platform for OneCrypto capability forwarding.
#
# Copyright (C) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME           = OneCryptoCapabilityTest
  PLATFORM_GUID           = 4ed5b61a-73d8-4500-9c46-9e35048d3347
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/OneCryptoCapabilityTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[Components]
  CryptoPkg/Test/UnitTest/Library/BaseCryptLibOnOneCrypto/CapabilityTest.inf
