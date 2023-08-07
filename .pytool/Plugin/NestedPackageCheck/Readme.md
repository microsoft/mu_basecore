# Nested Package Check Plugin

This CiBuildPlugin detects nested packages and fails if the nested package is due to the currently tested package

## What is a Nested Package?

As the name suggests, a nested package is when one package is located in a subdirectory of another package. Depending on
multiple factors (such as package layouts, the `PACKAGES_PATH` environment variable, and the order of paths in the
`PACKAGES_PATH ` environment variable), nested packages can cause hard to diagnose, or silent, build issues due to
incorrect path resolution. Due to this, the DEC Specification requires that EDK II packages cannot be nested  within
other EDKII Packages.

[DEC Specification v1.27](https://tianocore-docs.github.io/edk2-DecSpecification/release-1.27/2_dec_file_overview/#2-dec-file-overview)
\- *"An EDK II Package (directory) is a directory that contains an EDK II package declaration (DEC) file. Only one DEC
file is permitted per directory. EDK II Packages cannot be nested within other EDK II Packages."*

### Nested Package Example

```cmd
# An invalid layout of packages due to nested packages
c:/src/EDK2  # WORKSPACE
│
├── MdePkg  # PACKAGE - Valid
│   ├── MdePkg.dec
│   ├── MdeModulePkg  # PACKAGE - Nested
│   │   └── MdeModulePkg.dec
│   └── Include
│       └── CryptoPkg  # PACKAGE - Nested
│           └── CryptoPkg.dec
└── NetworkPkg
    └── NetworkPkg.dec  # PACKAGE - Valid

# A valid layout of packages
c:/src/EDK2  # WORKSPACE
│
├── MdePkg
│   └── MdePkg.dec
├── MdeModulePkg
│   └── MdeModulePkg.dec
├── CryptoPkg
│   └── CryptoPkg.dec
└── NetworkPkg
    └── NetworkPkg.dec
```

## Plugin Configuration

``` yaml
    "NestedPackageCheck": {
        "AuditOnly": False,
    }

```

### AuditOnly

Boolean - Default is False.
If True, run the test in an Audit only mode which will log all errors but instead of failing the
build, it will set the test as skipped.  This allows visibility into failures without breaking the
build.
