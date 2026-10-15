# @file
# Unit tests for the ImageValidation build plugin.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

"""Regression tests for ImageValidation profiles and path handling."""

import importlib.util
import tempfile
import unittest
from contextlib import ExitStack
from copy import deepcopy
from pathlib import Path
from unittest.mock import Mock, call, patch

# Load the plugin directly without installing it or modifying sys.path.
plugin_path = (
    Path(__file__).resolve().parents[1]
    / "Plugin"
    / "ImageValidation"
    / "ImageValidation.py"
)
spec = importlib.util.spec_from_file_location("ImageValidation", plugin_path)
if spec is None or spec.loader is None:
    raise ImportError(f"Cannot load ImageValidation plugin from {plugin_path}")
image_validation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(image_validation)


class ImageValidationTestCase(unittest.TestCase):
    """Provide an isolated filesystem and plugin instance for each test."""

    def setUp(self) -> None:
        """Create temporary files and register cleanup for patched dependencies."""
        temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        self.root = Path(temporary_directory.name)
        self.contexts = ExitStack()
        self.addCleanup(self.contexts.close)
        self.plugin = image_validation.ImageValidation()


class TestMakefileProfile(ImageValidationTestCase):
    """Verify profile extraction from generated module Makefiles."""

    def setUp(self) -> None:
        """Create an EFI output file beneath a module build directory."""
        super().setUp()
        self.efi_path = self.root / "OUTPUT" / "Example.efi"
        self.efi_path.parent.mkdir()
        self.efi_path.touch()

    def test_reads_supported_makefile_names(self) -> None:
        """Read module metadata from either supported Makefile name."""
        for filename in ("GNUmakefile", "Makefile"):
            with self.subTest(filename=filename):
                makefile = self.root / filename
                makefile.write_text(
                    "MODULE_NAME = Example\nMODULE_TYPE = DXE_DRIVER\n",
                    encoding="utf-8",
                )
                self.assertEqual(
                    self.plugin._get_profile_from_makefile(self.efi_path), "DXE_DRIVER"
                )
                makefile.unlink()

    def test_accepts_whitespace_comments_and_crlf(self) -> None:
        """Accept generated metadata with comments and Windows line endings."""
        (self.root / "GNUmakefile").write_bytes(
            b"# Generated metadata\r\n\tMODULE_NAME\t=\tExample  # name\r\n"
            b" MODULE_TYPE = DXE_RUNTIME_DRIVER\t# type\r\n"
        )
        self.assertEqual(
            self.plugin._get_profile_from_makefile(self.efi_path), "DXE_RUNTIME_DRIVER"
        )

    def test_prefers_gnumakefile(self) -> None:
        """Prefer GNUmakefile when both files describe the target module."""
        for filename, profile in (
            ("GNUmakefile", "DXE_DRIVER"),
            ("Makefile", "UEFI_APPLICATION"),
        ):
            (self.root / filename).write_text(
                f"MODULE_NAME = Example\nMODULE_TYPE = {profile}\n", encoding="utf-8"
            )
        self.assertEqual(
            self.plugin._get_profile_from_makefile(self.efi_path), "DXE_DRIVER"
        )

    def test_falls_back_to_matching_makefile(self) -> None:
        """Use Makefile when GNUmakefile has mismatched or incomplete metadata."""
        (self.root / "Makefile").write_text(
            "MODULE_NAME = Example\nMODULE_TYPE = UEFI_APPLICATION\n", encoding="utf-8"
        )
        for metadata in (
            "MODULE_NAME = DifferentModule\nMODULE_TYPE = DXE_DRIVER\n",
            "MODULE_NAME = Example\n",
            "MODULE_TYPE = DXE_DRIVER\n",
            "MODULE_NAME = Example\nMODULE_TYPE =\n",
        ):
            with self.subTest(metadata=metadata):
                (self.root / "GNUmakefile").write_text(metadata, encoding="utf-8")
                self.assertEqual(
                    self.plugin._get_profile_from_makefile(self.efi_path),
                    "UEFI_APPLICATION",
                )

    def test_missing_makefiles_return_none(self) -> None:
        """Leave the profile unresolved when no Makefile exists."""
        self.assertIsNone(self.plugin._get_profile_from_makefile(self.efi_path))

    def test_rejects_unmatched_or_incomplete_metadata(self) -> None:
        """Require a matching module name and a literal nonempty module type."""
        metadata_cases = (
            "",
            "MODULE_NAME = DifferentModule\nMODULE_TYPE = DXE_DRIVER\n",
            "MODULE_NAME = Example\n",
            "MODULE_TYPE = DXE_DRIVER\n",
            "MODULE_NAME = Example\nMODULE_TYPE =\n",
            "# MODULE_NAME = Example\nMODULE_TYPE = DXE_DRIVER\n",
            "MODULE_NAME = Example\nMODULE_TYPE = $(OTHER_TYPE)\n",
        )
        for metadata in metadata_cases:
            with self.subTest(metadata=metadata):
                (self.root / "GNUmakefile").write_text(metadata, encoding="utf-8")
                self.assertIsNone(self.plugin._get_profile_from_makefile(self.efi_path))

    def test_does_not_use_makefile_in_output_directory(self) -> None:
        """Look for metadata in the module directory rather than OUTPUT."""
        (self.efi_path.parent / "Makefile").write_text(
            "MODULE_NAME = Example\nMODULE_TYPE = DXE_DRIVER\n", encoding="utf-8"
        )
        self.assertIsNone(self.plugin._get_profile_from_makefile(self.efi_path))


class TestDirectoryWalking(ImageValidationTestCase):
    """Verify recursive file discovery and case-insensitive filtering."""

    def setUp(self) -> None:
        """Create nested EFI files and an unrelated text file."""
        super().setUp()
        (self.root / "OUTPUT").mkdir()
        self.paths = [
            self.root / "Main.EFI",
            self.root / "skipDriver.efi",
            self.root / "OUTPUT" / "Nested.efi",
        ]
        for path in self.paths:
            path.touch()
        (self.root / "notes.txt").touch()

    def test_string_and_path_directories_return_recursive_string_paths(self) -> None:
        """Accept string and Path inputs while returning filtered string paths."""
        for directory in (str(self.root), self.root):
            for ignorelist in (None, [], ["SKIP"]):
                with self.subTest(directory=directory, ignorelist=ignorelist):
                    result = self.plugin._walk_directory_for_extension(
                        [".eFi"], directory, ignorelist
                    )
                    expected = [
                        str(path)
                        for path in self.paths
                        if not ignorelist or not path.name.lower().startswith("skip")
                    ]
                    self.assertCountEqual(result, expected)
                    self.assertTrue(all(isinstance(path, str) for path in result))

    def test_empty_or_unmatched_extensions_return_empty_list(self) -> None:
        """Return no files when no extension matches."""
        for extensions in ([], [".bin"]):
            with self.subTest(extensions=extensions):
                self.assertEqual(
                    self.plugin._walk_directory_for_extension(extensions, self.root), []
                )

    def test_invalid_directories_raise(self) -> None:
        """Log and reject relative paths, missing directories, and files."""
        for directory in ("relative", self.root / "missing", self.paths[0]):
            with (
                self.subTest(directory=directory),
                self.assertLogs(level="CRITICAL"),
                self.assertRaises(ValueError),
            ):
                self.plugin._walk_directory_for_extension([".efi"], directory)

    def test_invalid_argument_types_raise(self) -> None:
        """Reject invalid extension lists, directories, and ignore lists."""
        for arguments in (
            (".efi", self.root, None),
            ([".efi"], None, None),
            ([".efi"], self.root, "skip"),
        ):
            with (
                self.subTest(arguments=arguments),
                self.assertLogs(level="CRITICAL"),
                self.assertRaises(TypeError),
            ):
                self.plugin._walk_directory_for_extension(*arguments)


class TestVariableResolution(ImageValidationTestCase):
    """Verify substitution of environment and build variables in paths."""

    def test_resolves_environment_and_build_variables(self) -> None:
        """Preserve empty values and use build variables only as a fallback."""
        builder = Mock(spec=image_validation.UefiBuilder)
        builder.env = Mock()
        builder.env.GetValue.side_effect = {"ROOT": "/workspace", "EMPTY": ""}.get
        builder.env.GetBuildValue.side_effect = {
            "ARCH": "X64",
            "EMPTY": "not-empty",
        }.get

        cases = (
            ("plain.efi", "plain.efi"),
            ("$(ROOT)/$(ARCH)/image.efi", "/workspace/X64/image.efi"),
            ("$(EMPTY)image.efi", "image.efi"),
            ("$(ROOT)/$(ROOT)", "/workspace//workspace"),
        )
        for text, expected in cases:
            with self.subTest(text=text):
                self.assertEqual(self.plugin._resolve_vars(builder, text), expected)
        builder.env.GetBuildValue.assert_called_once_with("ARCH")

    def test_undefined_variable_raises_descriptive_error(self) -> None:
        """Reject variables missing from both sources instead of a replace error."""
        builder = Mock(spec=image_validation.UefiBuilder)
        builder.env = Mock()
        builder.env.GetValue.return_value = None
        builder.env.GetBuildValue.return_value = None
        with self.assertRaisesRegex(ValueError, r"Undefined variable.*MISSING"):
            self.plugin._resolve_vars(builder, "$(MISSING)/image.efi")
        builder.env.GetBuildValue.assert_called_once_with("MISSING")


class TestImageBase(unittest.TestCase):
    """Verify all outcomes of the plugin's image-base requirement."""

    def setUp(self) -> None:
        self.check = image_validation.TestImageBase()
        self.pe = Mock()
        self.pe.OPTIONAL_HEADER.ImageBase = 0

    def test_unspecified_or_disabled_requirement_skips(self) -> None:
        for requirements in ({}, {"IMAGE_BASE": None}, {"IMAGE_BASE": -1}):
            with self.subTest(requirements=requirements):
                self.assertEqual(
                    self.check.execute(
                        Mock(spec=[]), {"TARGET_REQUIREMENTS": requirements}
                    ),
                    image_validation.Result.SKIP,
                )

    def test_matching_image_base_passes(self) -> None:
        for base in (0, 0x100000):
            with self.subTest(base=base):
                self.pe.OPTIONAL_HEADER.ImageBase = base
                self.assertEqual(
                    self.check.execute(
                        self.pe, {"TARGET_REQUIREMENTS": {"IMAGE_BASE": base}}
                    ),
                    image_validation.Result.PASS,
                )

    def test_mismatched_image_base_fails(self) -> None:
        self.pe.OPTIONAL_HEADER.ImageBase = 0x100000
        with self.assertLogs(level="ERROR"):
            self.assertEqual(
                self.check.execute(
                    self.pe, {"TARGET_REQUIREMENTS": {"IMAGE_BASE": 0}}
                ),
                image_validation.Result.FAIL,
            )

    def test_missing_image_base_or_header_warns(self) -> None:
        for missing_header in (False, True):
            with self.subTest(missing_header=missing_header):
                pe = Mock()
                if missing_header:
                    del pe.OPTIONAL_HEADER
                else:
                    del pe.OPTIONAL_HEADER.ImageBase
                with self.assertLogs(level="WARNING"):
                    self.assertEqual(
                        self.check.execute(
                            pe, {"TARGET_REQUIREMENTS": {"IMAGE_BASE": 0}}
                        ),
                        image_validation.Result.WARN,
                    )


class TestConfigurationMerge(unittest.TestCase):
    """Verify architecture and profile overrides without dropping defaults."""

    def setUp(self) -> None:
        self.default = image_validation.yaml.safe_load(
            image_validation.DEFAULT_CONFIG_FILE_PATH.read_text(encoding="utf-8")
        )

    def test_empty_override_preserves_defaults(self) -> None:
        self.assertEqual(
            image_validation.ImageValidation.merge_config(deepcopy(self.default), {}),
            self.default,
        )

    def test_profile_overrides_preserve_other_profiles_and_architectures(self) -> None:
        for alias, architecture in self.default["TARGET_ARCH"].items():
            for key in (alias, architecture):
                with self.subTest(key=key):
                    override = {"IMAGE_BASE": 0x100000}
                    expected = deepcopy(self.default)
                    expected[architecture]["DEFAULT"] = override
                    result = image_validation.ImageValidation.merge_config(
                        deepcopy(self.default), {key: {"DEFAULT": override}}
                    )
                    self.assertEqual(result, expected)

    def test_canonical_architecture_key_takes_precedence_over_alias(self) -> None:
        result = image_validation.ImageValidation.merge_config(
            deepcopy(self.default),
            {
                "IMAGE_FILE_MACHINE_AMD64": {"DEFAULT": {"IMAGE_BASE": 0x100000}},
                "X64": {"DEFAULT": {"IMAGE_BASE": 0x200000}},
            },
        )
        self.assertEqual(
            result["IMAGE_FILE_MACHINE_AMD64"]["DEFAULT"], {"IMAGE_BASE": 0x100000}
        )

    def test_top_level_overrides_replace_defaults(self) -> None:
        provided = {
            "TARGET_ARCH": {"X64": "IMAGE_FILE_MACHINE_AMD64"},
            "IGNORE_LIST": ["**/Ignored*.efi"],
        }
        expected = deepcopy(self.default)
        expected.update(provided)
        self.assertEqual(
            image_validation.ImageValidation.merge_config(self.default, provided),
            expected,
        )


class TestImageProfiles(ImageValidationTestCase):
    """Verify PE profile selection and validation result propagation."""

    def setUp(self) -> None:
        """Prepare a mock PE image, architecture profiles, and test manager."""
        super().setUp()
        self.pe = Mock()
        self.pe.FILE_HEADER.Machine = 0x8664
        self.plugin.config_data = {
            "IMAGE_FILE_MACHINE_AMD64": {
                "DEFAULT": {"IMAGE_BASE": 0},
                "DXE_DRIVER": {"IMAGE_BASE": 0},
                "DXE_RUNTIME_DRIVER": {},
            }
        }
        self.run_tests = self.contexts.enter_context(
            patch.object(self.plugin.test_manager, "run_tests")
        )
        self.run_tests.return_value = image_validation.Result.PASS
        self.parse_pe = self.contexts.enter_context(
            patch.object(image_validation, "PE", return_value=self.pe)
        )

    def test_explicit_profile_is_preserved(self) -> None:
        """Pass an explicitly configured profile to the test manager."""
        result = self.plugin._validate_image("image.efi", "DXE_DRIVER")
        self.assertEqual(result, image_validation.Result.PASS)
        self.parse_pe.assert_called_once_with("image.efi", fast_load=True)
        self.run_tests.assert_called_once_with(self.pe, "DXE_DRIVER")

    def test_detects_supported_subsystem_profiles(self) -> None:
        """Map each supported EFI subsystem to its module profile."""
        cases = (
            ("IMAGE_SUBSYSTEM_EFI_APPLICATION", "UEFI_APPLICATION"),
            ("IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER", "DXE_DRIVER"),
            ("IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER", "DXE_RUNTIME_DRIVER"),
        )
        for subsystem, profile in cases:
            with self.subTest(subsystem=subsystem):
                self.pe.OPTIONAL_HEADER.Subsystem = image_validation.SUBSYSTEM_TYPE[
                    subsystem
                ]
                self.assertEqual(
                    self.plugin._try_get_supported_subsystem(self.pe), profile
                )

    def test_missing_optional_header_returns_no_profile(self) -> None:
        """Leave the profile unresolved when the PE optional header is absent."""
        # Deletion forces AttributeError; otherwise Mock creates missing attributes.
        del self.pe.OPTIONAL_HEADER
        self.assertIsNone(self.plugin._try_get_supported_subsystem(self.pe))

    def test_uses_subsystem_when_profile_is_omitted(self) -> None:
        """Infer the profile from the PE subsystem when none is supplied."""
        self.pe.OPTIONAL_HEADER.Subsystem = image_validation.SUBSYSTEM_TYPE[
            "IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER"
        ]
        self.plugin._validate_image("image.efi")
        self.run_tests.assert_called_once_with(self.pe, "DXE_DRIVER")

    def test_unsupported_subsystem_uses_default_profile(self) -> None:
        """Use DEFAULT for a recognized non-EFI subsystem."""
        self.pe.OPTIONAL_HEADER.Subsystem = image_validation.SUBSYSTEM_TYPE[
            "IMAGE_SUBSYSTEM_WINDOWS_CUI"
        ]
        self.plugin._validate_image("image.efi")
        self.run_tests.assert_called_once_with(self.pe, "DEFAULT")

    def test_unknown_numeric_subsystem_uses_default_profile(self) -> None:
        """Use DEFAULT even when pefile has no name for the subsystem."""
        self.pe.OPTIONAL_HEADER.Subsystem = 0xFFFF
        self.assertIsNone(self.plugin._try_get_supported_subsystem(self.pe))
        self.assertEqual(
            self.plugin._validate_image("image.efi"), image_validation.Result.PASS
        )
        self.run_tests.assert_called_once_with(self.pe, "DEFAULT")

    def test_empty_profile_configuration_uses_default(self) -> None:
        """Fall back to DEFAULT when the selected profile has no requirements."""
        self.plugin._validate_image("image.efi", "DXE_RUNTIME_DRIVER")
        self.run_tests.assert_called_once_with(self.pe, "DEFAULT")

    def test_returns_test_manager_result(self) -> None:
        """Propagate all test-manager results without conversion."""
        for result in (
            image_validation.Result.PASS,
            image_validation.Result.WARN,
            image_validation.Result.FAIL,
            image_validation.Result.SKIP,
        ):
            with self.subTest(result=result):
                self.run_tests.return_value = result
                self.assertEqual(
                    self.plugin._validate_image("image.efi", "DXE_DRIVER"), result
                )

    def test_parse_failure_fails_validation(self) -> None:
        """Report a failed PE parse without running image tests."""
        self.parse_pe.side_effect = OSError("Cannot read image")
        with self.assertLogs(level="ERROR"):
            self.assertEqual(
                self.plugin._validate_image("image.efi"), image_validation.Result.FAIL
            )
        self.run_tests.assert_not_called()


class TestValidationIntegration(ImageValidationTestCase):
    """Exercise registered checks and profile handling with the real manager."""

    def setUp(self) -> None:
        super().setUp()
        self.plugin.config_data = image_validation.yaml.safe_load(
            image_validation.DEFAULT_CONFIG_FILE_PATH.read_text(encoding="utf-8")
        )
        self.plugin.test_manager.config_data = self.plugin.config_data
        self.pe = Mock()
        self.pe.FILE_HEADER.Machine = image_validation.MACHINE_TYPE[
            "IMAGE_FILE_MACHINE_AMD64"
        ]
        self.pe.OPTIONAL_HEADER.ImageBase = 0
        self.pe.OPTIONAL_HEADER.SectionAlignment = 0x1000
        self.pe.OPTIONAL_HEADER.Subsystem = image_validation.SUBSYSTEM_TYPE[
            "IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER"
        ]
        self.section = Mock()
        self.section.Name = b".text"
        self.section.Characteristics = 0x20000000
        self.pe.sections = [self.section]
        self.contexts.enter_context(
            patch.object(image_validation, "PE", return_value=self.pe)
        )

    def test_valid_image_and_empty_profile_pass(self) -> None:
        for profile in ("DEFAULT", "DXE_DRIVER", None):
            with self.subTest(profile=profile):
                self.assertEqual(
                    self.plugin._validate_image("image.efi", profile),
                    image_validation.Result.PASS,
                )

    def test_each_registered_requirement_can_fail_validation(self) -> None:
        """Catch missing registration as well as disabled individual checks."""
        for base, alignment, characteristics in (
            (0x100000, 0x1000, 0x20000000),
            (0, 0x2000, 0x20000000),
            (0, 0x1000, 0xA0000000),
        ):
            with self.subTest(
                base=base, alignment=alignment, characteristics=characteristics
            ):
                self.pe.OPTIONAL_HEADER.ImageBase = base
                self.pe.OPTIONAL_HEADER.SectionAlignment = alignment
                self.section.Characteristics = characteristics
                with self.assertLogs(level="ERROR"):
                    self.assertEqual(
                        self.plugin._validate_image("image.efi", "DEFAULT"),
                        image_validation.Result.FAIL,
                    )

    def test_absent_profile_is_rejected(self) -> None:
        with self.assertLogs(level="ERROR") as logs:
            self.assertEqual(
                self.plugin._validate_image("image.efi", "UNCONFIGURED"),
                image_validation.Result.FAIL,
            )
        self.assertTrue(any("UNCONFIGURED is invalid" in line for line in logs.output))

    def test_aarch64_runtime_profile_requires_64k_alignment(self) -> None:
        self.pe.FILE_HEADER.Machine = image_validation.MACHINE_TYPE[
            "IMAGE_FILE_MACHINE_ARM64"
        ]
        self.pe.OPTIONAL_HEADER.Subsystem = image_validation.SUBSYSTEM_TYPE[
            "IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER"
        ]
        with self.assertLogs(level="ERROR"):
            self.assertEqual(
                self.plugin._validate_image("image.efi"), image_validation.Result.FAIL
            )
        self.pe.OPTIONAL_HEADER.SectionAlignment = 0x10000
        self.assertEqual(
            self.plugin._validate_image("image.efi"), image_validation.Result.PASS
        )


class TestPostBuildProfiles(ImageValidationTestCase):
    """Verify profile selection during post-build image validation."""

    def setUp(self) -> None:
        """Prepare build output and mock the platform parsers and image tests."""
        super().setUp()
        self.build_directory = self.root / "Build"
        self.module_directory = self.build_directory / "X64" / "Example"
        self.efi_path = self.module_directory / "OUTPUT" / "Example.efi"
        self.efi_path.parent.mkdir(parents=True)
        self.efi_path.touch()
        self.builder = Mock(spec=image_validation.UefiBuilder)
        self.builder.ws = str(self.root)
        self.builder.env = Mock()
        self.values = {
            "ACTIVE_PLATFORM": "Platform.dsc",
            "FLASH_DEFINITION": "Platform.fdf",
            "TARGET_ARCH": "X64",
            "BUILD_OUTPUT_BASE": str(self.build_directory),
        }
        self.builder.env.GetValue.side_effect = self.values.get
        self.builder.env.GetBuildValue.return_value = None
        self.builder.env.GetAllBuildKeyValues.return_value = {}
        self.builder.edk2path = Mock()
        self.builder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath.side_effect = (
            lambda path: str(self.root / path)
        )

        dsc = self.contexts.enter_context(
            patch.object(image_validation, "DscParser")
        ).return_value
        dsc.LocalVars = {}
        self.fdf = self.contexts.enter_context(
            patch.object(image_validation, "FdfParser")
        ).return_value
        self.fdf.FVs = {}
        self.inf_parser = self.contexts.enter_context(
            patch.object(image_validation, "InfParser")
        )
        self.inf = self.inf_parser.return_value.SetEdk2Path.return_value
        self.inf.Dict = {"MODULE_TYPE": "UEFI_APPLICATION"}
        # Test profile routing, not PE contents; the EFI fixtures are empty files.
        self.validate_image = self.contexts.enter_context(
            patch.object(self.plugin, "_validate_image")
        )
        self.validate_image.return_value = image_validation.Result.PASS

    def test_validates_missing_inf_using_makefile(self) -> None:
        """Validate an image using Makefile metadata when its INF is missing."""
        for filename in ("GNUmakefile", "Makefile"):
            with self.subTest(filename=filename):
                makefile = self.module_directory / filename
                makefile.write_text(
                    "MODULE_NAME = Example\nMODULE_TYPE = DXE_DRIVER\n",
                    encoding="utf-8",
                )
                self.assertEqual(self.plugin.do_post_build(self.builder), 0)
                self.validate_image.assert_called_once_with(
                    str(self.efi_path), "DXE_DRIVER"
                )
                self.inf_parser.assert_not_called()
                self.validate_image.reset_mock()
                makefile.unlink()

    def test_inf_takes_precedence_over_makefile(self) -> None:
        """Prefer the as-built INF profile over conflicting Makefile metadata."""
        self.efi_path.with_suffix(".inf").touch()
        (self.module_directory / "GNUmakefile").write_text(
            "MODULE_NAME = Example\nMODULE_TYPE = DXE_DRIVER\n", encoding="utf-8"
        )
        self.assertEqual(self.plugin.do_post_build(self.builder), 0)
        self.inf.ParseFile.assert_called_once_with(self.efi_path.with_suffix(".inf"))
        self.validate_image.assert_called_once_with(
            str(self.efi_path), "UEFI_APPLICATION"
        )

    def test_inf_without_module_type_uses_default(self) -> None:
        """Use DEFAULT when an existing INF omits its module type."""
        self.efi_path.with_suffix(".inf").touch()
        self.inf.Dict = {}
        self.assertEqual(self.plugin.do_post_build(self.builder), 0)
        self.validate_image.assert_called_once_with(str(self.efi_path), "DEFAULT")

    def test_skips_image_without_inf_or_matching_makefile(self) -> None:
        """Warn and skip images with no usable module metadata."""
        for metadata in (
            "",
            "MODULE_NAME = DifferentModule\nMODULE_TYPE = DXE_DRIVER\n",
        ):
            with self.subTest(metadata=metadata):
                (self.module_directory / "GNUmakefile").write_text(
                    metadata, encoding="utf-8"
                )
                with self.assertLogs(level="WARNING") as logs:
                    self.assertEqual(self.plugin.do_post_build(self.builder), 0)
                self.assertTrue(
                    any("Cannot find Example.inf" in message for message in logs.output)
                )
                self.validate_image.assert_not_called()

    def test_makefile_image_validation_failure_fails_post_build(self) -> None:
        """Fail the post-build step when an image using Makefile metadata fails."""
        (self.module_directory / "GNUmakefile").write_text(
            "MODULE_NAME = Example\nMODULE_TYPE = DXE_DRIVER\n", encoding="utf-8"
        )
        self.validate_image.return_value = image_validation.Result.FAIL
        with self.assertLogs(level="ERROR"):
            self.assertEqual(self.plugin.do_post_build(self.builder), 1)
        self.validate_image.assert_called_once_with(str(self.efi_path), "DXE_DRIVER")

    def test_precompiled_failure_survives_build_output_processing(self) -> None:
        """Never discard a precompiled failure, even with no built images."""
        self.fdf.FVs = {
            "FVMAIN": {
                "Files": {"Prebuilt": {"type": "DRIVER", "PE32": ["Prebuilt.efi"]}}
            }
        }
        self.efi_path.with_suffix(".inf").touch()
        for built_image in (True, False):
            with self.subTest(built_image=built_image):
                if not built_image:
                    self.efi_path.unlink()
                self.validate_image.reset_mock()
                self.validate_image.side_effect = [
                    image_validation.Result.FAIL,
                    image_validation.Result.PASS,
                ]
                with self.assertLogs(level="ERROR"):
                    self.assertEqual(self.plugin.do_post_build(self.builder), 1)
                expected = [call(str(self.root / "Prebuilt.efi"), None)]
                if built_image:
                    expected.append(call(str(self.efi_path), "UEFI_APPLICATION"))
                self.assertEqual(self.validate_image.call_args_list, expected)

    def test_built_image_failure_survives_later_success(self) -> None:
        """Keep validating remaining images without losing an earlier failure."""
        second_image = self.efi_path.with_name("Second.efi")
        second_image.touch()
        for image in (self.efi_path, second_image):
            image.with_suffix(".inf").touch()
        self.validate_image.side_effect = [
            image_validation.Result.FAIL,
            image_validation.Result.PASS,
        ]
        with self.assertLogs(level="ERROR"):
            self.assertEqual(self.plugin.do_post_build(self.builder), 1)
        self.assertCountEqual(
            self.validate_image.call_args_list,
            [
                call(str(self.efi_path), "UEFI_APPLICATION"),
                call(str(second_image), "UEFI_APPLICATION"),
            ],
        )

    def test_undefined_precompiled_variable_fails_build_and_continues(self) -> None:
        """Report the unresolved path and still validate subsequent images."""
        self.fdf.FVs = {
            "FVMAIN": {
                "Files": {
                    "Prebuilt": {
                        "type": "DRIVER",
                        "PE32": ["$(MISSING)/Bad.efi", "Valid.efi"],
                    }
                }
            }
        }
        self.efi_path.with_suffix(".inf").touch()
        with self.assertLogs(level="ERROR") as logs:
            self.assertEqual(self.plugin.do_post_build(self.builder), 1)
        self.assertTrue(
            any("MISSING" in line and "Bad.efi" in line for line in logs.output)
        )
        self.assertEqual(
            self.validate_image.call_args_list,
            [
                call(str(self.root / "Valid.efi"), None),
                call(str(self.efi_path), "UEFI_APPLICATION"),
            ],
        )

    def test_gitignore_patterns_exclude_precompiled_and_built_images(self) -> None:
        config_path = self.root / "override.yaml"
        config_path.write_text(
            'IGNORE_LIST:\n  - "**/Ignored*.efi"\n  - "**/Example.efi"\n',
            encoding="utf-8",
        )
        self.values["PE_VALIDATION_PATH"] = str(config_path)
        self.fdf.FVs = {
            "FVMAIN": {
                "Files": {
                    "Prebuilt": {
                        "type": "UEFI_APPLICATION",
                        "PE32": ["Binaries/IgnoredDriver.efi", "Binaries/Keep.efi"],
                    }
                }
            }
        }
        self.assertEqual(self.plugin.do_post_build(self.builder), 0)
        self.validate_image.assert_called_once_with(
            str(self.root / "Binaries" / "Keep.efi"), "UEFI_APPLICATION"
        )
        self.inf_parser.assert_not_called()

    def test_config_override_is_delivered_to_test_manager(self) -> None:
        config_path = self.root / "override.yaml"
        config_path.write_text(
            "X64:\n  UEFI_APPLICATION:\n    IMAGE_BASE: 1048576\n", encoding="utf-8"
        )
        self.values["PE_VALIDATION_PATH"] = str(config_path)
        self.efi_path.with_suffix(".inf").touch()
        self.assertEqual(self.plugin.do_post_build(self.builder), 0)
        self.assertIs(self.plugin.test_manager.config_data, self.plugin.config_data)
        self.assertEqual(
            self.plugin.test_manager.config_data["IMAGE_FILE_MACHINE_AMD64"][
                "UEFI_APPLICATION"
            ],
            {"IMAGE_BASE": 0x100000},
        )

    def test_missing_or_malformed_override_config_fails_build(self) -> None:
        config_path = self.root / "override.yaml"
        self.values["PE_VALIDATION_PATH"] = str(config_path)
        for malformed in (False, True):
            with self.subTest(malformed=malformed):
                if malformed:
                    config_path.write_text("X64: [\n", encoding="utf-8")
                with self.assertLogs(level="ERROR"):
                    self.assertEqual(self.plugin.do_post_build(self.builder), 1)
                self.validate_image.assert_not_called()

    def test_missing_or_malformed_default_config_fails_build(self) -> None:
        config_path = self.root / "default.yaml"
        with patch.object(image_validation, "DEFAULT_CONFIG_FILE_PATH", config_path):
            for malformed in (False, True):
                with self.subTest(malformed=malformed):
                    if malformed:
                        config_path.write_text("TARGET_ARCH: [\n", encoding="utf-8")
                    with self.assertLogs(level="ERROR"):
                        self.assertEqual(self.plugin.do_post_build(self.builder), 1)
                    self.validate_image.assert_not_called()


if __name__ == "__main__":
    unittest.main()
