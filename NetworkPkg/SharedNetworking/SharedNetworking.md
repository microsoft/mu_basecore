# Shared Networking DXE

## What is it

Shared Networking is a packaged versions of networking components from EDK II.
Similar to SharedCrypto (see the SharedCryptoPkg), it precompiles certain
components and allows them to be included in a platform without having to build
the underlying library.

## How it works

Since many parts of the network simply publish a protocol (like TlsDxe), it was
fairly trivial to compile that into an EFI. This EFI is then downloaded via a
NuGet External Dependency (see SharedNetworking_ext_dep.json). Versions are
modified in a similar way to SharedCrypto.

### Versioning

A typical version consists of 4 numbers. The year, the month of the EDK II
release, the revision number, and the build number. An example of this would be
_2019.03.02.01_, which would translate to EDK II 1903 release, the second
revision and the first build. This means that there were two code changes within
1903 (either in BaseCryptLib or OpenSSL). Release notes will be provided on the
NuGet package page and on this repo. Build numbers are reved whenever there
needs to be a recompiled binary due to a mistake on our part or a build flag is
tweaked.

## How to use it

There are two ways to use SharedNetworking. For first way is to use the FV,
which contains all the networking components needed. The second is to replace
individual components with INF's.

## DSC/INF way

Including it in your platform is easy peezy lemon squeezy. In fact, you only
need three changes. In the example below we show X64, which happens to
correspond with DXE but that could easily be changed. Look at your platform for
where Networking is already defined. One thing to note is that each binary is
released for two targets, RELEASE and DEBUG. Make sure to include the right INF.

### DSC Changes

Parts need to be replaced on a compoenent by component basis. For example, here
is how to move over TlsDxe. You need to remove the reference to TLSLib since we
no longer need it (the only consumer is TlsDxe). Then switch the component to
the Shared version of TLS. It looks like this:

```edk2_dsc
[LibraryClasses.X64]
  #TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf # remove this line

[Components.X64]
  NetworkPkg/SharedNetworking/TlsDxe.$(TARGET).inf
```

### FDF Changes

```edk2_fdf

[FV.FVDXE]
 INF  NetworkPkg/SharedNetworking/TlsDxe.$(TARGET).inf # Shared_TLS instead of TlsDxe
 ...
```

## FV way

This way is still under development, so it maybe subject to change. In your FDF,
add these lines.

```edk2_fdf
[FV.FVDXE]

FILE FV_IMAGE = {GUID} {
  SECTION FV_IMAGE = NetworkPkg/SharedNetworking/Mu-SharedNetworking_extdep/$(TARGET)/{ARCH of your platform}/FVDXE.fv # Shared_Networking
  SECTION UI = "SharedNetworking"
}
```

With {GUID} being a guid you generated. We use
E205F779-07E3-4B64-A2E2-EEDE717B0F59. {Arch of your platform} being the platform
you're using. We currently support IA32, X64, and AARCH64. as supposered values
You'll also need to remove the networking components that were already in your
FDF.

## Why to Use SharedNetworking

Depending on your platform, it could net you some small space savings depending
on your linker. The main advantage is that when used with SharedCrypto, you can
remove the need to compile OpenSSL, reducing compile times.

## Questions

If you have any questions about anything in this package or the universe in
general, feel free to comment on our Github or contact the Project Mu team.
