## @file BuildReportEvaluation_plug_in.yaml
#
# Plugin to evaluate a platform's build report file and verify common settings
#
#  Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toolext.environment.plugintypes.uefi_helper_plugin import IUefiHelperPlugin
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin

from buildreport_table import BuildReportTable

from BuildReportQueries import *
import logging
import os


class BuildReportEvaluationPlugin(IUefiBuildPlugin, IUefiHelperPlugin):
    def RegisterHelpers(self, obj: "HelperFunctions") -> None:
        fp = os.path.abspath(__file__)
        obj.Register(
            "Build_CompiledButButNotIncludedInAnyFV",
            CompiledButButNotIncludedInAnyFV,
            fp,
        )

        obj.Register("Exclude_SetupDxeExistsInFV", SetupDxeExistsInFV, fp)
        obj.Register("Exclude_HddPasswordDxeExistsInFV", HddPasswordDxeExistsInFV, fp)
        obj.Register("Exclude_UiAppExistsInFV", UiAppExistsInFV, fp)
        obj.Register("Exclude_EbcDxeExistsInFV", EbcDxeExistsInFV, fp)

        obj.Register(
            "Legacy_PcdConOutUgaSupportIsNotZero", PcdConOutUgaSupportIsNotZero, fp
        )
        obj.Register(
            "Legacy_PcdTurnOffUsbLegacySupportIsNotOne",
            PcdTurnOffUsbLegacySupportIsNotOne,
            fp,
        )
        obj.Register("Legacy_Ps2UsedInFv", Ps2UsedInFv, fp)

        obj.Register(
            "Performance_FailedPerformanceLibUsingNonNullLib",
            FailedPerformanceLibUsingNonNullLib,
            fp,
        )

        obj.Register(
            "Stack_ModuleDoesNotContainStackCheckLib",
            ModuleDoesNotContainStackCheckLib,
            fp,
        )

        return 0

    def do_post_build(self, builder: UefiBuilder) -> int:
        error_count = 0

        queries = builder.env.GetValue("EVALUATION_QUERIES", "Build,Exclude,Legacy,Performance,Stack")
        if queries == "":
            return error_count

        env = builder.env.GetAllBuildKeyValues() | builder.env.GetAllNonBuildKeyValues()

        db = Edk2DB(":memory:", builder.edk2path)
        db.register(BuildReportTable())
        db.parse(env)

        for query in queries.split(","):
            for fn_name in builder.Helper.RegisteredFunctions.keys():
                if fn_name.startswith(query.strip()):
                    logging.info(f"Running query: {fn_name}")
                    method = getattr(builder.Helper, fn_name)
                    if callable(method):
                        error_count += method(db, "warning")

        return error_count
