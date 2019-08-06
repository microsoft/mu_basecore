# Override Validation Plugin

Module Level Override Validation Plugin and Linkage Creation Command Line Tool

## About

OverrideValidation is a UEFI Build Plugin and Command Line Tool used to create linkage between overriding and overridden modules and parse INF files referenced in platform DSC files during build process and then produce a TXT report of the module overriding status.  The TXT report then allows deeper analysis of the Overriding Hierarchy, the Override Linkage Validity, the Override Linkage Ages, and overall breakdown of usage.

### UEFI Build Plugin

When used in the plugin capacity this plugin will do its override linkage validation work in the do_pre_build function.  This plugin uses the following variables from the build environment:

 1. ACTIVE_PLATFORM - [REQUIRED] - must be workspace relative or package path relative pointing to the target platform dsc file, otherwise this validation will not run
 1. BUILD_OUTPUT_BASE - [REQUIRED] - must be an absolute path specified to store override log at $(BUILD_OUTPUT_BASE)/OVERRIDELOG.TXT, otherwise no report will be generated
 1. BUILDSHA - [OPTIONAL] - should have valid commit sha value for report purpose, if not provided, 'None' will be used for the corresponding field
 1. PRODUCT_NAME - [OPTIONAL] - should give friendly product name, if not provided, 'None' will be used for the corresponding field
 1. BUILDID_STRING - [OPTIONAL] - should give friendly version string of firmware version, if not provided, 'None' will be used for the corresponding field

### Command Line Tool

When used as a command line tool, this tool takes the absolute path of workspace (the root directory of Devices repo) as well as the absolute path of overridden module's inf file and then generate a screen-print line for users to include in overriding modules in order to create override linkage. Check the required parameters by using the -h option for command line argument details.

### Example

Command to generate an override record:

``` cmd
OverrideValidation.py -w C:\Repo -m C:\Repo\SM_UDK\MdePkg\Library\BaseMemoryLib\BaseMemoryLib.inf
```

Override record to be included in overriding module's inf:

``` cmd
#Override : 00000001 | MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf | cc255d9de141fccbdfca9ad02e0daa47 | 2018-05-09T17-54-17
```

Override log generated during pre-build process:

``` cmd
Platform:     PlatformName
Version:      123.456.7890
Date:         2018-05-11T17-56-27
Commit:       _SHA_2c9def7a4ce84ef26ed6597afcc60cee4e5c92c0
State:        3/4

Overrides
----------------------------------------------------------------

OVERRIDER: MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf
ORIGINALS:
  + MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf | SUCCESS | 2 days

OVERRIDER: PlatformNamePkg/Library/NvmConfigLib/NvmConfigLib.inf
ORIGINALS:
  + MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf | MISMATCH | 35 days
  | Current State: 62929532257365b261080b7e7b1c4e7a | Last Fingerprint: dc9f5e3af1efbac6cf5485b672291903
  + MdePkg/Library/BaseMemoryLibOptDxe/BaseMemoryLibOptDxe.inf | SUCCESS | 0 days
  + MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf | SUCCESS | 2 days

```

## Copyright & License

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
