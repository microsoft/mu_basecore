# SharedNetworking

Similar to SharedCrypto
(<https://github.com/microsoft/mu_plus/tree/release/201911/SharedCryptoPkg>),
SharedNetworking is a collection of pre-built network binaries you can include
in your platform or other EDK2 project.

SharedNetworking requires SharedCrypto for BaseCryptLib functionality (this only
applies to TlsLib and IScsiDxe)

The build script for this (SharedNetworkSettings.py) pulls in MU_PLUS as it has
a dependency on SharedCrypto (which currently resides in MU_PLUS). This is
temporary and will not carry forward to 202002. It also doesn't apply to the
remainder of Basecore or CI. The dependency is only pulled in when build
SharedNetworking itself, which doesn't happen often.

## Advantages

- Faster Compile Times
- Potentially smaller binary sizes (depending on compression and a variety of
  other factors)
- Easier to update and service since network binaries are packaged in an FV.

## Including it in your project

Just !include the SharedNetworking.fdf.inc as the example below shows:

```edk2_fdf
[FV.FVDXE]
  ...
  !include NetworkPkg/SharedNetworking.fdf.inc
```
