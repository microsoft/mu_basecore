# FltUsedLib

This library provides a global (`_fltused`) that the compiler generates in response to seeing
floating point type operations.

Examples are using `float`, `double` or using floating point conversions (assigning integers
to `0.1` would be a conversion). The compiler generates the `_fltused`, and a CRT type library
(C Runtime) will checks the value to see if C Floating Point libraries need to be linked in
final binary generation.

There are only a few places in Edk2/MU today that use float or double, but there are places
in silicon vendor provided code. The problematic situation that we run into is the libraries.
If a library knows that it uses floating points, and it defines its own `_fltused` value, then
it will be able to build by itself. If each library defines their own `_fltused`, it can generate
another set of linker problems, of duplicate defined symbols. For example, if MathLib (MU_PLUS)
defines its `_fltused`, and MathLib is linked into a module that also consumes a silicon vendor
library, the linker will see duplicate defined symbols and fail.

Relying into Edk2's LibraryClasses provides relief for both these scenarios. Each library/module
that triggers `_fltused` will be satisfied if they all consume the FltUsed library. Modules which
consume multiple libraries which rely on `_fltused`, will also be satisfied as only one declaration
of fltused will be linked into the final executable.

## Using

To use FltUsedLib, just include it in the INF of the module containing floating point flagged code.

```inf
[LibraryClasses]
  BaseLib
  BaseMemoryLib
  FltUsedLib
```

## Copyright & License

Copyright (c) Microsoft Corporation

SPDX-License-Identifier: BSD-2-Clause-Patent
