# ROM UEFI Variable Patcher Build Plugin

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About

This plugin can be used to patch UEFI variables in a ROM image in post-build from varriable information provided in
an XML file.

### UEFI Build Plugin

You must add the `uefivarpatcher` scope to you build configuration to enable this plugin.

The plugin operates in the `do_post_build function`.  This plugin uses the following variables from the build
environment:

 1. `VAR_PATCH_CONFIG_DIR` - [**REQUIRED**] - Package that contains the config file (`BuiltInVars.xml`)
 2. `FLASH_SIZE` - [**REQUIRED**] - Total flash image size.
 3. `FLASH_REGION_NVSTORAGE_OFFSET` - [**REQUIRED**] - Offset in bytes to the NV storage region.
 4. `FLASH_REGION_NVSTORAGE_SIZE` - [**REQUIRED**] - Size in bytes of the NV storage region.
 5. `OUTPUT_ROM` - [**REQUIRED**] - Output ROM file path. Multiple ROM files can be semicolon separated.

The variable configuration data must be provided in an XML file named `BuiltInVars.xml`. If a variable in the XML file
does not exist in the ROM image, the script will attempt to create it.

### XML File Example

```xml
<?xml version="1.0"?>
<Variables>
    <Variable>
        <Name>Variable1</Name>
        <GUID>12345678-1234-5678-1234-567812345678</GUID>
        <Attributes>0x0000000000000001</Attributes>
        <Data type="hex">12345678</Data>
    </Variable>
    <Variable>
        <Name>Variable2</Name>
        <GUID>87654321-4321-6789-4321-678943216789</GUID>
        <Attributes>0x0000000000000002</Attributes>
        <Data type="hex">87654321</Data>
    </Variable>
</Variables>
```
