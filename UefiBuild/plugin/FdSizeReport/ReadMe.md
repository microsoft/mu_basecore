# Flash Descriptor Size Report Generator Plugin and Command Line Tool

## Copyright

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## About

FdSizeReportGenerator is a UEFI Build Plugin and Command Line Tool used to parse EDK2 build reports and FDF files and then produce an HTML report of the module sizes and fd sizes.  The HTML report then allows deeper analysis of the Flash Usage, the Module Sizes, and overall breakdown of usage.


### UEFI Build Plugin
When used in the plugin capacity this plugin will do its work in the do_post_build function.  This plugin uses the following variables from the build environment: 
 1. BUILDREPORTING - [REQUIRED] - must be True otherwise plugin will not run  
 1. FLASH_DEFINITION - [REQUIRED] - must point to the platform FDF file
 1. BUILDREPORT_FILE - [REQUIRED] - must point to the build report file
 1. FDSIZEREPORT_FILE - [OPTIONAL] - should be path for output HTML report.  If not set default path will be set based on *BUILD_OUTPUT_BASE* variable
 1. PRODUCT_NAME - [OPTIONAL] - should give friendly product name
 1. BUILDID_STRING - [OPTIONAL] - should give friendly version string of firmware version


### Command Line Tool
When used as a command line tool check the required parameters by using the -h option. 
