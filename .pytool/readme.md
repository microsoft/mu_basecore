# Edk2 Continuous Integration

## Basic Status

| Package | Windows VS2019 | Ubuntu GCC5 | Known Issues |
| :----   | :-----         | :----       | :---         |
|ArmPkg | | |
|ArmPlatformPkg |||
|ArmVirtPkg |||
|CryptoPkg | :heavy_check_mark: | :heavy_check_mark:  | New changes for NULL basecryptlib and tlslib, add IntrinsicLib.inf to dsc|
|DynamicTablesPkg |||
|EmbeddedPkg |||
|EmulatorPkg |||
|FatPkg|:heavy_check_mark:|:heavy_check_mark:||
|FmpDevicePkg|:heavy_check_mark:|:heavy_check_mark:||
|IntelFsp2Pkg|||
|IntelFsp2WrapperPkg|||
| MdeModulePkg | :heavy_check_mark: |:heavy_check_mark: |DxeIpl dependency on ArmPkg, Missing Visual Studio AARCH64/ARM support for EBC |
|MdePkg | :heavy_check_mark: |:heavy_check_mark: | Update DSC to add UefiFileHandleLib, Update BaseIoLibIntrinsic to support MSFT ARM/AARCH64|
|NetworkPkg | :heavy_check_mark: |:heavy_check_mark:| Libraries missing from components section for package dsc|
|OvmfPkg |||
|PcAtChipsetPkg | :heavy_check_mark: |:heavy_check_mark:|
|SecurityPkg|:heavy_check_mark:|:heavy_check_mark:| Incorrect libraryclass in dec|
|ShellPkg | :heavy_check_mark: |:heavy_check_mark:||
|SignedCapsulePkg|||
|SourceLevelDebugPkg|||
|StandaloneMmPkg|||
|UefiCpuPkg | :heavy_check_mark: |:heavy_check_mark: ||
|UefiPayloadPkg|||

For more detailed status look at the test results of the latest CI run on the repo readme.

## Background

While a number of CI solutions exist, this proposal will focus on the usage of Azure Dev Ops and Build Pipelines. For demonstration, a sample [TianoCore repo](https://github.com/tianocore/edk2-staging.git) (branch edk2-ci) and [Dev Ops Pipeline](https://dev.azure.com/tianocore/edk2-ci-play/_build?definitionId=14) have been set up.

Furthermore, this proposal will leverage the TianoCore python tools PIP modules: [library](https://pypi.org/project/edk2-pytool-library/) and [extensions](https://pypi.org/project/edk2-pytool-extensions/) (with repos located [here](https://github.com/tianocore/edk2-pytool-library) and [here](https://github.com/tianocore/edk2-pytool-extensions)).

The primary execution flows can be found in the `ci/AzurePipelines/Windows-VS2019.yml` and `ci/AzurePipelines/Ubuntu-GCC5.yml` files. These YAML files are consumed by the Azure Dev Ops Build Pipeline and dictate what server resources should be used, how they should be configured, and what processes should be run on them. An overview of this schema can be found [here](https://docs.microsoft.com/en-us/azure/devops/pipelines/yaml-schema?view=azure-devops&tabs=schema).

Inspection of these files reveals the EDKII Tools commands that make up the primary processes for the CI build: 'stuart_setup', 'stuart_update', and 'stuart_ci_build'. These commands come from the EDKII Tools PIP modules and are configured as described below. More documentation on the stuart tools can be found [here](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/using.md) and [here](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/features/feature_invocables.md).

## Configuration

Configuration of the CI process consists of (in order of precedence):

* command-line arguments passed in via the Pipeline YAML
* a per-package configuration file (e.g. `<package-name>.ci.yaml`) that is detected by the CI system in EDKII Tools.
* a global configuration Python module (e.g. `CISetting.py`) passed in via the command-line

The global configuration file is described in [this readme](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/usability/using_settings_manager.md) from the EDKII Tools documentation. This configuration is written as a Python module so that decisions can be made dynamically based on command line parameters and codebase state.

The per-package configuration file can override most settings in the global configuration file, but is not dynamic. This file can be used to skip or customize tests that may be incompatible with a specific package. By default, the global configuration will try to run all tests on all packages.

## Current PyTool Test Capabilities

All CI tests are instances of EDKII Tools plugins. Documentation on the plugin system can be found [here](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/usability/using_plugin_manager.md) and [here](https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/features/feature_plugin_manager.md). Upon invocation, each plugin will be passed the path to the current package under test and a dictionary containing its targeted configuration, as assembled from the command line, per-package configuration, and global configuration.

Note: CI plugins are considered unique from build plugins and helper plugins, even though some CI plugins may execute steps of a build.

In the example, these plugins live alongside the code under test (in the `ci/Plugin` directory), but may be moved to the 'edk2-test' repo if that location makes more sense for the community.

### Module Inclusion Test - DscCompleteCheck

This test scans all available modules (via INF files) and compares them to the package-level DSC file for the package each module is contained within. The test considers it an error if any module does not appear in the `Components` section of at least one package-level DSC (indicating that it would not be built if the package were built).

### Code Compilation Test - CompilerPlugin

Once the Module Inclusion Test has verified that all modules would be built if all package-level DSCs were built, the Code Compilation Test simply runs through and builds every package-level DSC on every toolchain and for every architecture that is supported. Any module that fails to build is considered an error.

### GUID Uniqueness Test - GuidCheck

This test works on the collection of all packages rather than an individual package. It looks at all FILE_GUIDs and GUIDs declared in DEC files and ensures that they are unique for the codebase. This prevents, for example, accidental duplication of GUIDs when using an existing INF as a template for a new module.

### Cross-Package Dependency Test - DependencyCheck

This test compares the list of all packages used in INFs files for a given package against a list of "allowed dependencies" in plugin configuration for that package. Any module that depends on a disallowed package will cause a test failure.

### Library Declaration Test - LibraryClassCheck

This test scans at all library header files found in the `Library` folders in all of the package's declared include directories and ensures that all files have a matching LibraryClass declaration in the DEC file for the package. Any missing declarations will cause a failure.

### Invalid Character Test - CharEncodingCheck

This test scans all files in a package to make sure that there are no invalid Unicode characters that may cause build errors in some character sets/localizations.

### Spell Checking - cspell

This test runs a spell checker on all files within the package.  This is done using the NodeJs cspell tool.  For details check `ci/Plugin/SpellCheck`.  For this plugin to run during ci you must install nodejs and cspell and have both available to the command line when running your CI.

Install

* Install nodejs from https://nodejs.org/en/
* Install cspell
  1. Open cmd prompt with access to node and npm
  2. Run `npm install -g cspell`

  More cspell info: https://github.com/streetsidesoftware/cspell

## Current Azure Pipeline Tests

When adding a test it can be added as either a *PyTool* test or just added to the CI build process.  This should be a deliberate choice.  Any change added as a pipeline test is not as easily run on a private/local workspace.  But there are times where this is still the preferred method.

## PyTool Scopes

Scopes are how the PyTool ext_dep, path_env, and plugins are activated.  Meaning that if an invocable process has a scope active then those ext_dep and path_env will be active. To allow easy integration of PyTools capabilities there are a few standard scopes.

| Scope   | Invocable | Description    |
| :----   | :-----    | :----          |
| global  | edk2_invocable++ - should be base_abstract_invocable       | Running an invocables |
| global-win | edk2_invocable++ | Running on Microsoft Windows |
| global-nix | edk2_invocable++ | Running on Linux based OS    |
| edk2-build |  | This indicates that an invocable is building EDK2 based UEFI code |
| cibuild | set in .pytool/CISettings.py | Suggested target for edk2 continuous integration builds.  Tools used for CiBuilds can use this scope.  Example: asl compiler |

## Future investments

* PatchCheck tests as plugins
* MacOS/xcode support
* Clang/LLVM support
* Visual Studio AARCH64 and ARM support
* BaseTools C tools CI/PR and binary release process
* BaseTools Python tools CI/PR process
* Host based unit testing
* Extensible private/closed source platform reporting
* Platform builds, validation
* UEFI SCTs
* Other automation
