## @file BuildReportEvaluationPlugin.yaml
#
# Plugin to evaluate a platform's build report file and verify common settings
#
#  Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from buildreport_table import *
from edk2toollib.database.edk2_db import Edk2DB

import logging
import os
import json
from sqlalchemy import text

try:
    from edk2toolext.environment.uefi_build import UefiBuilder
    from edk2toolext.environment.plugintypes.uefi_helper_plugin import IUefiHelperPlugin
    from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin

    class BuildReportEvaluationPlugin(IUefiBuildPlugin, IUefiHelperPlugin):
        class BuildReportEvaluationPlugin:
            """
            A plugin for evaluating build reports in the UEFI build process.
            This plugin implements the `IUefiBuildPlugin` and `IUefiHelperPlugin` interfaces
            to perform post-build evaluation of build reports using specified JSON queries.
            Methods:
                do_post_build(builder: UefiBuilder) -> int:
                    Executes the post-build evaluation process. It retrieves the build report
                    queries, parses the environment variables, initializes an in-memory database,
                    and evaluates the build report JSON data.
                    Args:
                        builder (UefiBuilder): The UEFI build object containing the build environment
                        and paths.
                    Returns:
                        int: Returns 0 to indicate successful execution.
            """

        def do_post_build(self, builder: UefiBuilder) -> int:
            queries = builder.env.GetValue(
                "BUILD_REPORT_EVALUATION_JSONS",
                [
                    os.path.join(
                        os.path.dirname(os.path.abspath(__file__)), "queries.json"
                    ),
                ],
            )

            queries_json = get_report_queries_json(builder.edk2path, queries)

            env = (
                builder.env.GetAllBuildKeyValues()
                | builder.env.GetAllNonBuildKeyValues()
            )
            db = generate_db(env, builder.edk2path)

            evaluate_build_report_json(db, queries_json)
            return 0

except ImportError:
    pass


def arguments() -> "Namespace":
    """
    Parse and return command-line arguments for the Build Report Evaluation Plugin.
    Returns:
        argparse.Namespace: Parsed command-line arguments with the following attributes:
            - WorkSpace (str): Absolute path to the workspace. Required.
            - build_report_file (str): Path to the build report file. Required.
            - RegenPackagePath (list): List of package paths to resolve relative paths
              when using --regenpath. Defaults to an empty list. Workspace is always included.
            - queries_json (list): List of paths to queries JSON files used to evaluate
              the build report. Defaults to a "queries.json" file in the same directory
              as this script.
    """
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "-w",
        "--workspace",
        dest="WorkSpace",
        required=True,
        type=str,
        help="""Specify the absolute path to your workspace by passing -w WORKSPACE or --workspace WORKSPACE.""",
    )
    parser.add_argument(
        "-b",
        "--build_report_file",
        dest="build_report_file",
        required=True,
        type=str,
        help="Provide the path to the build report file.",
    )
    parser.add_argument(
        "-p",
        "--packagepath",
        dest="RegenPackagePath",
        nargs="*",
        default=[],
        help="""Specify the packages path to be used to resolve relative paths when using --regenpath. ignored otherwise. Workspace is always included.""",
    )
    parser.add_argument(
        "-q",
        "--queries_json",
        dest="queries_json",
        nargs="*",
        default=[
            os.path.join(os.path.dirname(os.path.abspath(__file__)), "queries.json")
        ],
        help="""Specify the queries json file to be used to evaluate the build report. ignored otherwise.""",
    )
    arguments = parser.parse_args()
    arguments.WorkSpace = os.path.abspath(arguments.WorkSpace)

    return arguments


def generate_db(env: dict, pathtool: "Edk2Path") -> "Edk2DB":
    """
    Creates an in-memory database for processing build reports.

    Args:
        env (object): The environment object containing configuration and context for the build process.
        pathtool (object): A tool or utility for handling paths during the build process.

    Returns:
        Edk2DB: An instance of the Edk2DB class initialized with the provided environment and path tool.
    """
    db = Edk2DB(":memory:", env)
    db.register(BuildReportTable())
    db.pathobj = pathtool
    db.parse(env)
    return db


def get_report_queries_json(pathobj, json_files: list[str]) -> list:
    """
    Reads and aggregates JSON data from a list of file paths.
    Args:
        pathobj: Unused parameter, reserved for future use.
        json_files (list[str]): A list of file paths to JSON files.
    Returns:
        list: A list containing aggregated data from the JSON files.
    Notes:
        - If a file in the list does not exist, a warning is logged, and the file is skipped.
        - The function assumes that each JSON file contains a list of data that can be extended
          into the final result.
    Raises:
        JSONDecodeError: If a file contains invalid JSON.
    """
    data = []
    for json_file in json_files:
        if not os.path.exists(json_file):
            logging.warning(f"Query file {json_file} does not exist.")
            continue

        with open(json_file, "r") as file:
            data.extend(json.load(file))
    return data


def evaluate_build_report_json(db: "Edk2DB", queries: dict):
    """
    Evaluate build report JSON queries and log warnings for any results.

    Args:
        db (Database): The database object providing a session context.
        queries (list): A list of dictionaries, where each dictionary contains:
            - "query" (str): A string representation of a query to be evaluated.
            - "warning_message" (str): A warning message to log if the query produces results.

    Logs:
        Logs a warning message and the paths of results if the query produces any results.

    """
    with db.session() as session:
        for entry in queries:
            if "query" in entry:
                # Use a safer alternative to eval for executing queries
                query = entry["query"]
                results = session.execute(text(query)).all()
                if results:
                    logging.warning(f"{entry['warning_message']}")
                    for result in results:
                        logging.warning(f"\t{result.path}")
                    logging.debug(f"\t{entry['helpful_description']}")


if __name__ == "__main__":
    from edk2toollib.uefi.edk2.path_utilities import Edk2Path
    import argparse

    args = arguments()
    env = {"TARGET": "DEBUG", "BUILDREPORT_FILE": args.build_report_file}
    pathtool = Edk2Path(args.WorkSpace, args.RegenPackagePath)

    queries_json = get_report_queries_json(pathtool, args.queries_json)

    db = generate_db(env, pathtool)

    evaluate_build_report_json(db, queries_json)
