/** @file
  Lib to include if using floats

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// create a global to satisfy the compilers insertion of the _fltused
// in reponse to detecting floating point type operations.
//
// In some systems (not EDK2) this is used to tune static C runtime linking.
// i.e. There are (larger) working and (small stub) non-working floating point functions,
// and _fltused is in the object files with the working functions.
char  _fltused;
