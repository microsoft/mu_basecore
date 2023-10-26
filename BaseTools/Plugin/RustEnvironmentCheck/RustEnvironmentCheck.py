# @file RustEnvironmentCheck.py
# Plugin to confirm Rust tools are present needed to compile Rust code during
# firmare build.
#
# This provides faster, direct feedback to a developer about the changes they
# may need to make to successfully build Rust code. Otherwise, the build will
# fail much later during firmware code compilation when Rust tools are invoked
# with messages that are ambiguous or difficult to find.
#
# Note:
#   - The entire plugin is enabled/disabled by scope.
#   - Individual tools can be opted out by setting the environment variable
#     `RUST_ENV_CHECK_TOOL_EXCLUSIONS` with a comma separated list of the tools
#     to exclude. For example, "rustup, cargo tarpaulin" would not require that
#     those tools be installed.
#
# Copyright (c) Microsoft Corporation.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import logging
import re
from collections import namedtuple
from edk2toolext.environment import shell_environment
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toollib.utility_functions import RunCmd
from io import StringIO

RustToolInfo = namedtuple("RustToolInfo", ["presence_cmd", "install_help"])
RustToolChainInfo = namedtuple("RustToolChainInfo", ["error", "toolchain"])


class RustEnvironmentCheck(IUefiBuildPlugin):
    """Checks that the system environment is ready to build Rust code."""

    def do_pre_build(self, _: UefiBuilder) -> int:
        """Rust environment checks during pre-build.

        Args:
            builder (UefiBuilder): A UEFI builder object for this build.

        Returns:
            int: The number of environment issues found. Zero indicates no
            action is needed.
        """
        def verify_cmd(name: str, params: str = "--version") -> bool:
            """Indicates if a command can successfully be executed.

            Args:
                name (str): Tool name.
                params (str, optional): Tool params. Defaults to "--version".

            Returns:
                bool: True on success. False on failure to run the command.
            """
            cmd_output = StringIO()
            ret = RunCmd(name, params, outstream=cmd_output,
                         logging_level=logging.DEBUG)
            return ret == 0

        def verify_workspace_rust_toolchain_is_installed() -> RustToolChainInfo:
            """Verifies the rust toolchain used in the workspace is available.

            Note: This function does not use the toml library to parse the toml
            file since the file is very simple and its not desirable to add the
            toml module as a dependency.

            Returns:
                RustToolChainInfo: A tuple that indicates if the toolchain is
                available and any the toolchain version if found.
            """
            WORKSPACE_TOOLCHAIN_FILE = "rust-toolchain.toml"

            toolchain_version = None
            try:
                with open(WORKSPACE_TOOLCHAIN_FILE, 'r') as toml_file:
                    content = toml_file.read()
                    match = re.search(r'channel\s*=\s*"([^"]+)"', content)
                    if match:
                        toolchain_version = match.group(1)
            except FileNotFoundError:
                # If a file is not found. Do not check any further.
                return RustToolChainInfo(error=False, toolchain=None)

            if not toolchain_version:
                # If the file is not in an expected format, let that be handled
                # elsewhere and do not look further.
                return RustToolChainInfo(error=False, toolchain=None)

            installed_toolchains = StringIO()
            ret = RunCmd("rustup", "toolchain list",
                         outstream=installed_toolchains,
                         logging_level=logging.DEBUG)

            # The ability to call "rustup" is checked separately. Here do not
            # continue if the command is not successful.
            if ret != 0:
                return RustToolChainInfo(error=False, toolchain=None)

            installed_toolchains = installed_toolchains.getvalue().splitlines()
            return RustToolChainInfo(
                error=not any(toolchain_version in toolchain
                              for toolchain in installed_toolchains),
                toolchain=toolchain_version)

        generic_rust_install_instructions = \
            "Visit https://rustup.rs/ to install Rust and cargo."

        tools = {
            "rustup": RustToolInfo(
                presence_cmd=("rustup",),
                install_help=generic_rust_install_instructions
                ),
            "rustc": RustToolInfo(
                presence_cmd=("rustc",),
                install_help=generic_rust_install_instructions
                ),
            "cargo": RustToolInfo(
                presence_cmd=("cargo",),
                install_help=generic_rust_install_instructions
                ),
            "cargo build": RustToolInfo(
                presence_cmd=("cargo", "build --help"),
                install_help=generic_rust_install_instructions
                ),
            "cargo check": RustToolInfo(
                presence_cmd=("cargo", "check --help"),
                install_help=generic_rust_install_instructions
                ),
            "cargo fmt": RustToolInfo(
                presence_cmd=("cargo", "fmt --help"),
                install_help=generic_rust_install_instructions
                ),
            "cargo test": RustToolInfo(
                presence_cmd=("cargo", "test --help"),
                install_help=generic_rust_install_instructions
                ),
            "cargo make": RustToolInfo(
                presence_cmd=("cargo", "make --version"),
                install_help="Read installation instructions at "
                "https://github.com/sagiegurari/cargo-make#installation "
                "to install Cargo make."
                ),
            "cargo tarpaulin": RustToolInfo(
                presence_cmd=("cargo", "tarpaulin --version"),
                install_help="View the installation instructions at "
                "https://crates.io/crates/cargo-tarpaulin to install Cargo "
                "tarpaulin. A tool used for Rust code coverage."
                ),
        }

        excluded_tools_in_shell = shell_environment.GetEnvironment().get_shell_var(
            "RUST_ENV_CHECK_TOOL_EXCLUSIONS")
        excluded_tools = ([t.strip() for t in
                           excluded_tools_in_shell.split(",")] if
                          excluded_tools_in_shell else [])

        errors = 0
        for tool_name, tool_info in tools.items():
            if tool_name not in excluded_tools and not verify_cmd(*tool_info.presence_cmd):
                logging.error(
                    f"Rust Environment Failure: {tool_name} is not installed "
                    "or not on the system path.\n\n"
                    f"Instructions:\n{tool_info.install_help}\n\n"
                    f"Ensure \"{' '.join(tool_info.presence_cmd)}\" can "
                    "successfully be run from a terminal before trying again.")
                errors += 1

        rust_toolchain_info = verify_workspace_rust_toolchain_is_installed()
        if rust_toolchain_info.error:
            # The "rustc -Vv" command could be run in the script with the
            # output given to the user. This is approach is also meant to show
            # the user how to use the tools since getting the target triple is
            # important.
            logging.error(
                f"This workspace requires the {rust_toolchain_info.toolchain} "
                "toolchain.\n\n"
                "Run \"rustc -Vv\" and use the \"host\" value to install the "
                "toolchain needed:\n"
                f"  \"rustup toolchain install {rust_toolchain_info.toolchain}-"
                "<host>\"\n\n"
                "  \"rustup component add rust-src "
                f"{rust_toolchain_info.toolchain}-<host>\"")
            errors += 1

        return errors
