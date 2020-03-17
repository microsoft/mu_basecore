/** @file
  Lib to include if using floats

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// create a global to satisfy the compilers insertion of the _fltused
// in reponse to detecting floating point type operations.
int  _fltused = 0x9875;
