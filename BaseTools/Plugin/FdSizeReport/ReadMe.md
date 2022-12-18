# Flash Descriptor Size Report Generator Plugin and Command Line Tool

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About

FdSizeReportGenerator is a UEFI Build Plugin and Command Line Tool used to parse
EDK2 build reports and FDF files and then produce an HTML report of the module
sizes and fd sizes.  The HTML report then allows deeper analysis of the Flash
Usage, the Module Sizes, and overall breakdown of usage.

### UEFI Build Plugin

When used in the plugin capacity this plugin will do its work in the
do_post_build function.  This plugin uses the following variables from the build
environment:

 1. BUILDREPORTING - [REQUIRED] - must be True otherwise plugin will not run
 1. BUILDREPORT_TYPES - [REQUIRED] - for FdSizeReport, should at least contain the FLASH parameter.
 1. FLASH_DEFINITION - [REQUIRED] - must point to the platform FDF file
 1. BUILDREPORT_FILE - [REQUIRED] - must point to the build report file
 1. FDSIZEREPORT_FILE - [OPTIONAL] - should be path for output HTML report.  If
    not set default path will be set based on *BUILD_OUTPUT_BASE* variable
 1. PRODUCT_NAME - [OPTIONAL] - should give friendly product name
 1. BUILDID_STRING - [OPTIONAL] - should give friendly version string of
    firmware version

#### Integrating into a self describing build environment

In a self describing build environment, the self.env.SetValue can be used to set the environment variables

The below example is relevant to supporting QemuQ35 platform

```INI
   self.env.SetValue("BUILDREPORTING", "TRUE", "Platform Hardcoded")
   self.env.SetValue("BUILDREPORT_TYPES", "PCD DEPEX FLASH BUILD_FLAGS LIBRARY", "Platform Hardcoded")
   self.env.SetValue("FLASH_DEFINITION", "QemuQ35Pkg.fdf", "Platform Hardcoded")
   self.env.SetValue("BUILDREPORT_FILE", "BuildReport.txt", "Platform Hardcoded")
   self.env.SetValue("PRODUCT_NAME", "QemuQ35", "Platform Hardcoded")
```

### Command Line Tool

When used as a command line tool check the required parameters by using the -h
option.
