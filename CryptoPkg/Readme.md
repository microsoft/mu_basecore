# Crypto Package

This package provides cryptographic services that are used to implement firmware
features such as UEFI Secure Boot, Measured Boot, firmware image authentication,
and network boot. The cryptographic service implementation in this package uses
services from the [OpenSSL](https://www.openssl.org/) project and
[MbedTLS](https://www.trustedfirmware.org/projects/mbed-tls/) project.

EDK II firmware modules/libraries that requires the use of cryptographic
services can either statically link all the required services, or the EDK II
firmware module/library can use a dynamic Protocol/PPI service to call
cryptographic services. The dynamic Protocol/PPI services are only available to
PEIMs, DXE Drivers, UEFI Drivers, and SMM Drivers, and only if the cryptographic
modules are included in the platform firmware image.

There may be firmware image size differences between the static and dynamic
options. Some experimentation may be required to find the solution that
provides the smallest overall firmware overhead.

## Public Library Classes

* **BaseCryptLib**        - Provides library functions based on OpenSSL for
                            cryptographic primitives.
* **BaseCryptLibMbedTls** - Provides library functions based on MbedTLS for
                            cryptographic primitives.
* **TlsLib**              - Provides TLS library functions for EFI TLS protocol.
* **HashApiLib**          - Provides Unified API for different hash implementations.

## Private Library Classes

* **OpensslLib**   - Provides library functions from the openssl project.
* **IntrinsicLib** - Provides C runtime library (CRT) required by openssl
                     and mbedtls.

## Private Protocols and PPIs

* **EDK II Crypto PPI**          - PPI that provides all the services from
                                   the BaseCryptLib and TlsLib library classes.
* **EDK II Crypto Protocol**     - Protocol that provides all the services from
                                   the BaseCryptLib and TlsLib library classes.
* **EDK II SMM Crypto Protocol** - SMM Protocol that provides all the services
                                   from the BaseCryptLib and TlsLib library
                                   classes.

### Statically Linking Cryptographic Services

The figure below shows an example of a firmware module that requires the use of
cryptographic services. The cryptographic services are provided by three library
classes called BaseCryptLib, TlsLib, and HashApiLib. These library classes are
implemented using APIs from the OpenSSL project that are abstracted by the
private library class called OpensslLib. The OpenSSL project implementation
depends on C runtime library services. The EDK II project does not provide a
full C runtime library for firmware components. Instead, the CryptoPkg includes
the smallest subset of services required to build the OpenSSL project in the
private library class called IntrinsicLib.

The CryptoPkg provides several instances of the BaseCryptLib and OpensslLib with
different cryptographic service features and performance optimizations. The
platform developer must select the correct instances based on cryptographic
service requirements in each UEFI/PI firmware phase (SEC, PEI, DXE, UEFI,
UEFI RT, and SMM), firmware image size requirements, and firmware boot
performance requirements.

```text
+================================+
| EDK II Firmware Module/Library |
+================================+
     ^          ^         ^
     |          |         |
     |          |         v
     |          |   +============+
     |          |   | HashApiLib |
     |          |   +============+
     |          |         ^
     |          |         |
     v          v         v
+========+  +====================+
| TlsLib |  |    BaseCryptLib    |
+========+  +====================+
     ^                ^
     |                |
     v                v
+================================+
|     OpensslLib (Private)       |
+================================+
               ^
               |
               v
+================================+
|     IntrinsicLib (Private)     |
+================================+
```

## Dynamically Linking Cryptographic Services

The figure below shows the entire stack when dynamic linking is used with
cryptographic services produced by the CryptoPei, CryptoDxe, or CryptoSmm module
through a PPI/Protocol. This solution requires the CryptoPei, CryptoDxe, and
CryptoSmm modules to be configured with the set of cryptographic services
required by all the PEIMs, DXE Drivers, UEFI Drivers, and SMM Drivers. Dynamic
linking is not available for SEC or UEFI RT modules.

The EDK II modules/libraries that require cryptographic services use the same
BaseCryptLib/TlsLib/HashApiLib APIs. This means no source changes are required
to use static linking or dynamic linking. It is a platform configuration option
to select static linking or dynamic linking. This choice can be made globally,
per firmware module type, or for individual modules.

```text
+===================+    +===================+     +===================+
|    EDK II PEI     |    |  EDK II DXE/UEFI  |     |     EDK II SMM    |
|   Module/Library  |    |   Module/Library  |     |   Module/Library  |
+===================+    +===================+     +===================+
  ^   ^        ^           ^   ^        ^            ^   ^        ^
  |   |        |           |   |        |            |   |        |
  |   |        v           |   |        v            |   |        v
  |   |  +==========+      |   |  +==========+       |   |  +==========+
  |   |  |HashApiLib|      |   |  |HashApiLib|       |   |  |HashApiLib|
  |   |  +==========+      |   |  +==========+       |   |  +==========+
  |   |        ^           |   |        ^            |   |        ^
  |   |        |           |   |        |            |   |        |
  v   v        v           v   v        v            v   v        v
+===================+    +===================+     +===================+
|TlsLib|BaseCryptLib|    |TlsLib|BaseCryptLib|     |TlsLib|BaseCryptLib|
+-------------------+    +-------------------+     +-------------------+
|   BaseCryptLib    |    |   BaseCryptLib    |     |   BaseCryptLib    |
|   OnPpiProtocol/  |    |   OnPpiProtocol/  |     |   OnPpiProtocol/  |
|  PeiCryptLib.inf  |    |   DxeCryptLib.inf |     |  SmmCryptLib.inf  |
+===================+    +===================+     +===================+
           ^                      ^                         ^
          ||| (Dynamic)          ||| (Dynamic)             ||| (Dynamic)
           v                      v                         v
+===================+    +===================+    +=====================+
|     Crypto PPI    |    |  Crypto Protocol  |    | Crypto SMM Protocol |
+-------------------|    |-------------------|    |---------------------|
|     CryptoPei     |    |     CryptoDxe     |    |      CryptoSmm      |
+===================+    +===================+    +=====================+
     ^       ^                ^       ^                 ^       ^
     |       |                |       |                 |       |
     v       |                v       |                 v       |
+========+   |           +========+   |            +========+   |
| TlsLib |   |           | TlsLib |   |            | TlsLib |   |
+========+   v           +========+   v            +========+   v
  ^  +==============+      ^  +==============+       ^  +==============+
  |  | BaseCryptLib |      |  | BaseCryptLib |       |  | BaseCryptLib |
  |  +==============+      |  +==============+       |  +==============+
  |          ^             |          ^              |          ^
  |          |             |          |              |          |
  v          v             v          v              v          v
+===================+    +===================+     +===================+
|    OpensslLib     |    |    OpensslLib     |     |    OpensslLib     |
+===================+    +===================+     +===================+
          ^                        ^                         ^
          |                        |                         |
          v                        v                         v
+===================+    +===================+     +===================+
|    IntrinsicLib   |    |    IntrinsicLib   |     |    IntrinsicLib   |
+===================+    +===================+     +===================+
```

## OneCrypto: Phase-Agnostic Cryptographic Services

OneCrypto provides an alternative approach to cryptographic services using phase-agnostic
binary drivers. Unlike the traditional dynamic linking approach where separate crypto drivers
are needed for each phase (CryptoPei, CryptoDxe, CryptoSmm), OneCrypto uses a single binary
driver that can be loaded and executed across multiple firmware phases.

Platform code uses the **BaseCryptLibOnOneCrypto** library instance, which provides the same
BaseCryptLib and TlsLib APIs but calls the OneCrypto Protocol instead of statically linking
OpenSSL or using phase-specific protocols.

**IMPORTANT: Platform code must NEVER call the OneCrypto Protocol directly.** Always use
BaseCryptLibOnOneCrypto to ensure version safety, type safety, and robust error handling.

### Key Features

* **Phase-Agnostic Binary**: Single crypto driver shared across DXE and Standalone MM phases
* **Version Safety**: Runtime version validation prevents mismatched protocol versions
* **Reduced Duplication**: Eliminates need for multiple phase-specific crypto drivers
* **Consistent API**: Platform code continues using standard BaseCryptLib/TlsLib APIs
* **Mandatory Updates**: When a function is added to BaseCryptLib.h, updating OneCrypto is
  **mandatory** to maintain the versioned protocol contract. See
  `CryptoPkg/Library/BaseCryptLibOnOneCrypto/Readme.md` for detailed instructions on adding
  new cryptographic functions to OneCrypto.

### OneCrypto Architecture

```text
+===================+    +===================+     +=======================+
|    EDK II DXE     |    |  EDK II UEFI      |     |   Standalone MM       |
|   Module/Library  |    |   Module/Library  |     |   Module/Library      |
+===================+    +===================+     +=======================+
  ^   ^        ^           ^   ^        ^            ^   ^           ^
  |   |        |           |   |        |            |   |           |
  |   |        v           |   |        v            |   |           v
  |   |  +==========+      |   |  +==========+       |   |     +==========+
  |   |  |HashApiLib|      |   |  |HashApiLib|       |   |     |HashApiLib|
  |   |  +==========+      |   |  +==========+       |   |     +==========+
  |   |        ^           |   |        ^            |   |           ^
  |   |        |           |   |        |            |   |           |
  v   v        v           v   v        v            v   v           v
+===================+    +===================+     +========================+
|TlsLib|BaseCryptLib|    |TlsLib|BaseCryptLib|     |TlsLib|BaseCryptLib     |
+-------------------+    +-------------------+     +------------------------+
|BaseCryptLibOn     |    |BaseCryptLibOn     |     |BaseCryptLibOn          |
|OneCrypto/         |    |OneCrypto/         |     |OneCrypto/              |
|DxeCryptLib.inf    |    |DxeCryptLib.inf    |     |StandaloneMmCryptLib.inf|
+===================+    +===================+     +========================+
           ^                      ^                         ^
          ||| (Dynamic)          ||| (Dynamic)             ||| (Dynamic)
           v                      v                         v
+===================+    +===================+    +========================+
|  OneCrypto        |    |  OneCrypto        |    |  OneCrypto             |
|  Protocol         |    |  Protocol         |    |  Protocol              |
+-------------------|    |-------------------|    |------------------------|
|OneCryptoLoaderDxe |    |OneCryptoLoaderDxe |    |OneCryptoLoaderSupvMm   |
+===================+    +===================+    +========================+
           ^                      ^                         ^
           |                      |                         |
           +----------------------+-------------------------+
                                  |
                                  v
                    +=============================+
                    |  OneCryptoBinSupvMm.efi     |
                    |  (Phase-Agnostic Binary)    |
                    +=============================+
                    |  Crypto Implementation      |
                    |  (OpenSSL/MbedTLS based)    |
                    +=============================+
```

### OneCrypto Components

* **OneCryptoLoader** - Loads the phase-agnostic binary and publishes the OneCrypto Protocol
  * `OneCryptoLoaderDxe.inf` - DXE phase loader
  * `OneCryptoLoaderSupvMm.inf` - Standalone MM phase loader
* **OneCryptoBin** - Phase-agnostic binary driver containing cryptographic implementations
  * `OneCryptoBinSupvMm.inf` - Single binary used by both DXE and Standalone MM
* **BaseCryptLibOnOneCrypto** - Library instance that wraps the OneCrypto Protocol
  * `DxeCryptLib.inf` - DXE/UEFI driver/application support
  * `StandaloneMmCryptLib.inf` - Standalone MM driver support
  * Provides version validation and type-safe API translation
  * **Required wrapper** - Direct protocol access is unsafe and unsupported

### Adding New Cryptographic Functions

When adding new cryptographic functions to BaseCryptLib.h, updating OneCrypto is **mandatory**
to maintain protocol compatibility. The OneCrypto Protocol is versioned and validated at runtime,
so all implementations must stay synchronized.

For detailed step-by-step instructions on adding new functions, see
[BaseCryptlibOnOneCrypto](./Library/BaseCryptLibOnOneCrypto/README.md).

## Supported Cryptographic Families and Services

The table below provides a summary of the supported cryptographic services. It
indicates if the family or service is deprecated or recommended to not be used.
It also shows which *CryptLib library instances support the family or service.
If a cell is blank then the service or family is always disabled and the
`PcdCryptoServiceFamilyEnable` setting for that family or service is ignored.
If the cell is not blank, then the service or family is configurable using
`PcdCryptoServiceFamilyEnable` as long as the correct OpensslLib or TlsLib is
also configured.

|Key      | Description                                                                    |
|---------|--------------------------------------------------------------------------------|
|  blank  | Family or service is always disabled.                                          |
| C       | Configurable using PcdCryptoServiceFamilyEnable.                               |
| C-Tls   | Configurable using PcdCryptoServiceFamilyEnable. Requires TlsLib.inf.          |
| C-Full  | Configurable using PcdCryptoServiceFamilyEnable. Requires OpensslLibFull*.inf. |

|Family/Service                   | Deprecated | Don't Use | SecCryptLib | PeiCryptLib | BaseCryptLib | SmmCryptLib | RuntimeCryptLib |
|:--------------------------------|:----------:|:---------:|:-----------:|:-----------:|:------------:|:-----------:|:---------------:|
| HmacMd5                         |     Y      |     Y     |             |             |              |             |                 |
| HmacSha1                        |     Y      |     Y     |             |             |              |             |                 |
| HmacSha256                      |     N      |     N     |             |      C      |      C       |      C      |        C        |
| HmacSha384                      |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Md4                             |     Y      |     Y     |             |             |              |             |                 |
| Md5                             |     Y      |     Y     |             |      C      |      C       |      C      |        C        |
| Pkcs.Pkcs1v2Encrypt             |     N      |     N     |             |             |      C       |      C      |                 |
| Pkcs.Pkcs5HashPassword          |     N      |     N     |             |             |      C       |      C      |                 |
| Pkcs.Pkcs7Verify                |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Pkcs.VerifyEKUsInPkcs7Signature |     N      |     N     |             |      C      |      C       |      C      |                 |
| Pkcs.Pkcs7GetSigners            |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Pkcs.Pkcs7FreeSigners           |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Pkcs.Pkcs7Sign                  |     N      |     N     |             |             |      C       |             |                 |
| Pkcs.Pkcs7GetAttachedContent    |     N      |     N     |             |      C      |      C       |      C      |                 |
| Pkcs.Pkcs7GetCertificatesList   |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Pkcs.AuthenticodeVerify         |     N      |     N     |             |             |      C       |             |                 |
| Pkcs.ImageTimestampVerify       |     N      |     N     |             |             |      C       |             |                 |
| Dh                              |     N      |     N     |             |             |      C       |             |                 |
| Random                          |     N      |     N     |             |             |      C       |      C      |        C        |
| Rsa.VerifyPkcs1                 |     Y      |     Y     |             |             |              |             |                 |
| Rsa.New                         |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Rsa.Free                        |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Rsa.SetKey                      |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Rsa.GetKey                      |     N      |     N     |             |             |      C       |             |                 |
| Rsa.GenerateKey                 |     N      |     N     |             |             |      C       |             |                 |
| Rsa.CheckKey                    |     N      |     N     |             |             |      C       |             |                 |
| Rsa.Pkcs1Sign                   |     N      |     N     |             |             |      C       |             |                 |
| Rsa.Pkcs1Verify                 |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Sha1                            |     N      |     Y     |             |      C      |      C       |      C      |        C        |
| Sha256                          |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Sha384                          |     N      |     N     |      C      |      C      |      C       |      C      |        C        |
| Sha512                          |     N      |     N     |      C      |      C      |      C       |      C      |        C        |
| X509                            |     N      |     N     |             |             |      C       |      C      |        C        |
| Tdes                            |     Y      |     Y     |             |             |              |             |                 |
| Aes.GetContextSize              |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Aes.Init                        |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Aes.EcbEncrypt                  |     Y      |     Y     |             |             |              |             |                 |
| Aes.EcbDecrypt                  |     Y      |     Y     |             |             |              |             |                 |
| Aes.CbcEncrypt                  |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Aes.CbcDecrypt                  |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Arc4                            |     Y      |     Y     |             |             |              |             |                 |
| Sm3                             |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Hkdf                            |     N      |     N     |             |      C      |      C       |      C      |        C        |
| Tls                             |     N      |     N     |             |             |    C-Tls     |             |                 |
| TlsSet                          |     N      |     N     |             |             |    C-Tls     |             |                 |
| TlsGet                          |     N      |     N     |             |             |    C-Tls     |             |                 |
| RsaPss.Sign                     |     N      |     N     |             |             |      C       |             |                 |
| RsaPss.Verify                   |     N      |     N     |             |      C      |      C       |      C      |                 |
| ParallelHash                    |     N      |     N     |             |             |              |      C      |                 |
| AeadAesGcm                      |     N      |     N     |             |             |      C       |             |                 |
| Bn                              |     N      |     N     |             |             |      C       |             |                 |
| Ec                              |     N      |     N     |             |             |    C-Full    |             |                 |

## Platform Configuration of Cryptographic Services

Configuring the cryptographic services requires library mappings and PCD
settings in a platform DSC file. This must be done for each of the firmware
phases (SEC, PEI, DXE, UEFI, SMM, UEFI RT).

Platform developers can choose from three approaches for providing cryptographic services:

1. **Static Linking** - Cryptographic implementations (OpenSSL/MbedTLS) are directly compiled
   into each module that needs crypto. Larger per-module size but no runtime dependencies.

2. **Dynamic Linking (Protocol/PPI)** - Cryptographic services are provided by phase-specific
   drivers (CryptoPei, CryptoDxe, CryptoSmm) through protocols. Modules use
   BaseCryptLibOnProtocolPpi to call these services. Reduces per-module size but requires
   separate crypto driver for each phase.

3. **OneCrypto (Phase-Agnostic Binary)** - Cryptographic services are provided by a single
   phase-agnostic binary driver loaded by OneCryptoLoader. Modules use BaseCryptLibOnOneCrypto
   to call the OneCrypto Protocol. Reduces firmware image duplication and provides version
   safety. See "OneCrypto: Phase-Agnostic Cryptographic Services" section above for details.

The following table can be used to help select the best OpensslLib instance for
each phase when using static or dynamic (Protocol/PPI) linking. This table does not apply
to OneCrypto, which uses pre-built binary drivers. The Size column only shows the estimated
size increase for a compressed IA32/X64 module that uses the cryptographic services with
`OpensslLib.inf` as the baseline size. The actual size increase depends on the
specific set of enabled cryptographic services. If ECC services are not
required, then the size can be reduced by using OpensslLib.inf instead of
`OpensslLibFull.inf`. Performance optimization requires a size increase.

| OpensslLib Instance     | SSL | ECC | Perf Opt |      CPU Arch    | Size  |
|:------------------------|:---:|:---:|:--------:|:----------------:|:-----:|
| OpensslLibCrypto.inf    |  N  |  N  |    N     |        All       |   +0K |
| OpensslLib.inf          |  Y  |  N  |    N     |        All       |   +0K |
| OpensslLibAccel.inf     |  Y  |  N  |    Y     | IA32/X64/AARCH64 |  +20K |
| OpensslLibFull.inf      |  Y  |  Y  |    N     |        All       | +115K |
| OpensslLibFullAccel.inf |  Y  |  Y  |    Y     | IA32/X64/AARCH64 | +135K |

### SEC Phase Library Mappings

The SEC Phase only supports static linking of cryptographic services. The
following library mappings are recommended for the SEC Phase. It uses the SEC
specific version of the BaseCryptLib and the null version of the TlsLib because
TLS services are not typically used in SEC.

```text
[LibraryClasses.common.SEC]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SecCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

### PEI Phase Library Mappings

The PEI Phase supports either static or dynamic linking of cryptographic
services. The following library mappings are recommended for the PEI Phase. It
uses the PEI specific version of the BaseCryptLib and the null version of the
TlsLib because TLS services are not typically used in PEI.

```text
[LibraryClasses.common.PEIM]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

If dynamic linking is used, then all PEIMs except CryptoPei use the following
library mappings. The CryptoPei module uses the static linking settings.

```text
[LibraryClasses.common.PEIM]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf

[Components]
  CryptoPkg/Driver/CryptoPei.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  }
```

### DXE Phase, UEFI Driver, UEFI Application Library Mappings

The DXE/UEFI Phase supports either static or dynamic linking of cryptographic
services. The following library mappings are recommended for the DXE/UEFI Phase.
It uses the DXE specific version of the BaseCryptLib and the full version of the
OpensslLib and TlsLib. If ECC services are not required then a smaller
OpensslLib instance can be used.

```text
[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

If dynamic linking is used, then all DXE Drivers except CryptoDxe use the
following library mappings. The CryptoDxe module uses the static linking
settings.

```text
[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf

[Components]
  CryptoPkg/Driver/CryptoDxe.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
      IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  }
```

If OneCrypto is used, then all DXE Drivers use the BaseCryptLibOnOneCrypto library
instance. The OneCrypto binary driver and loader must be included in the platform.

```text
[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnOneCrypto/DxeCryptLib.inf
  TlsLib|CryptoPkg/Library/BaseCryptLibOnOneCrypto/DxeCryptLib.inf

[Components]
  $(ONE_CRYPTO_PATH)/OneCryptoLoaders/OneCryptoLoaderDxe.inf
  $(ONE_CRYPTO_PATH)/OneCryptoBin/OneCryptoBinSupvMm.inf
```

### SMM Phase Library Mappings

The SMM Phase supports either static or dynamic linking of cryptographic
services. The following library mappings are recommended for the SMM Phase. It
uses the SMM specific version of the BaseCryptLib and the null version of the
TlsLib.

```text
[LibraryClasses.common.DXE_SMM_DRIVER]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

If dynamic linking is used, then all SMM Drivers except CryptoSmm use the
following library mappings. The CryptoDxe module uses the static linking
settings.

```text
[LibraryClasses.common.DXE_SMM_DRIVER]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf

[Components]
  CryptoPkg/Driver/CryptoSmm.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  }
```

### Standalone MM Phase Library Mappings

Standalone MM Phase supports either static or dynamic linking of cryptographic
services. For OneCrypto, Standalone MM modules can use the BaseCryptLibOnOneCrypto
library instance with the same shared binary driver used by DXE.

```text
[LibraryClasses.common.MM_STANDALONE]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnOneCrypto/StandaloneMmCryptLib.inf
  TlsLib|CryptoPkg/Library/BaseCryptLibOnOneCrypto/StandaloneMmCryptLib.inf

[Components]
  $(ONE_CRYPTO_PATH)/OneCryptoLoaders/OneCryptoLoaderSupvMm.inf
  $(ONE_CRYPTO_PATH)/OneCryptoBin/OneCryptoBinSupvMm.inf
```

Note: The same OneCryptoBinSupvMm.inf binary is used by both DXE (via OneCryptoLoaderDxe)
and Standalone MM (via OneCryptoLoaderSupvMm), reducing firmware image duplication.

### UEFI Runtime Driver Library Mappings

UEFI Runtime Drivers only support static linking of cryptographic services.
The following library mappings are recommended for UEFI Runtime Drivers. They
use the runtime specific version of the BaseCryptLib and the null version of the
TlsLib because TLS services are not typically used at runtime.

```text
[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
```

### PCD Configuration Settings

There are 2 PCD settings that are used to configure cryptographic services.
`PcdHashApiLibPolicy` is used to configure the hash algorithm provided by the
BaseHashApiLib library instance. `PcdCryptoServiceFamilyEnable` is used to
configure the cryptographic services supported by the CryptoPei, CryptoDxe,
and CryptoSmm modules.

* `gEfiCryptoPkgTokenSpaceGuid.PcdHashApiLibPolicy` - This PCD indicates the
  HASH algorithm to use in the BaseHashApiLib to calculate hash of data. The
  default hashing algorithm for BaseHashApiLib is set to HASH_ALG_SHA256.
  |  Setting   |    Algorithm     |
  |------------|------------------|
  | 0x00000001 | HASH_ALG_SHA1    |
  | 0x00000002 | HASH_ALG_SHA256  |
  | 0x00000004 | HASH_ALG_SHA384  |
  | 0x00000008 | HASH_ALG_SHA512  |
  | 0x00000010 | HASH_ALG_SM3_256 |

* `gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable` - Enable/Disable
   the families and individual services produced by the EDK II Crypto
   Protocols/PPIs. The default is all services disabled. This Structured PCD is
   associated with the `PCD_CRYPTO_SERVICE_FAMILY_ENABLE` structure that is
   defined in `Include/Pcd/PcdCryptoServiceFamilyEnable.h`.

   There are three layers of priority that determine if a specific family or
   individual cryptographic service is actually enabled in the CryptoPei,
   CryptoDxe, and CryptoSmm modules.

   1) OpensslLib instance selection. When the CryptoPei, CryptoDxe, or CryptoSmm
      drivers are built, they are statically linked to an OpensslLib library
      instance. If the required cryptographic service is not enabled in the
      OpensslLib instance linked, then the service is always disabled.
   2) BaseCryptLib instance selection.
      * CryptoPei is always linked with the PeiCryptLib instance of the
        BaseCryptLib library class. The table above has a column for the
        PeiCryptLib. If the family or service is blank, then that family or
        service is always disabled.
      * CryptoDxe is always linked with the BaseCryptLib instance of the
        BaseCryptLib library class. The table above has a column for the
        BaseCryptLib. If the family or service is blank, then that family or
        service is always disabled.
      * CryptoSmm is always linked with the SmmCryptLib instance of the
        BaseCryptLib library class. The table above has a column for the
        SmmCryptLib. If the family or service is blank, then that family or
        service is always disabled.
   3) If a family or service is enabled in the OpensslLib instance and it is
      enabled in the BaseCryptLib instance, then it can be enabled/disabled
      using `PcdCryptoServiceFamilyEnable`. This structured PCD is associated
      with the `PCD_CRYPTO_SERVICE_FAMILY_ENABLE` data structure that contains
      bit fields for each family of services. All of the families are disabled
      by default. An entire family of services can be enabled by setting the
      family field to the value `PCD_CRYPTO_SERVICE_ENABLE_FAMILY`. Individual
      services can be enabled by setting a single service name (bit) to `TRUE`.
      Settings listed later in the DSC file have priority over settings listed
      earlier in the DSC file, so it is valid for an entire family to be enabled
      first and then for a few individual services to be disabled by setting
      those service names to `FALSE`.

#### Common PEI PcdCryptoServiceFamilyEnable Settings

```text
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                    | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family                    | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                          | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha384.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha512.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sm3.Family                           | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Family                           | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Pkcs1Verify             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.New                     | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Free                    | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.SetKey                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs5HashPassword      | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Hkdf.Family                          | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
```

#### Common DXE and SMM PcdCryptoServiceFamilyEnable Settings

```text
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Hkdf.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs1v2Encrypt             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs5HashPassword          | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7Verify                | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.VerifyEKUsInPkcs7Signature | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7GetSigners            | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7FreeSigners           | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.AuthenticodeVerify         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Random.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Pkcs1Verify                 | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.New                         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Free                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.SetKey                      | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.GetPublicKeyFromX509        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Services.HashAll                  | FALSE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetSubjectName             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetCommonName              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetOrganizationName        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetTBSCert                 | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Tls.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsSet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsGet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.GetContextSize              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.Init                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcEncrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcDecrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Services.Encrypt              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Services.Decrypt              | TRUE
```
