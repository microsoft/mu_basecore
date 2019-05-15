# Shared TLS DXE

## What is it

Shared TLS DXE is a packaged version of TLS DXE. Similar to SharedCrypto (see the SharedCryptoPkg), it precompiles certain components and allows them to be included in a platform without having to build the underlying library.

## How it works

Since TLSDxe publishes a protocol, it was fairly trivial to compile that into an EFI. This EFI is then downloaded via a NuGet External Dependency (see SharedTls_ext_dep.json). Versions are modified in a similar way to SharedCrypto.

### Versioning
A typical version consists of 4 numbers. The year, the month of the EDK II release, the revision number, and the build number. An example of this would be _2019.03.02.01_, which would translate to EDK II 1903 release, the second revision and the first build.
This means that there were two code changes within 1903 (either in BaseCryptLib or OpenSSL).
Release notes will be provided on the NuGet package page and on this repo. Build numbers are reved whenever there needs to be a recompiled binary due to a mistake on our part or a build flag is tweaked.

## How to use it

Including it in your platform is easy peezy lemon squeezy. In fact, you only need three changes.
In the example below we show X64, which happens to correspond with DXE but that could easily be changed.
Look at your platform for where TLS is already defined.
One thing to note is that TLS is release for two targets, RELEASE and DEBUG. Make sure to include the right INF.

### DSC Changes

You need to remove the reference to TLSLib since we no longer need it (the only consumer is TlsDxe). Then switch the component to SharedTLS. It looks like this:

```
[LibraryClasses.X64]
  #TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf # remove this line

[Components.X64]
  NetworkPkg/SharedTlsDxe/TlsDxe.$(TARGET).inf
```

### FDF Changes

```
[FV.FVDXE]
 INF  NetworkPkg/SharedTlsDxe/TlsDxe.$(TARGET).inf # Shared_TLS instead of TlsDxe
 ...
```

## Why to Use SharedTls

Depending on your platform, it could net you some small space savings depending on your linker. The main advantage is that when used with SharedCrypto, you can remove the need to compile OpenSSL, reducing compile times.

## Questions

If you have any questions about anything in this package or the universe in general, feel free to comment on our Github or contact the Project Mu team.
