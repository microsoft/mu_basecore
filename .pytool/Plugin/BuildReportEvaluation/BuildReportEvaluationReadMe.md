# BuildReportEvaluationPlugin

The `BuildReportEvaluationPlugin` is a tool designed to evaluate and generate reports for different build types in a project.
This plugin integrates with the stuart_build system `PlatformBuild` system to provide detailed insights and metrics for
each build type, helping developers to identify issues and optimize the build process.

## Features

- **Build Type Evaluation**: Supports evaluation of multiple build items.
- **Detailed Reports**: Generates build time warnings based on expected configurations.
- **Integration with PlatformBuild**: Build time warnings are configurable based on PlatformBuild settings.

## Configuration

1. In PlatformBuild.py, add `platform-checks` to the scopes.

2. (OPTIONAL) Modify the queries that should be run by setting environment variable EVALUATION_QUERIES.
    The default set of queries are
        - Build - Ensure compiled components are actually included in FV. The data from this can be used to speed up the
                  build process by removing unused drivers.
        - Exclude - Ensure EBC, UiApp, SetupDxe, HddPassword are not included in any FV.
        - Legacy - Ensure legacy drivers, such as Ps2Keyboard and Ps2Mouse are not included. Ensure Uga support is not enabled.
        - Performance - Ensure that Performance measurement drivers (Non-Null versions) are not included in the output image.
        - Stack - Ensure that all Modules (excluding applications and USER_DEFINED) have a stack check lib linked.

## Usage

Once `platform-checks` is in the scope, BuildReportEvaluationPlugin will automatically run.

## Expanding

Platforms can generate additional checks that will run as part of this. They should create a
<name>_plug_in.yaml file similar to BuildReportEvaluation_plug_in.yaml/ The class needs to
have derive from `IUefiHelperPlugin` and include a RegisterHelpers function. The RegisterHelpers
function should register the additonal functions that are platform specific, and the
BuildReportEvaluationPlugin will automically pick those functions up and run them post build.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
