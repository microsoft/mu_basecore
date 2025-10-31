# @file RepoRemoteCheck.py
#
# Plugin that validates git repositories against remote criteria.
#
##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import logging
import os
import uuid
import yaml
from datetime import datetime
from edk2toolext.environment.plugintypes.uefi_build_plugin import (
    IUefiBuildPlugin
)
from typing import Dict, Any
from git import Repo, GitCommandError

DEFAULT_CONFIG_FILE = "RepoRemoteCheckConfig.yml"


class RepoRemoteCheck(IUefiBuildPlugin):
    """Plugin to validate git repositories against remote commit criteria.

    This plugin checks whether commits in specified git repositories are
    present or absent in specified remote URLs based on configuration rules.
    """

    def __init__(self):
        """Initialize the RepoRemoteCheck plugin."""
        pass

    def do_pre_build(self, thebuilder):
        """Execute the repository remote check before the build.

        Args:
            thebuilder: The build environment object

        Returns:
            int: 0 on success, 1 on failure
        """
        starttime = datetime.now()
        logging.info("-" * 57)
        logging.info("----------- Repo Remote Check Starting -----------")
        logging.info("-" * 57)

        # Load the configuration file
        config_path = thebuilder.env.GetValue(
            "REPO_REMOTE_CHECK_CONFIG_PATH", None)
        if config_path is None:
            # Use default configuration file in workspace root
            config_path = os.path.join(thebuilder.ws, DEFAULT_CONFIG_FILE)
            if not os.path.isfile(config_path):
                logging.info(
                    "REPO_REMOTE_CHECK_CONFIG_PATH not set and default "
                    f"config '{DEFAULT_CONFIG_FILE}' not found. Skipping "
                    "repository remote check.")
                return 0
            logging.info(f"Using default configuration: {config_path}")
        elif not os.path.isfile(config_path):
            logging.error(
                "Invalid REPO_REMOTE_CHECK_CONFIG_PATH. "
                f"File not found: {config_path}")
            return 1

        # Parse the configuration file
        try:
            with open(config_path, 'r') as f:
                if config_path.endswith('.json'):
                    import json
                    config_data = json.load(f)
                else:
                    config_data = yaml.safe_load(f)
        except Exception as e:
            logging.error(
                f"Error parsing configuration file "
                f"{config_path}: {e}")
            return 1

        # Validate the overal configuration structure
        if not config_data or 'repositories' not in config_data:
            logging.error(
                "Configuration file must contain a 'repositories' key")
            return 1

        repositories = config_data.get('repositories', [])
        if not isinstance(repositories, list):
            logging.error("'repositories' must be a list")
            return 1

        # Process each repository
        failed = False
        for repo_config in repositories:
            if not self._check_repository(thebuilder.ws, repo_config):
                failed = True

        endtime = datetime.now()
        elapsed = endtime - starttime

        if failed:
            logging.error("-" * 57)
            logging.error("----------- Repo Remote Check Failed -----------")
            logging.error(
                f"----------- {elapsed.total_seconds():.2f}s "
                "elapsed -----------")
            logging.error("-" * 57)
            return 1

        logging.info("-" * 57)
        logging.info("----------- Repo Remote Check Passed -----------")
        logging.info(
            f"----------- {elapsed.total_seconds():.2f}s "
            "elapsed -----------")
        logging.info("-" * 57)
        return 0

    def _check_repository(self, workspace_root: str,
                          repo_config: Dict[str, Any]) -> bool:
        """Check a single repository against its configured conditions.

        Args:
            workspace_root: The root workspace path
            repo_config: Dictionary containing repository configuration

        Returns:
            bool: True if all conditions pass, False otherwise
        """
        # Validate repository configuration
        if 'path' not in repo_config:
            logging.error("Repository configuration missing 'path' key")
            return False

        repo_path = repo_config['path']
        abs_repo_path = os.path.join(workspace_root, repo_path)

        # Check if the repo path exists
        if not os.path.exists(abs_repo_path):
            logging.error(f"Repository path does not exist: {repo_path}")
            return False

        # Check if the repo path is a git repository
        try:
            repo = Repo(abs_repo_path)
        except Exception as e:
            logging.error(
                f"Path is not a valid git repository: "
                f"{repo_path} - {e}")
            return False

        # Get the current HEAD commit
        try:
            current_commit = repo.head.commit.hexsha
            logging.debug(
                f"Repository {repo_path}: "
                f"Current HEAD is {current_commit[:8]}")
        except Exception as e:
            logging.error(
                f"Failed to get HEAD commit for {repo_path}: {e}")
            return False

        # Get all conditions for this repo
        conditions = repo_config.get('conditions', [])
        if not isinstance(conditions, list):
            logging.error(
                f"'conditions' must be a list for repository {repo_path}")
            return False

        if not conditions:
            logging.warning(
                f"No conditions specified for repository "
                f"{repo_path}, skipping")
            return True

        # Check each condition
        all_passed = True
        for condition in conditions:
            if not self._check_condition(
                    repo, repo_path, current_commit, condition):
                all_passed = False

        return all_passed

    def _check_condition(self, repo: Repo, repo_path: str, commit_sha: str,
                         condition: Dict[str, Any]) -> bool:
        """Check a single condition against a repository.

        Args:
            repo: GitPython Repo object
            repo_path: Display path for the repository
            commit_sha: The commit SHA to check
            condition: Dictionary containing condition configuration

        Returns:
            bool: True if condition passes, False otherwise
        """
        if 'type' not in condition:
            logging.error(
                f"Condition missing 'type' key for repository {repo_path}")
            return False

        condition_type = condition['type']
        remote_url = condition.get('remote')

        if not remote_url:
            logging.error(
                f"Condition missing 'remote' key for repository {repo_path}")
            return False

        # Check if the commit exists in the remote
        try:
            commit_in_remote = self._is_commit_in_remote(
                repo, commit_sha, remote_url)
        except Exception as e:
            logging.error(
                f"Error checking remote {remote_url} for repository "
                f"{repo_path}: {e}")
            return False

        # Evaluate the condition
        if condition_type == "present_in_remote":
            if not commit_in_remote:
                logging.error(
                    f"CONDITION FAILED: Repository '{repo_path}' "
                    f"commit {commit_sha[:8]} is NOT present in remote "
                    f"'{remote_url}' (expected: present)")
                return False
            logging.info(
                f"Repository '{repo_path}' commit {commit_sha[:8]} "
                f"is present in remote '{remote_url}'")
            return True

        elif condition_type == "not_present_in_remote":
            if commit_in_remote:
                logging.error(
                    f"CONDITION FAILED: Repository '{repo_path}' "
                    f"commit {commit_sha[:8]} IS present in remote "
                    f"'{remote_url}' (expected: not present)")
                return False
            logging.info(
                f"Repository '{repo_path}' commit {commit_sha[:8]} "
                f"is NOT present in remote '{remote_url}'")
            return True

        else:
            logging.error(
                f"Unknown condition type '{condition_type}' for repository "
                f"{repo_path}. Valid types: 'present_in_remote', "
                "'not_present_in_remote'")
            return False

    def _is_commit_in_remote(self, repo: Repo, commit_sha: str,
                             remote_url: str) -> bool:
        """Check if a commit exists in a remote repository.

        Args:
            repo: GitPython Repo object
            commit_sha: The commit SHA to check
            remote_url: The remote URL to check against

        Returns:
            bool: True if commit exists in remote, False otherwise

        Raises:
            Exception: If there's an error accessing the remote
        """
        try:
            # Use git branch -r --contains to check if commit exists
            # Fetch from the remote (with depth limited to avoid fetching
            # everything). A temporary remote name is used to avoid
            # potential naming conflicts.
            temp_remote_name = f"temp_remote_{uuid.uuid4().hex[:8]}"

            try:
                repo.git.remote('add', temp_remote_name, remote_url)
                repo.git.fetch(temp_remote_name, '--quiet')

                # Check if the commit exists in any remote branch
                # git branch -r --contains returns a list of branches
                # containing the commit. If the list is empty, then
                # the commit doesn't exist on the remote.
                try:
                    result = repo.git.branch(
                        '-r',
                        '--contains',
                        commit_sha,
                        f'{temp_remote_name}/*'
                    )
                    return bool(result.strip())
                except GitCommandError:
                    # Command failed, likely because the commit doesn't exist
                    return False

            finally:
                try:
                    repo.git.remote('remove', temp_remote_name)
                except Exception:
                    pass

        except GitCommandError as e:
            if "does not appear to be a git repository" in str(e).lower():
                raise Exception(f"Invalid git remote URL: {remote_url}")
            elif ("could not read" in str(e).lower() or
                  "failed to connect" in str(e).lower() or
                  "authentication failed" in str(e).lower()):
                raise Exception(
                    f"Failed to connect to remote: {remote_url}")
            else:
                raise Exception(f"Git error checking remote: {e}")
        except Exception as e:
            raise Exception(f"Unexpected error checking remote: {e}")
