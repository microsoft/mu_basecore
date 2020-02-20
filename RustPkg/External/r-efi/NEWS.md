# r-efi - UEFI Reference Specification Protocol Constants and Definitions

## CHANGES WITH 2.1.0:

        * Add the graphics-output-protocol.

        * Expose reserved fields in open structures, otherwise they cannot be
          instantiated from outside the crate itself.

        Contributions from: David Herrmann, Richard Wiedenhöft, Rob Bradford

        - Tübingen, 2019-03-20

## CHANGES WITH 2.0.0:

        * Add a set of UEFI protocols, including simple-text-input,
          file-protocol, simple-file-system, device-path, and more.

        * Fix signature of `BootServices::allocate_pages`.

        Contributions from: David Rheinsberg, Richard Wiedenhöft, Tom Gundersen

        - Tübingen, 2019-03-01

## CHANGES WITH 1.0.0:

        * Enhance the basic UEFI type integration with the rust ecosystem. Add
          `Debug`, `Eq`, `Ord`, ... derivations, provide converters to/from the
          core library, and document the internal workings.

        * Fix `Boolean` to use `newtype(u8)` to make it ABI compatible to UEFI.
          This now accepts any byte value that UEFI accetps without any
          conversion required.

        Contributions from: Boris-Chengbiao Zhou, David Rheinsberg, Tom
                            Gundersen

        - Tübingen, 2019-02-14

## CHANGES WITH 0.1.1:

        * Feature gate examples to make `cargo test` work on non-UEFI systems
          like CI.

        Contributions from: David Herrmann

        - Tübingen, 2018-12-10

## CHANGES WITH 0.1.0:

        * Initial release of r-efi.

        Contributions from: David Herrmann

        - Tübingen, 2018-12-10
