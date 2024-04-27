# StackCheckLib

## Table of Contents

- [StackCheckLib](#stackchecklib)
  - [Table of Contents](#table-of-contents)
  - [Introduction and Library Instances](#introduction-and-library-instances)
    - [StackCheckLibStaticInit](#stackchecklibstaticinit)
    - [StackCheckLibDynamicInit](#stackchecklibdynamicinit)
    - [StackCheckLibNull](#stackchecklibnull)
  - [How Failures are Handled](#how-failures-are-handled)
  - [Debugging Stack Cookie Check Failures](#debugging-stack-cookie-check-failures)
  - [Usage](#usage)

## Introduction and Library Instances

`StackCheckLib` contains the required functionality for initializing the stack cookie
value, checking the value, and triggering an interrupt when a mismatch occurs.
The stack cookie is a random value placed on the stack between the stack variables
and the return address so that continuously writing past the stack variables will
cause the stack cookie to be overwritten. Before the function returns, the stack
cookie value will be checked and if there is a mismatch then `StackCheckLib` handles
the failure.

Because UEFI doesn't use the C runtime libraries provided by MSVC, the stack
check code is written in assembly within this library. GCC and Clang compilers
have built-in support for stack cookie checking, so this library only handles failures.

### StackCheckLibStaticInit

`StackCheckLibStaticInit` is an instance of `StackCheckLib` which does not update the
stack cookie value for the module at runtime. It's always preferable to use
`StackCheckLibDynamicInit` for improved security but there are cases where the stack
cookie global cannot be written to such as in execute-in-place (XIP) modules and during
the Cache-as-RAM (CAR) phase of the boot process. The stack cookie value is initialized
at compile time via Project Mu updates to the AutoGen process. Each module will define
`STACK_COOKIE_VALUE` which is used for the module stack cookie value.

### StackCheckLibDynamicInit

`StackCheckLibDynamicInit` is an instance of `StackCheckLib` which updates the stack
cookie value for the module at runtime. This is the preferred method for stack cookie
initialization as it provides improved security. The stack cookie value is initialized
at runtime by calling `GetRandomNumber32()` or `GetRandomNumber64()` to generate a random
value via the platform's random number generator protocol. If the random number generator
returns an error, then the value will still have the build-time randomized value to fall
back on.

### StackCheckLibNull

`StackCheckLibNull` is an instance of `StackCheckLib` which does not perform any stack
cookie checks. This is useful for modules which will fail if stack cookie checks are
inserted. Of course, this is not recommended for production code.

## How Failures are Handled

When a stack cookie check fails, the `StackCheckLib` library will first call into a hook
function `StackCheckFailureHook()` which only has a NULL implementation in Project Mu.
The NULL implementation will simply print the failure address and return, but a platform
can implement their own instance of this library which can perform additional actions
before the system triggers an interrupt.

After `StackCheckFailureHook()` returns, the library will trigger an interrupt with
PcdStackCookieExceptionVector.

- On IA32 and X64 platforms, PcdStackCookieExceptionVector is used as an index into the
Interrupt Descriptor Table.
- On ARM platforms, a software interrupt (`SWI`) is called with the value of
PcdStackCookieExceptionVector. The value can be retrieved by the handler by reading
bits [7:0] of the instruction opcode which will allow the handler to determine if the
interrupt was triggered by the stack cookie check. Reference:
[Arm A64 Instruction Set Architecture Version 2024-3](https://developer.arm.com/documentation/ddi0597/2024-03/Base-Instructions/SVC--Supervisor-Call-?lang=en)
- On AARCH64 platforms, a supervisor call (`SVC`) is called with the value
of PcdStackCookieExceptionVector. This value can similarly be retrieved by the
handler to determine if the interrupt was triggered by the stack cookie check. Reference:
[Arm A64 Instruction Set Architecture Version 2024-3](https://developer.arm.com/documentation/ddi0602/2024-03/Base-Instructions/SVC--Supervisor-Call-?lang=en)

## Debugging Stack Cookie Check Failures

Tracking down the origin of stack cookie failures can be difficult. Programmers may attempt
printf debugging to determine which function has an overflow only to find that the failure
disappears on the next boot. This curiosity is usually due to the black-box heuristic used
by compilers to determine where to put stack cookie checks or compiler optimization features
removing the failing check. The address where the failed stack cookie check occurred will
be printed using DebugLib. If .map files are available, the address combined with the image
offset can be used to determine the function which failed.

GNU-based compilers have the `-fstack-protector-all` flag to force stack cookie checks on
all functions which could create a more consistent environment for debugging assuming an
earlier failure doesn't mask the targeted one and the flash space can accommodate the
increased size.

The Visual Studio (MSVC) toolchain has the ability to generate `.cod` files during compilation
which interleave C and the generated assembly code. These files will contain the stack cookie
checks and are useful for determining where the checks are placed. To generate these files,
append `/FAcs` to the build options for each target module. The easiest way to do this is to
update the tools_def file so the `<TARGET>_<TOOLCHAIN>_<ARCH>_CC_FLAGS` includes `/FAcs`.

## Usage

Project Mu updated the tools_def to add `/GS` to VS2022 and VS2019 IA32/X64 builds and
`-fstack-protector` to GCC builds. This will cause stack cookie references to be inserted
throughout the code. Every module should have a `StackCheckLib` instances linked to satisfy
these references. So every module doesn't need to add `StackCheckLib` to the LibraryClasses
section of the INF file, `StackCheckLib` instances should be linked as NULL in the platform
DSC fies. The only exception to this is host-based unit tests as they will be compiled with
the runtime libraries which already contain the stack cookie definitions and will collide
with `StackCheckLib`.

SEC and PEI_CORE modules should always use `StackCheckLibNull` and pre-memory modules
should use `StackCheckLibStaticInit`. All other modules should use `StackCheckLibDynamicInit`.
Below is an **example** of how to link the `StackCheckLib` instances in the platform DSC file
but it may need customization based on the platform's requirements:

```text
[LibraryClasses.common.SEC, LibraryClasses.common.PEI_CORE]
  NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf

[LibraryClasses.common.PEIM]
  NULL|MdePkg/Library/StackCheckLib/StackCheckLibStaticInit.inf

[LibraryClasses.common.MM_CORE_STANDALONE, LibraryClasses.common.MM_STANDALONE, LibraryClasses.common.DXE_CORE, LibraryClasses.common.SMM_CORE, LibraryClasses.common.DXE_SMM_DRIVER, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_SAL_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  NULL|MdePkg/Library/StackCheckLib/StackCheckLibDynamicInit.inf
```
