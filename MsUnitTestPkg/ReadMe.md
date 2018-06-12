# Ms Unit Test Support Package
## &#x1F539; Copyright
Copyright (c) 2017, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## &#x1F539; About
This package adds a unit test framework targeted at the UEFI shell environment. 
It allows for unit test development to focus on the tests and leave, error logging, result formatting,
context persistance, and test running to the framework.  The unit test framework works well for low
level unit tests as well as system level tests and fits easily in automation frameworks. 

The code is designed for a unit test application to leverage the framework which is made 
up of a number of libraries which allow for easy customization of the different elements.  
A few different instances are created to both show how easy some behaviors can be customized as 
well as provide different implementatiosn that support different use cases.  

### &#x1F538; UnitTestLib
The main "framework" library.  This provides the framework init, suite init, and add test case 
functionality.  It also supports the running of the suites and logging/reporting of results.

### &#x1F538; UnitTestAssetLib
The UnitTestAssetLib provides helper macros and functions for checking test conditons and 
reporting errors.  Status and error info will be logged into the test context.  There are a number
of Assert macros that make the unit test code friendly to view and easy to understand.  


### &#x1F538; UnitTestBootUsbLib
One of the unique features of the unit test framework is to be able to save text context
and reboot the system.  Since unit tests are generally run from a bootable usb key the framework
has library calls to set boot next for usb.  There is numerous ways this could be done on a given
platform / BDS implementation and therefore this simple library allows customization if needed. 
This package supplies two intstances:
* UsbClass Lib: This uses the Usb Class boot option as defined in the UEFI spec and leveraged 
by industry standard USB applications.  
* UsbMicrosoft Lib: This uses a private boot option found in Microsoft UEFI to boot to usb 

### &#x1F538; UnitTestLogLib
Library to support logging information during the test execution.  This data is logged to the test 
context and will be available in the test reporting phase.  This shold be used for logging test
details and helpful messages to resolve test failures.  

### &#x1F538; UnitTestResultReportLib
Library provides function to run at the end of a framework test run and responsibile for the 
report format.  This is a common customization point and allows the unit test framework to fit 
its output reports into other test infrastructure.  In this package a simple library instances has 
been supplied to output test results to the console as plain text.
  

### &#x1F538; UnitTestPersistenceLib
Persistence lib has the main job of saving and restoring test context to a storage medium so that for tests
that require exiting the active process and then resuming state can be maintained.  This is critical
in supporting a system reboot in the middle of a test run.  

## &#x1F539; Samples
There is a sample unit test provided as both an example of how to write a unit test and leverage 
many of the features of the framework.  This sample can be found in the SampleUnitTestApp directory.    

## &#x1F539; Usage
TBD - Write how to use the code
 