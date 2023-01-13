# Crypto Driver

This is a potentially prepacked version of the BaseCryptLib and TlsLib, delivered via protocol.

There are two routes: using the pre-compiled version and compiling it into your platform.

## Benefits

But first, why would you care about this?

It has a few benefits, namely:

- Smaller Binary sizes
- Easier to service/upgrade
- Transparency on what version of crypto you're using
- Reduced build times (if using pre-compiled version)

There are different flavors of Crypto available, with different functions supported.
Don't need to use HMAC in your PEI phase?
Select a service level or flavor that doesn't include HMAC in your platform.

## How include on your platform

Now there are a few options for you. We'll start with the pre-compiled route.

### The Pre-compiled (easy) way

The easy way involves setting a few variables and a few includes.
The hard way is just to do it yourself.

First the easy way:

1. Define the service level that you want for each phase of UEFI in the defines section of your DSC.

    ``` dsc
    [Defines]
        DEFINE PEI_CRYPTO_SERVICES = TINY_SHA
        DEFINE DXE_CRYPTO_SERVICES = STANDARD
        DEFINE SMM_CRYPTO_SERVICES = STANDARD
        DEFINE PEI_CRYPTO_ARCH = IA32
        DEFINE DXE_CRYPTO_ARCH = X64
        DEFINE SMM_CRYPTO_ARCH = X64
    ```

    The above example is for a standard intel platform, and the service levels or flavors available.

2. Add the DSC include

    ``` dsc
    !include CryptoPkg/Driver/Bin/CryptoDriver.inc.dsc
    ```

    This sets the definitions for BaseCryptLib as well as includes the correct flavor level of the component you
    wish to use.

3. Add the FDF includes to your platform FDF

    Currently, it isn't possible in an FDF to redefine a FV section and have them be combined.
    There are two includes: BOOTBLOCK and DXE.
    The first includes the PEI phase and is meant to be stuck in your BOOTBLOCK FV.
    The second contains the DXE and SMM modules and is meant to be stuck in your FVDXE.

    ``` fdf
    [FV.FVBOOTBLOCK]
      ...
    !include CryptoPkg/Driver/Bin/CryptoDriver.BOOTBLOCK.inc.fdf
    ...

    [FV.FVDXE]
      ...
      !include CryptoPkg/Driver/Bin/CryptoDriver.DXE.inc.fdf
    ```

### Recommendations

It is highly recommended to put this logic behind conditionals like so:

``` fdf
[FV.FVBOOTBLOCK]
!if $(ENABLE_SHARED_CRYPTO) == TRUE
  !include CryptoPkg/Driver/Bin/CryptoDriver.BOOTBLOCK.inc.fdf
!endif
```

This allows developers on the platform to use their own BaseCryptLib or TlsLib if they want.
Just add a check if it's not defined in your DSC like so.

``` dsc
!ifndef ENABLE_SHARED_CRYPTO # by default true
  ENABLE_SHARED_CRYPTO = TRUE
!endif
```

### The DIY way

If you want to take advantage of the BaseCryptOnProtocol but don't want to use a pre-compiled method, you can compile
it within your platform itself.

Shown here is for an Intel platform, adjust the architectures as needed.

``` dsc

[LibraryClasses.IA32]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf
  TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf

[LibraryClasses.X64]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf
  TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf

[LibraryClasses.X64.DXE_SMM_DRIVER]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf
  TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf

[Components.IA32]
  CryptoPkg/Driver/CryptoPei.inf {
      <LibraryClasses>
        BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
        TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      <PcdsFixedAtBuild>
        .. All the flavor PCDs here ..
  }

[Components.X64]
  CryptoPkg/Driver/CryptoDxe.inf {
      <LibraryClasses>
        BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
        TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
      <PcdsFixedAtBuild>
        .. All the flavor PCDs here ..
  }
  CryptoPkg/Driver/CryptoSmm.inf {
      <LibraryClasses>
        BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
        TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      <PcdsFixedAtBuild>
        .. All the flavor PCDs here ..
  }
```

The PCDs are long and default to all false.
The flavors are stored as .inc.dsc files at `CryptoPkg\Driver\Packaging`.
An example would be `CryptoPkg\Driver\Packaging\Crypto.pcd.TINY_SHA.inc.dsc` which is a flavor that just has Sha1,
Sha256, and Sha386.

You'll need to include these components in your FDF as well.

``` fdf
[FV.FVBOOTBLOCK]
  INF CryptoPkg/Driver/CryptoPei.inf
[FV.FVDXE]
  INF CryptoPkg/Driver/CryptoSmm.inf
  INF CryptoPkg/Driver/CryptoDxe.inf
```
