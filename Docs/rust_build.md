# Rust Build

Mu Basecore has integrated support to build Rust modules in the normal firmware build process.

[Cargo make](https://crates.io/crates/cargo-make/) is used as a build runner to allow users to build Rust packages
individually and simply on the command line similar to the way they are built during the firmware build process. It
is a CLI tool that helps abstract away many of the CLI arguments necessary to build a Rust module so that developers
can easily check, test, and build individual Rust packages without the need to copy and/or memorize a long list of
arguments.

Based on changes in: https://github.com/tianocore/edk2-staging/tree/edkii-rust

## Generally Getting Started with Rust

It is recommended to:

1. Update `mu_basecore` to ensure it includes the changes needed for Rust build support (as of this file being added).

2. Download and install `Rust` and `cargo` from [Getting Started - Rust Programming Language (rust-lang.org)](https://www.rust-lang.org/learn/get-started)

3. Verify `cargo` is working:

   \>`cargo --version`

4. Install the desired Rust tool chain

   - Example: `1.68.2 x86_64 toolchain`

   - Windows:

      \>`rustup toolchain install 1.68.2-x86_64-pc-windows-msvc`

      \>`rustup component add rust-src --toolchain 1.68.2-x86_64-pc-windows-msvc`

   - Linux:

      \>`rustup toolchain install 1.68.2-x86_64-unknown-linux-gnu`

      \>`rustup component add rust-src --toolchain 1.68.2-x86_64-unknown-linux-gnu`

5. Install `cargo make`

   \>`cargo install --force cargo-make`

At this point, the essential Rust applications are installed, and a repo can begin to add and build Rust code.

## Important Build and Config Files

Currently, two files are provided in Mu Basecore that play an important role in the building and formatting of Rust
code in Project Mu based repositories.

- `Makefile.toml` - Defines the tasks and environment settings use within the Project Mu build system to buid and
  test code.

- `rustfmt.toml` - Defines Rust formatting options used.

Projects are recommended to "extend" `Makefile.toml` by following the instructions in the "Default Tasks and Extending"
section of the [cargo make documentation](https://sagiegurari.github.io/cargo-make/). Note that the file path to the
Mu Basecore file will be relative to the project's own makefile.

The `rustfmt.toml` file is directly provided for use in Mu Basecore and to define the formatting options expected for
Rust code in other Project Mu based repositories.

## Build Variations

First, the simplest case is a new user approaching a package that supports Rust modules. In that case, just build the
package like "normal" (e.g., stuart build) and the "normal" build output (i.e., EFI drivers and firmware volumes) will
be produced.

### Build an Individual Module

  ```cmd
  cargo make build <Module Name>
  ```

The following command line options are available:

1. `-p PROFILE [development|release]`. `DEFAULT` = `development (debug)`
2. `-e ARCH=[IA32|X64|LOCAL]`. `DEFAULT` = `X64`
3. `-e TARGET_TRIPLE=[triple]`.

- `ARCH=LOCAL` is used to build any locally executable tools associated with a Rust library package (e.g., a
  dual-purpose executable and library).

- `TARGET_TRIPLE=<triple>` is used to cross-compile locally executable tools associated with a Rust library package.

- Note: A Rust package must be specified.
- The output location is:
  - `target/[x86_64-unknown-uefi|i686-unknown-uefi]/[debug|release]/module_name.[efi|rlib]`

### Test an Individual Module

```cmd
cargo make test <Optional: Module Name>
```

- Note: If a package is not specified, all packages will be tested.

### Supported Build Combinations

1. C source + Rust source mixed in INF (Library or Module)
   - Rust source code is supported by build rule – `Rust-To-Lib-File` (`.rs` => `.lib`)

   - Limitation: Rust code cannot have external dependencies.

2. Pure Rust Module only.

   - A `Cargo.toml` file is added to the INF file `[Sources]` section.

   - Rust Module build is supported by build rule – `Toml-File.RUST_MODULE` (`Toml` => `.efi`)

   - Limitation: Runtime might be a problem, such as virtual address translation for Rust code internal global
     variables.

3. Pure Rust Module + Pure Rust Library with Cargo Dependency.

   - Cargo dependency means the Rust lib dependency declared in `Cargo.toml`.

4. Pure Rust Module + C Library with EDK II Dependency.

   - Rust Module build is supported by build rule – `Toml-File` (`Toml` => `.lib`)

   - The EDK II dependency means the EDK II lib dependency declared in INF.

     - If a Rust module is built with C, the cargo must use `staticlib`. Otherwise, `rlib` is used.
     - A simple example that specifies `staticlib` in the package `Cargo.toml` file using `crate-type = ["staticlib"]`
       in the `[lib]` section is shown below.

       ```toml
       [lib]
       crate-type = ["staticlib"]
       ```

5. C Module + Pure Rust Library with EDK II Dependency.

   - Rust library build is supported by build rule – `Toml-File`. (`Toml` => `.lib`)

6. Pure Rust Module + Pure Rust Library with EDK II Dependency.
   - Same as (4) + (5).
