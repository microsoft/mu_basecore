# PECOFF Image Validation - Post Build pe Image Validation Plugin

This tool will validate all pe images against a set of tests and their
associated requirements as defined below. A configuration file is used to
describe different profiles, and their associated requirements. A configuration
file path is provided via the command line as `PE_VALIDATION_PATH=<PATH>` or
can be configured in the the PlatformBuild.py within the `SetPlatformEnv()`
method using 
`self.env.SetValue("PE_VALIDATION_PATH", <PATH>, "Platform Hardcoded")`. A 
profile is equivalent to the binary's MODULE_TYPE as defined in it's inf. All
profiles must be defined, forcing the developer to acknowledge each, however
requirements for each profile do not need to be specified... If one or more
requirement does not exist, the "DEFAULT" profile requirements will be used.
The developer can have default requirements via the "DEFAULT" profile then
override those requirements in other profiles. An example of a full config
file can be seen at the bottom of the readme.

A default set of requirements are provided by the tool that apply to all
X64 and AARCH64 binaries. These requirements are:

1. IMAGE_BASE = 0x0
2. 4k Section Alignment (X64, AARCH64), 64k Section Alignment (AARCH64-DXE_RUNTIME_DRIVER)
3. No Section can be both Write and Execute.

Image Base is verified to be 0x0 for EFIs as some PE loaders (including edk2's)
will attempt to load the image at that address, which is generally not needed in a
UEFI environment. Section alignment is verified at 4k as it is required for DXE
memory protections, exclduing AARCH64 DXE_RUNTIME_DRIVER binaries, which are
required to be 64K per the UEFI specifcation. Sections are verified **not** to be
both Write and Execute as it is required by DXE memory protections.

These requirements can be expanded to IA32, ARM, etc, or overwritten for X64
and AARCH64 as defined in the configuration file provided via PE_VALIDATION_PATH.
Individual profiles for individual architectures can be overwritten in this file.

Configuration supports poth `json` and `yaml` formats. Here is a small example for setting
an ignore list, and overriding configuration for a specific module type. Read below for all
customization options.

```json
{
    "IGNORE_LIST": ["Driver.efi"],
    "X64": {
        "DXE_DRIVER": {
            ...
        }
    } 
}
```

```yaml
IGNORE_LIST:
    - Driver.efi
X64:
    DXE_DRIVER:
        ...

```

**See Example below for customizing Image Validation**

## Ignoring Files

If your platform deems a particular binary does not, and cannot meet the
requirements set by the Image Validation plugin, or the platform's custom
config, it can be ignored by adding a `IGNORE_LIST = [...]` section to the
configuration file provided via PE_VALIDATION_PATH.

## Common Errors

### Profile type <PROFILE_NAME> is invalid. Exiting...

PROFILE_NAME is not specified in the configuration file,
but is defined in the platform's fdf. The profile needs to be added to the
configuration file, even if the requirements are the same as the DEFAULT
requirements. This was a design choice to ensure the platform is not
accidentally passing due to falling back to the DEFAULT profile if a profile
is missing.

### Test specific failures

See the below Tests section for test specific failures.

## Tests

### Section Data / Code Separation Verification

- Description: This test ensures that each section of the binary is not both
write-able and execute-able. Sections can only be one or the other
(or neither). This test is done by iterating over each section and checking the
characteristics label for the Write Mask (0x80000000) and Execute
Mask (0x20000000).

- Default Requirement: `DATA_CODE_SEPARATION: True` for X64 and AARCH64

- Config File Requirements: ```"DATA_CODE_SEPARATION": <True or False>```

- Output:
  - @Success: Only one (or neither) of the two masks (Write, Execute) are present
  - @Fail   : Both the Write and Execute flags are present

- Possible Solution: Update the failed section's characteristics to ensure it
is either Write-able or Read-able, but not both.

### Section Alignment Verification

- Description: Checks the section alignment value found in the optional header.
This value must meet the requirements specified in the config file.

- Default Requirement: 4K for X64 and AARCH64. 64k For AARCH64 DXE_RUNTIME_DRIVER

- Config File Requirements: An array of dictionaries that contain a Comparison
and a value for the particular MachineType and FV file type. See the below
example. Can optionally describe an Alignment logic separator when doing
multiple comparisons.

```json
"ALIGNMENT_LOGIC_SEP": "OR",
"ALIGNMENT" : [
    {
        "COMPARISON" : "==",
        "VALUE"      : 64
    },
    {
        "COMPARISON" : "==",
        "VALUE"      : 32
    }
]
```

- Output:
  - @Success: Image alignment passes all requirements specified in the config
  file
  - @Warn   : Image alignment value is not found in the optional header, or the
  value is set to 0
  - @Fail   : Image alignment does not meet the requirements specified in the
  config file

- Possible Solution: Update the section alignment of the binary to match the
requirements specified in the config file.

### Target Base Address Validation

- Description: Checks the base address value found in the optional header.
This value must meet the requirements specified in the config file.

- Default Requirement: `IMAGE_BASE: 0`

- Config File Requirements: ```IMAGE_BASE: <integer>```

### Writing your own tests

If a developer wishes to write their own test, they must implement the
interface described in the Test Interface class:

```python
class TestInterface:
    def name(self):
        """Returns the name of the test"""
        raise NotImplementedError("Must Override Test Interface")

    def execute(self, parser, config_data):
        """Executes the test"""
        raise NotImplementedError("Must Override Test Interface")
```

The parser is the parsed pe file that you are testing. Documentation on how to
use the parser can be found by looking up the documentation for the pefile
module. The config_data provided to the test will is the filtered data from the
config file based upon the compilation target and profile. As an example,
looking at the configuration file, if a pe that is being validated is of type
IMAGE_FILE_MACHINE_ARM64 and profile BASE, the config_data provided will be:

```json
"ALIGNMENT" : [
    {
        "COMPARISON" : ">=",
        "VALUE"      : 4096   
    },
    {
        "COMPARISON" : "!=",
        "VALUE"      : 65536
    }     
]
```

The developer also has the ability to provide additional requirements other
then ALIGNMENT, and those requirements will also be provided to the test as
seen in the below example:

```json
"ALIGNMENT" : [
    {
        "COMPARISON" : ">=",
        "VALUE"      : 4096   
    },
    {
        "COMPARISON" : "!=",
        "VALUE"      : 65536
    }     
],
"OTHER_REQUIREMENT"  : 5,
"OTHER_REQUIREMENT2" : {"Req1" : 1, "Req2" : 2}
```

For the test to be executed, provide the test to the test manager using
```add_test(test)``` or ```add_tests(tests)``` functions

## Config File Example

The config file is used to describe all requirements for the scanned pe file.
The configuration data provided to each test is provided to the test by first
locating the Compilation Target, then by the optional profile parameter. If
no profile parameter is provided, "DEFAULT" is used. The current allowed 
settings are as follows:

### Top Level Settings

**WARNING: The Configuration file must start with brackets, i.e. `{ <CONTENT> }`**

#### IGNORE_LIST

This defines a list of all pe file names that this tool should not execute on.

```json
"IGNORE_LIST" : ["efi1", "efi2", "etc"]
```

#### IMAGE_FILE_MACHINE_XXXX

This will be any number of supported Image File Machine Constants that are supported by the build system. This will not be a list (using [ ]), rather a comma separated list of all machine constants.

``` json
"IMAGE_FILE_MACHINE_XXX1" : {"<Profiles>"},
"IMAGE_FILE_MACHINE_XXX2" : {"<Profiles>"},
```

### Machine Level Settings

#### Profiles

This will be any number of supported profiles for the particular Image File Machine Constant. This will not be a list (using [ ]), rather a comma separated list of all machine constants.

```json
"DXE_CORE" : {"<Settings>"},
"UEFI_APPLICATION" : {"<Settings>"}
```

### Profile Level Settings

#### DATA_CODE_SEPARATION

This setting controls if data code separation (cannot be both write and execute) are required for this profile.

``` json
"DATA_CODE_SEPARATION" : <bool>
```

#### ALLOWED_SUBSYSTEMS

This setting allows the developer to specify the type of subsystem the efi should be for a particular profile. Subystems are defined at <https://docs.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-170>.

``` json
"ALLOWED_SUBSYSTEMS": ["subsystem1", "subsystem2", "etc"]
```

#### ALIGNMENT

This setting allows the developer to specify memory alignment requirements for a particular profile as a list of requirements.

``` json
"ALIGNMENT" : [
    {
        "COMPARISON" : "<Comparison Operator>",
        "VALUE" : <Value>
    },
    {
        "COMPARISON" : "<Comparison Operator>",
        "VALUE" : <Value>
    }
]
```

#### ALIGNMENT_LOGIC_SEP

This setting is only used if the alignment requirements specify multiple requirements. It is used to specify how the multiple requirements interact.

```json
"ALIGNMENT_LOGIC_SEP" : "<Logical Operator>"
```

#### IMAGE_BASE

This setting is used to specify what the base address of an image should be

```json
"IMAGE_BASE": <integer>
```

### Full Configuration File Example

```json
{   
    "IGNORE_LIST" : ["Shell.efi"],
    "IMAGE_FILE_MACHINE_AMD64" : {
        "DEFAULT" : {
            "DATA_CODE_SEPARATION": true,
            "ALIGNMENT_LOGIC_SEP": "OR",
            "ALIGNMENT" : [
                {
                    "COMPARISON" : "==",
                    "VALUE"      : 64
                },
                {
                    "COMPARISON" : "==",
                    "VALUE"      : 32
                }
            ]
        },
        "APPLICATION" : {
            "ALLOWED_SUBSYSTEMS" : ["EFI_APPLICATION"],
            "ALIGNMENT" : [
                {
                    "COMPARISON" : "==",
                    "VALUE"      : 64
                }
            ]
        },
        "UEFI_APPLICATION" : {
            "ALLOWED_SUBSYSTEMS" : ["EFI_APPLICATION"],
            "ALIGNMENT" : [
                {
                    "COMPARISON" : "==",
                    "VALUE"      : 32
                }
            ]
        }
    },
    "IMAGE_FILE_MACHINE_ARM" : {
        "DEFAULT": {"DATA_CODE_SEPARATION": true}
    },
    "IMAGE_FILE_MACHINE_ARM64" : {
        "DEFAULT": {"DATA_CODE_SEPARATION": true}
    },
    "IMAGE_FILE_MACHINE_I386" : {
        "DEFAULT" : {
            "DATA_CODE_SEPARATION": true,
            "ALIGNMENT" : [
                {
                    "COMPARISON" : ">=",
                    "VALUE"      : 4096
                }
            ]
        },
        "APPLICATION" : {
            "ALLOWED_SUBSYSTEMS" : ["EFI_APPLICATION"]
        }
    }  
}
```