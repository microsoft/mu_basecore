r-efi
=====

UEFI Reference Specification Protocol Constants and Definitions

The r-efi project provides the protocol constants and definitions of the
UEFI Reference Specification as native rust code. The scope of this project is
limited to those protocol definitions. The protocols are not actually
implemented. As such, this project serves as base for any UEFI application that
needs to interact with UEFI, or implement (parts of) the UEFI specification.

### Project

 * **Website**: <https://r-util.github.io/r-efi>
 * **Bug Tracker**: <https://github.com/r-util/r-efi/issues>

### Requirements

The requirements for this project are:

 * `rustc >= 1.31.0`

### Build

To build this project, run:

```sh
cargo build
```

Available configuration options are:

 * **examples**: This feature-selector enables compilation of examples. This
                 is disabled by default, since they will only compile
                 successfully on UEFI targets.

No special requirements exist to compile for UEFI targets. Native compilations
work out of the box without any adjustments. In case of cross-compilation, you
need a target-configuration as input to the rust compiler.

Our recommended way to cross-compile this project is to use `cargo-xbuild`. To
setup your toolchain for cross-compilation, you need:

```sh
rustup toolchain install nightly
# OR
rustup update

rustup component add --toolchain nightly rust-src
cargo install --force cargo-xbuild
```

Be sure to update all components to the most recent version. Most of these
depend on features only available in nightly, so you need recent versions.

With these installed, it then becomes as simple as the following command to
build the example applications shipped with this project:

```sh
cargo \
    +nightly \
    xbuild \
    --target x86_64-unknown-uefi \
    --features examples \
    --examples
```

### Repository:

 - **web**:   <https://github.com/r-util/r-efi>
 - **https**: `https://github.com/r-util/r-efi.git`
 - **ssh**:   `git@github.com:r-util/r-efi.git`

### License:

 - **Apache-2.0** OR **LGPL-2.1-or-later**
 - See AUTHORS file for details.
