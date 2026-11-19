import os
import tempfile
import unittest
from unittest import mock

from Common.BuildToolError import FILE_CASE_MISMATCH, FILE_NOT_FOUND
from Common.DataType import TAB_COMMON
from Common import GlobalData
from CommonDataClass.DataClass import MODEL_EFI_INCLUDE
from Workspace.DecBuildData import DecBuildData


class DecIncludeTests(unittest.TestCase):
    def setUp(self):
        self.package = DecBuildData.__new__(DecBuildData)
        self.package._Clear()
        self.package.MetaFile = 'TestPkg/TestPkg.dec'
        self.package._PackageDir = 'TestPkg'
        self.package._Arch = TAB_COMMON
        self.package._RawData = {
            (MODEL_EFI_INCLUDE, TAB_COMMON): [
                ('Include', None, None, TAB_COMMON, '', 1, 12),
            ],
        }

    def check_include(self, error_code):
        include_path = mock.Mock()
        include_path.Validate.return_value = (error_code, 'TestPkg/Include')
        with mock.patch.object(DecBuildData, '_Macros', new_callable=mock.PropertyMock, return_value={}), \
                mock.patch('Workspace.DecBuildData.PathClass', return_value=include_path), \
                mock.patch('Workspace.DecBuildData.EdkLogger.warn') as warning, \
                mock.patch('Workspace.DecBuildData.EdkLogger.error', side_effect=RuntimeError) as error:
            if error_code not in (0, FILE_NOT_FOUND):
                with self.assertRaises(RuntimeError):
                    self.package.Includes
                error.assert_called_once()
                warning.assert_not_called()
            else:
                expected_includes = [] if error_code == FILE_NOT_FOUND else [include_path]
                self.assertEqual(expected_includes, self.package.Includes)
                self.assertEqual(expected_includes, self.package._CommonIncludes)
                self.assertEqual([], self.package._PrivateIncludes)
                error.assert_not_called()
                if error_code == FILE_NOT_FOUND:
                    warning.assert_called_once_with(
                        'build', 'DEC include directory does not exist; ignoring include path',
                        ExtraData='TestPkg/Include', File=self.package.MetaFile, Line=12,
                    )
                else:
                    warning.assert_not_called()

    def test_missing_include_warns_and_is_dropped(self):
        self.check_include(FILE_NOT_FOUND)

    def test_existing_include_does_not_warn(self):
        self.check_include(0)

    def test_other_validation_errors_remain_fatal(self):
        self.check_include(FILE_CASE_MISMATCH)

    def test_uninitialized_submodule_include_warns_and_build_data_continues(self):
        with tempfile.TemporaryDirectory() as package_dir:
            self.package._PackageDir = package_dir
            self.package._RawData[MODEL_EFI_INCLUDE, TAB_COMMON][0] = (
                'MissingSubmodule/Include', None, None, TAB_COMMON, 'PRIVATE', 1, 12,
            )
            os.mkdir(os.path.join(package_dir, 'Include'))
            self.package._RawData[MODEL_EFI_INCLUDE, TAB_COMMON].append(
                ('Include', None, None, TAB_COMMON, '', 2, 13),
            )
            with mock.patch.object(DecBuildData, '_Macros', new_callable=mock.PropertyMock, return_value={}), \
                    mock.patch.object(GlobalData, 'gAllFiles', {}), \
                    mock.patch('Common.Misc.mws.PACKAGES_PATH', []), \
                    mock.patch('Workspace.DecBuildData.EdkLogger.warn') as warning:
                includes = self.package.Includes
            self.assertEqual(1, len(includes))
            self.assertEqual(os.path.join(package_dir, 'Include'), str(includes[0]))
            self.assertEqual([], self.package._PrivateIncludes)
            self.assertEqual(includes, self.package._CommonIncludes)
            warning.assert_called_once()


def TheTestSuite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(DecIncludeTests)


if __name__ == '__main__':
    unittest.main()