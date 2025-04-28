# BuildReportEvaluationPlugin

The `BuildReportEvaluationPlugin` is a tool designed to evaluate and generate reports for different build types in a project.
This plugin integrates with the stuart_build system `PlatformBuild` system to provide detailed insights for
each build, helping developers to identify issues and optimize the build process and prevent regression of settings.

## Features

- **Detailed Reports**: Generates build time warnings based on expected configurations.
- **Standalone Evaluation of Build Reports**: Can be run standalone against a Build report file.
- **Integration with PlatformBuild**: Build time warnings are configurable based on PlatformBuild settings.

## Configuration

1. In PlatformBuild.py, add `platform-checks` to the scopes.

2. (OPTIONAL) Supply the list of query files that should be used. If not specified, the default set of queries will be used.
It is recommend to add to the SetPlatformEnv function of `UefiBuilder` the overriding json list.
        self.env.SetValue("BUILD_REPORT_EVALUATION_JSONS", "<path to json1> <path to json2>", "Override Default jsons")

## Usage

## Use as part of stuart_build

Once `platform-checks` is in the scope, BuildReportEvaluationPlugin will automatically run.

## use as standalone application

Using as a standalone application requires the build report file to be passed in, as long as the list of packages paths
that the project was using as a space seperated list. Below is a example used on the Q35 reference project.
`
python MU_BASECORE\.pytool\Plugin\BuildReportEvaluation\BuildReportEvaluationPlugin.py -w . -b Build\QemuQ35Pkg\DEBUG_VS2022\BUILD_REPORT.TXT -p Platforms MU_BASECORE Common/MU Common/MU_TIANO Common/MU_OEM_SAMPLE Features/DEBUGGER Features/DFCI Features/CONFIG Features/MM_SUPV -q MU_BASECORE\.pytool\Plugin\BuildReportEvaluation\queries.json
`

## Expanding

Platforms can generate additonal queries that should be run by creating their own json file in the format of
    {
        "query": "<query to run against the database>",
        "warning_message": "<Warning message to be displayed when modules fail query>",
        "helpful_description": "<Message that describes why the query is important>"
    },

Queries are expected to be SQL, and buildreport_table can be examined to see the tables generated.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
