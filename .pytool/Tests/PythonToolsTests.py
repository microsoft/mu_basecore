# @file
# Unit tests for Python tools and plugins in .pytool.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

"""Assemble the explicitly supported Python tool unit-test suites."""

import unittest

import test_ImageValidation


def the_test_suite() -> unittest.TestSuite:
    """Include the ImageValidation regression tests."""
    suites = [
        unittest.defaultTestLoader.loadTestsFromModule(test_ImageValidation)
    ]
    return unittest.TestSuite(suites)


if __name__ == "__main__":
    result = unittest.TextTestRunner(verbosity=2).run(the_test_suite())
    raise SystemExit(0 if result.wasSuccessful() else 1)
