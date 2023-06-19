# GoogleTest - Unit Testing Framework 

## About
This unit test framework, called **GoogleTest** is implemented as a set of EDK II libraries. It is one of two unit test frameworks supported by EDK II. Please refer to [the ReadMe](./ReadMe.md) for a comparison of the two.
[GoogleTest](http://google.github.io/googletest/) and can be used to implement host-based unit tests. [GoogleTest on GitHub](https://github.com/google/googletest) is included in the UnitTestFrameworkPkg as a submodule. Use of GoogleTest for target-based unit tests of EDK II components is not supported.
Host-based unit tests that require mocked interfaces can use the mocking infrastructure included with GoogleTest called [gMock](https://github.com/google/googletest/tree/main/googlemock).


## GoogleTest Samples

There is a sample unit test provided as both an example of how to write a unit test and leverage
many of the GoogleTest features. This sample can be found in the `Test/GoogleTest/Sample/SampleGoogleTest`
directory.

The sample is provided for the HOST_APPLICATION build type, which can be run on a host system without
needing a target.

There is also a sample unit test provided as both an example of how to write a unit test with
mock functions and leverage some of the gMock features. This sample can be found in the
`SecurityPkg/Library/SecureBootVariableLib/GoogleTest` directory.

It too is provided for the HOST_APPLICATION build type, which can be run on a host system without
needing a target.

## GoogleTest Usage

This section is built a lot like a "Getting Started". We'll go through some of the components that are needed
when constructing a unit test and some of the decisions that are made by the test writer. We'll also describe
how to check for expected conditions in test cases and a bit of the logging characteristics.

Most of these examples will refer to the `SampleGoogleTestHost` app found in this package, but
the examples related to mock functions will refer to the `SecureBootVariableLibGoogleTest` app
found in the `SecurityPkg`.

### GoogleTest Requirements - INF

In our INF file, we'll need to bring in the `GoogleTestLib` library. Conveniently, the interface
header for the `GoogleTestLib` is in `UnitTestFrameworkPkg`, so you shouldn't need to depend on any other
packages. As long as your DSC file knows where to find the lib implementation that you want to use,
you should be good to go.

See this example in `SampleGoogleTestHost.inf`...

```inf
[Packages]
  MdePkg/MdePkg.dec
  UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec

[LibraryClasses]
  GoogleTestLib
  BaseLib
  DebugLib
```

Also, if you want your test to automatically be picked up by the Test Runner plugin, you will need
to make sure that the module `BASE_NAME` contains the word `Test`...

```inf
[Defines]
  BASE_NAME      = SampleGoogleTestHost
```

### GoogleTest Requirements - DSC

In our DSC fiele, we'll need to bring in the INF file that was just created into the `[Components]`
section so that the unit tests will be built.

See this example in `UnitTestFrameworkPkgHostTest.dsc`...

```inf
#include <gtest/gtest.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
}
```

The first include brings in the GoogleTest definitions. Other EDK II related include
files must be wrapped in `extern "C" {}` because they are C include files. Link
failures will occur if this is not done.

Also, when using GoogleTest it is helpful to add a `using` declaration for its
`testing` namespace. This `using` statement greatly reduces the amount of code you
need to write in the tests when referencing the utilities within the `testing`
namespace. For example, instead of writing `::testing::Return` or `::testing::Test`,
you can just write `Return` or `Test` respectively, and these types of references
occur numerous times within the tests.

Lastly, in the case that tests within a GoogleTest application require the usage of
mock functions, it is also necessary to include the header files for those interfaces
as well. As an example, the `SecureBootVariableLibGoogleTest` uses the mock versions
of `UefiLib` and `UefiRuntimeServicesTableLib`. So its test file contains the below
includes. Note that the `using` declaration mentioned above is also shown in the code
below for completeness of the example.

```cpp
#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockUefiLib.h>
#include <GoogleTest/Library/MockUefiRuntimeServicesTableLib.h>

extern "C" {
  #include <Uefi.h>
  ...
}

using namespace testing;
```

Now that we've got that squared away, let's look at our 'Main()' routine (or DriverEntryPoint() or whatever).

### GoogleTest Configuration

Unlike the Framework, GoogleTest does not require test suites or test cases to
be registered. Instead, the test cases declare the test suite name and test
case name as part of their implementation. The only requirement for GoogleTest
is to have a `main()` function that initializes the GoogleTest infrastructure
and calls the service `RUN_ALL_TESTS()` to run all the unit tests.

```cpp
int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
```

However, while GoogleTest does not require test suites or test cases to be
registered, there is still one rule within EDK II that currently needs to be
followed. This rule is that all tests for a given GoogleTest application must
be contained within the same source file that contains the `main()` function
shown above. These tests can be written directly in the file or a `#include`
can be used to add them into the file indirectly.

The reason for this is due to EDK II taking the host application INF file and
first compiling all of its source files into a static library. This static
library is then linked into the final host application. The problem with this
method is that only the tests in the object file containing the `main()`
function are linked into the final host application. This is because the other
tests are contained in their own object files within the static library and
they have no symbols in them that the final host application depends on, so
those object files are not linked into the final host application.

### GoogleTest - A Simple Test Case

Below is a sample test case from `SampleGoogleTestHost`.

```cpp
TEST(SimpleMathTests, OnePlusOneShouldEqualTwo) {
  UINTN  A;
  UINTN  B;
  UINTN  C;

  A = 1;
  B = 1;
  C = A + B;

  ASSERT_EQ (C, 2);
}
```

This uses the simplest form of a GoogleTest unit test using `TEST()` that
declares the test suite name and the unit test name within that test suite.
The unit test performs actions and typically makes calls to the code under test
and contains test assertions to verify that the code under test behaves as
expected for the given inputs.

In this test case, the `ASSERT_EQ` assertion is being used to establish that the business logic has functioned
correctly. There are several assertion macros, and you are encouraged to use one that matches as closely to your
intended test criterium as possible, because the logging is specific to the macro and more specific macros have more
detailed logs. When in doubt, there are always `ASSERT_TRUE` and `ASSERT_FALSE`. Assertion macros that fail their
test criterium will immediately return from the test case with a failed status and log an error string.
_Note_ that this early return can have implications for memory leakage.

For most `ASSERT` macros in GoogleTest there is also an equivalent `EXPECT` macro. Both macro versions
will ultimately cause the `TEST` to fail if the check fails. However, the difference between the two
macro versions is that when the check fails, the `ASSERT` version immediately returns from the `TEST`
while the `EXPECT` version continues running the `TEST`.

There is no return status from a GooglTest unit test. If no assertions (or expectations) are
triggered then the unit test has a passing status.

### GoogleTest - A gMock Test Case

Below is a sample test case from `SecureBootVariableLibGoogleTest`. Although
actually, the test case is not written exactly like this in the test file, but
more on that in a bit.

```cpp
TEST(SetSecureBootModeTest, SetVarError) {
  MockUefiRuntimeServicesTableLib RtServicesMock;
  UINT8                           SecureBootMode;
  EFI_STATUS                      Status;

  // Any random magic number can be used for these tests
  SecureBootMode = 0xAB;

  EXPECT_CALL(RtServicesMock, gRT_SetVariable)
    .WillOnce(Return(EFI_INVALID_PARAMETER));

  Status = SetSecureBootMode(SecureBootMode);
  EXPECT_EQ(Status, EFI_INVALID_PARAMETER);
}
```

Keep in mind that this test is written to verify that `SetSecureBootMode()` will
return `EFI_INVALID_PARAMETER` when the call to `gRT->SetVariable()` within the
implementation of `SetSecureBootMode()` returns `EFI_INVALID_PARAMETER`. With that
in mind, let's discuss how a mock function is used to accomplish this in the test.

In this test case, the `MockUefiRuntimeServicesTableLib` interface is instantiated as
`RtServicesMock` which enables its associated mock functions. These interface
instantiations that contain the mock functions are very important for mock function
based unit tests because without these instantiations, the mock functions within that
interface will not exist and can not be used.

The next line of interest is the `EXPECT_CALL`, which is a standard part of the gMock
framework. This macro is telling the test that a call is expected to occur to a
specific function on a specific interface. The first argument is the name of the
interface object that was instantiated in this test, and the second argument is the
name of the mock function within that interface that is expected to be called. The
`WillOnce(Return(EFI_INVALID_PARAMETER))` associated with this `EXPECT_CALL` states
that the `gRT_SetVariable()` function (remember from earlier in this documentation
that this refers to the `gRT->SetVariable()` function) will be called once during
this test, and when it does get called, we want it to return `EFI_INVALID_PARAMETER`.

Once this `EXPECT_CALL` has been setup, the call to `SetSecureBootMode()` occurs in
the test, and its return value is saved in `Status` so that it can be tested. Based
on the `EXPECT_CALL` that was setup earlier, when `SetSecureBootMode()` internally
calls `gRT->SetVariable()`, it returns `EFI_INVALID_PARAMETER`. This value should
then be returned by `SetSecureBootMode()`, and the
`EXPECT_EQ(Status, EFI_INVALID_PARAMETER)` verifies this is the case.

There is much more that can be done with `EXPECT_CALL` and mock functions, but we
will leave those details to be explained in the gMock documentation.

Now it was mentioned earlier that this test case is not written exactly like this
in the test file, and the next section describes how this test is slightly
refactored to reduce the total amount of code in the entire test suite.

### GoogleTest - A gMock Test Case (refactored)

The sample test case from `SecureBootVariableLibGoogleTest` in the prior section is
actually written as shown below.

```cpp
class SetSecureBootModeTest : public Test {
  protected:
    MockUefiRuntimeServicesTableLib RtServicesMock;
    UINT8       SecureBootMode;
    EFI_STATUS  Status;

    void SetUp() override {
      // Any random magic number can be used for these tests
      SecureBootMode = 0xAB;
    }
};

TEST_F(SetSecureBootModeTest, SetVarError) {
  EXPECT_CALL(RtServicesMock, gRT_SetVariable)
    .WillOnce(Return(EFI_INVALID_PARAMETER));

  Status = SetSecureBootMode(SecureBootMode);
  EXPECT_EQ(Status, EFI_INVALID_PARAMETER);
}
```

This code may at first seem more complicated, but you will notice that the code
with in it is still the same. There is still a `MockUefiRuntimeServicesTableLib`
instantiation, there is still a `SecureBootMode` and `Status` variable defined,
there is still an `EXPECT_CALL`, and etc. However, the benefit of constructing
the test this way is that the new `TEST_F()` requires less code than the prior
`TEST()`.

This is made possible by the usage of what GoogleTest calls a _test fixture_.
This concept of a test fixture allows multiple tests to use (or more specifically
inherit from a base class) a common set of variables and initial conditions.
Notice that using `TEST_F()` requires the first argument to be a name that aligns
with a test fixture (in this case `SetSecureBootModeTest`), and the second
argument is the name of the test (just like in the `TEST()` macro).

All `TEST_F()` tests that use a specific test fixture can be thought of as having
all of that test fixture's variables automatically defined in the test as well as
having that text fixture's `SetUp()` function called before entering the test.

This means that another `TEST_F()` can be written without needing to worry about
defining a bunch of variables or instantiating a bunch of interfaces for mock
functions. For example, the below test (also in `SecureBootVariableLibGoogleTest`)
uses the same test fixture and makes use of its `RtServicesMock`, `Status`, and
`SecureBootMode` variables.

```cpp
TEST_F(SetSecureBootModeTest, PropogateModeToSetVar) {
  EXPECT_CALL(RtServicesMock,
    gRT_SetVariable(
      Char16StrEq(EFI_CUSTOM_MODE_NAME),
      BufferEq(&gEfiCustomModeEnableGuid, sizeof(EFI_GUID)),
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof(SecureBootMode),
      BufferEq(&SecureBootMode, sizeof(SecureBootMode))))
    .WillOnce(Return(EFI_SUCCESS));

  Status = SetSecureBootMode(SecureBootMode);
  EXPECT_EQ(Status, EFI_SUCCESS);
}
```

The biggest benefit is that the `TEST_F()` code can now focus on what is being
tested and not worry about any repetitive setup. There is more that can be done
with test fixtures, but we will leave those details to be explained in the
gMock documentation.

Now, as for what is in the above test, it is slightly more complicated than the
first test. So let's explain this added complexity and what it is actually
testing. In this test, there is still an `EXPECT_CALL` for the
`gRT_SetVariable()` function. However, in this test we are stating that we
expect the input arguments passed to `gRT_SetVariable()` be specific values.
The order they are provided in the `EXPECT_CALL` align with the order of the
arguments in the `gRT_SetVariable()` function. In this case the order of the
`gRT_SetVariable()` arguments is as shown below.

```cpp
IN  CHAR16                       *VariableName,
IN  EFI_GUID                     *VendorGuid,
IN  UINT32                       Attributes,
IN  UINTN                        DataSize,
IN  VOID                         *Data
```

So in the `EXPECT_CALL` we are stating that the call to `gRT_SetVariable()`
will be made with the below input argument values.

1. `VariableName` is equal to the `EFI_CUSTOM_MODE_NAME` string
2. `VendorGuid` is equal to the `gEfiCustomModeEnableGuid` GUID (which has a byte length of `sizeof(EFI_GUID)`)
3. `Attributes` is equal to `EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS`
4. `DataSize` is equal to `sizeof(SecureBootMode)`
5. `Data` is equal to `SecureBootMode` (which has a byte length of `sizeof(SecureBootMode)`)

If any one of these input arguments does not match in the actual call to
`gRT_SetVariable()` in the design, then the test will fail. There is much more
that can be done with `EXPECT_CALL` and mock functions, but again we will
leave those details to be explained in the gMock documentation.

### GoogleTest - More Complex Cases

To write more advanced tests, take a look at the
[GoogleTest User's Guide](http://google.github.io/googletest/).


// TODO - What more information can add value?

### If still in doubt

Hop on GitHub and ask @corthon, @mdkinney, or @spbrogan. ;)

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
