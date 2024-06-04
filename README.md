# Mu Rust Platform Initialization (PI)

Platform Initialization (PI) Specification definitions and support code in rust.

This repository is part of [Project Mu](https://microsoft.github.io/mu).

## Requirements

- rustc >= 1.68.0

## Build

```sh
cargo build
```

### Build with Official Toolchains

These instructions are derived from those in [r-efi](https://github.com/r-efi/r-efi/blob/main/README.md).

Starting with rust-version 1.68, rustup distributes pre-compiled toolchains for many UEFI targets. You can enumerate
and install them via `rustup`. This example shows how to enumerate all available targets for your stable toolchain
and then install the UEFI target for the `x86_64` architecture:

```sh
rustup target list --toolchain=stable
rustup target add --toolchain=stable x86_64-unknown-uefi
```

This project can then be compiled directly for the selected target:

```sh
cargo +stable build --lib --target x86_64-unknown-uefi
```

### Build via Foreign Targets

The project can be built for non-UEFI targets via the standard rust toolchains. This allows non-UEFI targets to
interact with UEFI systems or otherwise host UEFI operations. Furthermore, this allows running the foreign test-suite
of this project as long as the target supports the full standard library:

```sh
cargo +stable build --all-targets
cargo +stable test --all-targets
```

## Test

```sh
cargo test
```

## Contributing

Contributions are always welcome and encouraged!

Please run the following commands before creating a pull request:

- \>`cargo fmt`
- \>`cargo test --all`
  - Verify tests pass.
- \>`cargo doc --open`
  - Verify documentation appearance.

Guidance and requirements:

- [Contribution Guidance](https://microsoft.github.io/mu/How/contributing/).
- [Code Requirements](https://microsoft.github.io/mu/CodeDevelopment/requirements/)
- [Doc Requirements](https://microsoft.github.io/mu/CodeDevelopment/rust_documentation_conventions/)

## Issues

Please open any issues in the [issues section](https://github.com/microsoft/mu_rust_hid/issues).

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).

For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com). with any additional questions or comments.

## Copyright & License

- Copyright (c) Microsoft Corporation
- SPDX-License-Identifier: BSD-2-Clause-Patent
