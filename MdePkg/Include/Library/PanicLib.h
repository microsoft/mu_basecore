/** @file
  Provides services to print panic messages to a debug output device.

  Note that a reserved macro named MDEPKG_NDEBUG is brought from debuglib for the intention
  of size reduction when compiler optimization is disabled. If MDEPKG_NDEBUG is
  defined, then panic related macros wrapped by it are the NULL implementations.

Copyright (c) 2023, Microsoft Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PANIC_LIB_H__
#define __PANIC_LIB_H__

/**
  Prints a panic message containing a filename, line number, and description.
  This is always followed by a dead loop.

  Print a message of the form "PANIC <FileName>(<LineNumber>): <Description>\n"
  to the debug output device.  Immediately after that CpuDeadLoop() is called.

  If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.
  If Description is NULL, then a <Description> string of "(NULL) Description" is printed.

  @param  FileName     The pointer to the name of the source file that generated the panic condition.
  @param  LineNumber   The line number in the source file that generated the panic condition
  @param  Description  The pointer to the description of the panic condition.

**/
VOID
EFIAPI
PanicReport (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  );

//
// Source file line number.
// Default is use the to compiler provided __LINE__ macro value. The __LINE__
// mapping can be overriden by predefining PANIC_LINE_NUMBER
//
// Defining PANIC_LINE_NUMBER to a fixed value is useful when comparing builds
// across source code formatting changes that may add/remove lines in a source
// file.
//
#ifdef PANIC_LINE_NUMBER
#else
#define PANIC_LINE_NUMBER  __LINE__
#endif

#if defined (__clang__) && defined (__FILE_NAME__)
#define _PANIC(Message)  PanicReport (__FILE_NAME__, PANIC_LINE_NUMBER, Message)
#else
#define _PANIC(Message)  PanicReport (__FILE__, PANIC_LINE_NUMBER, Message)
#endif

/**
  Macro that calls PanicReport().

  If MDEPKG_NDEBUG is not defined PanicReport() is called passing
  in the source filename, source line number, and Message.

  @param  Expression  A format string

**/
#if !defined (MDEPKG_NDEBUG)
#define PANIC(Message)        \
    do {                            \
      _PANIC (Message);     \
      ANALYZER_UNREACHABLE ();  \
    } while (FALSE)
#else
#define PANIC(Message)
#endif

#endif // __PANIC_LIB_H__
