## @file CommonBuildEntry.py
# This module contains code that is shared between all the entry points for PlatformBUild
# scripts.
#
##
# Copyright (c) 2017, Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
import os
import sys
import re
import logging
import subprocess
from datetime import datetime
import SelfDescribingEnvironment


# Simplified Comparison Function borrowed from StackOverflow...
# https://stackoverflow.com/questions/1714027/version-number-comparison
# With Python 3.0 help from:
# https://docs.python.org/3.0/whatsnew/3.0.html#ordering-comparisons
def version_compare(version1, version2):
    def normalize(v):
        return [int(x) for x in re.sub(r'(\.0+)*$','', v).split(".")]
    (a, b) = (normalize(version1), normalize(version2))
    return (a > b) - (a < b)


#
# Shell Command with Output Helper
# This helper will attempt to run a shell command and return
# the stdout results.
# Also abstracts the bytes-vs-strings ambiguity in Python 2, 3.
# Will raise an error if return code is non-zero.
def cmd_with_output(cmd_string, cwd):
    c = subprocess.Popen(cmd_string, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=cwd)
    c.wait()

    # Get the data.
    cmd_result = c.stdout.read()
    # PYTHON COMPAT HACK BEGIN
    if type(cmd_result) is bytes:
      cmd_result = cmd_result.decode()
    # PYTHON COMPAT HACK END

    # Check for errors.
    if c.returncode != 0:
        raise RuntimeError(cmd_result)

    return cmd_result


#
# minimum_env_init() will attempt to follow all of the steps
# necessary to get this process off the ground.
def minimum_env_init(my_workspace_path, my_project_scope):
    # TODO: Check the Git version against minimums.

    # Check the Python version against minimums.
    cur_py = "%d.%d.%d" % sys.version_info[:3]
    soft_min_py = "3.6"
    hard_min_py = "2.7"
    if version_compare(hard_min_py, cur_py) > 0:
        raise RuntimeError("Please upgrade Python! Current version is %s. Minimum is %s." % (cur_py, hard_min_py))
    if version_compare(soft_min_py, cur_py) > 0:
        logging.critical("Please upgrade Python! Current version is %s. Minimum is %s." % (cur_py, soft_min_py))

    # Initialized the build environment.
    return SelfDescribingEnvironment.BootstrapEnvironment(my_workspace_path, my_project_scope)


#
# configure_base_logging() sets up only the logging that will
# be used by all commands. (ie. doesn't configure any logging files)
def configure_base_logging(mode="standard"):
    # Initialize logging.
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)

    # Adjust console mode depending on mode.
    console = logging.StreamHandler()
    if mode == "vs":
        console.setLevel(logging.DEBUG)
        console.setFormatter(logging.Formatter("%(message)s"))
    elif mode == 'verbose':
        console.setLevel(logging.DEBUG)
        console.setFormatter(logging.Formatter("%(levelname)s - %(message)s"))
    elif mode == 'simple':
        console.setLevel(logging.INFO)
        console.setFormatter(logging.Formatter("%(levelname)s - %(message)s"))
    else:
        console.setLevel(logging.CRITICAL)
        console.setFormatter(logging.Formatter("%(levelname)s - %(message)s"))

    # Add the console as a logger, now that it's configured.
    logger.addHandler(console)

#
# setup_process() automates all of the processes that should be unique
# to each platform build. It will attempt to set up the repos and
# anything else that's important.
def setup_process(my_workspace_path, my_project_scope, my_required_repos, force_it=False):
    # Grab the remaining Git repos.
    if my_required_repos:
        #### Git Repos: STEP 1 --------------------------------------
        # Make sure that the repos are all synced.
        print("## Syncing Git repositories: %s..." % ", ".join(my_required_repos))
        cmd_parts = ("git", "submodule", "sync")
        cmd_parts += my_required_repos
        cmd_string = " ".join(cmd_parts)
        try:
            c = subprocess.Popen(cmd_string, stderr=subprocess.STDOUT, cwd=my_workspace_path)
            c.wait()
            if c.returncode != 0:
                raise RuntimeError(c.stdout.read())
        except:
            print("FAILED!\n")
            print("Failed to sync required repositories!\n")
            raise
        print("Done.\n")

        #### Git Repos: STEP 2 --------------------------------------
        # Iterate through all repos and see whether they should be fetched.
        for required_repo in my_required_repos:
            print("## Checking Git repository: %s..." % required_repo)

            #### Git Repos: STEP 2a ---------------------------------
            # Need to determine whether to skip this repo.
            repo_path = os.path.join(my_workspace_path, required_repo)
            skip_repo = False
            # If the repo exists (and we're not forcing things) make
            # sure that it's not in a "dirty" state.
            if os.path.exists(repo_path) and not force_it:
                git_data = None
                cmd_parts = ("git", "diff", required_repo)
                cmd_string = " ".join(cmd_parts)
                try:
                    git_data = cmd_with_output(cmd_string, my_workspace_path)
                except:
                    # We don't actually care. We'll handle repo errors later.
                    # We only care if we can get the data.
                    pass

                # If anything was returned, we should skip processing the repo.
                # It is either on a different commit or it has local changes.
                if git_data:
                    print("-- NOTE: Repo currently exists and appears to have local changes!")
                    print("-- Skipping fetch!")
                    skip_repo = True

            #### Git Repos: STEP 2b ---------------------------------
            # If we're not skipping, grab it.
            if not skip_repo or force_it:
                print("## Fetching repo.")
                cmd_parts = ("git", "submodule", "update", "--init", "--recursive", "--progress", required_repo)
                cmd_string = " ".join(cmd_parts)
                try:
                    c = subprocess.Popen(cmd_string, stderr=subprocess.STDOUT, cwd=my_workspace_path)
                    c.wait()
                    if c.returncode != 0:
                        raise RuntimeError(c.stdout.read())
                except:
                    print("FAILED!\n")
                    print("Failed to fetch required repositories!\n")
                    raise
            print("Done.\n")

    # Now that we should have all of the required code,
    # we're ready to build the environment and fetch the
    # dependencies for this project.
    print("## Fetching all external dependencies...")
    (build_env, shell_env) = minimum_env_init(my_workspace_path, my_project_scope)
    SelfDescribingEnvironment.UpdateDependencies(my_workspace_path, my_project_scope)
    print("Done.\n")

    # TODO: Install any certs any other things that might be required.


def update_process(my_workspace_path, my_project_scope):
    # Get the environment set up.
    logging.info("## Parsing environment...")
    (build_env, shell_env) = minimum_env_init(my_workspace_path, my_project_scope)
    logging.info("Done.\n")

    # Update the environment.
    logging.info("## Updating environment...")
    SelfDescribingEnvironment.UpdateDependencies(my_workspace_path, my_project_scope)
    logging.info("Done.\n")


def build_process(my_workspace_path, my_project_scope, my_module_pkg_paths):
    #
    # Initialize file-based logging.
    #
    logfile = os.path.join(my_workspace_path, "Build", "BUILDLOG.TXT")
    if not os.path.isdir(os.path.dirname(logfile)):
        os.makedirs(os.path.dirname(logfile))

    filelogger = logging.FileHandler(filename=(logfile), mode='w')
    filelogger.setLevel(logging.DEBUG)
    filelogger.setFormatter(logging.Formatter("%(levelname)s - %(message)s"))
    logging.getLogger('').addHandler(filelogger)

    logging.info("Log Started: " + datetime.strftime(datetime.now(), "%A, %B %d, %Y %I:%M%p" ))
    logging.info("Running Python version: " + str(sys.version_info))

    #
    # Next, get the environment set up.
    #
    try:
        (build_env, shell_env) = minimum_env_init(my_workspace_path, my_project_scope)
        if not SelfDescribingEnvironment.VerifyEnvironment(my_workspace_path, my_project_scope):
            raise RuntimeError("Validation failed.")
    except:
        raise RuntimeError("Environment is not in a state to build! Please run '--UPDATE'.")
    # NOTE: This implicitly assumes that the PlatformBuild script path is in PYTHONPATH.
    from PlatformBuildWorker import PlatformBuilder

    #
    # Now we can actually kick off a build.
    #
    PB = PlatformBuilder(my_workspace_path, my_module_pkg_paths, build_env.plugins, sys.argv)
    retcode = PB.Go()

    if(retcode != 0):
        logging.critical("Error")
        logging.critical("Log file at " + logfile)
    else:
        logging.critical("Success")

    #get all vars needed as we can't do any logging after shutdown otherwise our log is cleared.  
    #Log viewer
    ep = PB.env.GetValue("LaunchBuildLogProgram")
    LogOnSuccess = PB.env.GetValue("LaunchLogOnSuccess")
    LogOnError = PB.env.GetValue("LaunchLogOnError")
    
    #end logging
    logging.shutdown()
    #no more logging

    if(ep != None):
        cmd = ep + " " + logfile

    #
    # Conditionally launch the shell to show build log
    #
    #
    if( ((retcode != 0) and (LogOnError.upper() == "TRUE")) or (LogOnSuccess.upper() == "TRUE")):
        subprocess.Popen(cmd, shell=True)
        
    sys.exit(retcode)


def build_entry(my_script_path, my_workspace_path, my_required_repos, my_project_scope, my_module_pkgs, my_module_pkg_paths):
    logging_mode = "standard"
    script_process = "build"
    force_process = False

    # Check for some well-known parameters.
    for arg in sys.argv:
        if "--ANDUPDATE" == arg.upper():
            script_process = "and_update"
        if "--FORCE" == arg.upper():
            force_process = True
        if "--SETUP" == arg.upper():
            logging_mode = "simple"
            script_process = "setup"
        if "--UPDATE" == arg.upper():
            logging_mode = "simple"
            script_process = "update"
        if "--VERBOSE" == arg.upper():
            logging_mode = "verbose"
        if "--VSMODE" == arg.upper():
            logging_mode = "vs"

    # TODO: Scrub the parameters so they're not passed on to the next script.

    # Turn on logging for the remainder of the process.
    configure_base_logging(logging_mode)

    # Execute the requested process.
    if script_process == "setup":
        setup_process(my_workspace_path, my_project_scope, my_required_repos, force_it=force_process)
    elif script_process == "update":
        update_process(my_workspace_path, my_project_scope)
    else:
        if script_process == "and_update":
            update_process(my_workspace_path, my_project_scope)
        build_process(my_workspace_path, my_project_scope, my_module_pkg_paths)
