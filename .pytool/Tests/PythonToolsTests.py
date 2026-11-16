# @file
# Unit tests for Python tools and plugins in .pytool.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

"""Assemble the explicitly supported Python tool unit-test suites."""

import unittest
from pathlib import Path

import test_ImageValidation
import test_RustHostUnitTestPlugin


def the_test_suite() -> unittest.TestSuite:
    """Include the supported plugin regression tests."""
    helper_tests = Path(__file__).resolve().parents[1] / "Plugin" / "RustPackageHelper" / "tests"
    suites = [
        unittest.defaultTestLoader.loadTestsFromModule(test_ImageValidation),
        unittest.defaultTestLoader.loadTestsFromModule(test_RustHostUnitTestPlugin),
        unittest.TestLoader().discover(start_dir=str(helper_tests)),
    ]
    return unittest.TestSuite(suites)


if __name__ == "__main__":
    result = unittest.TextTestRunner(verbosity=2).run(the_test_suite())
    raise SystemExit(0 if result.wasSuccessful() else 1)
