# Host UnitTest Compiler Plugin

A CiBuildPlugin that compiles the dsc for host based unit test apps.

To run the unit tests and collect the results after successful compilation, The
host UnitTest Compliler Plugin will execute any IUefiBuildPlugin that has the
scope 'host-based-test'.

## Configuration

The package relative path of the DSC file to build.

``` yaml
"HostUnitTestCompilerPlugin": {
    "DscPath": "<path to dsc from root of pkg>"
}
```

### DscPath

Package relative path to the DSC file to build.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

