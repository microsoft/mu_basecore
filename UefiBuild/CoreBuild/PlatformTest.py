##
## Script to Build CORE UEFI firmware
##
##
## Copyright Microsoft Corporation, 2018
##
import os, sys
import logging
import logging
import sys
import os
import fnmatch

#
#==========================================================================
# PLATFORM BUILD ENVIRONMENT CONFIGURATION
#
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
WORKSPACE_PATH = os.path.dirname(os.path.dirname(SCRIPT_PATH))
PROJECT_SCOPE = ('corebuild',)

#==========================================================================
#

# Smallest 'main' possible. Please don't add unnecessary code.
if __name__ == '__main__':
    # Include the most basic paths so that we can get to known build components.
    sys.path.append(os.path.join(WORKSPACE_PATH, 'UefiBuild'))
    import CommonBuildEntry

    # Make sure that we can get some logging out.
    CommonBuildEntry.configure_base_logging('verbose')

    # Bring up the common minimum environment.
    CommonBuildEntry.update_process(WORKSPACE_PATH, PROJECT_SCOPE)

    # Tear down logging so the following script doesn't trample it.
    # NOTE: This uses some non-standard calls.
    default_logger = logging.getLogger('')
    while default_logger.handlers:
        default_logger.removeHandler(default_logger.handlers[0])

    #actual code for python unit tests
    from UtilityFunctions import RunCmd
    overall_success = 0
    failed = {}
    matches = []
    for root, dirnames, filenames in os.walk(WORKSPACE_PATH):
        for filename in fnmatch.filter(filenames, '*_test.py'):
            matches.append(os.path.join(root))
            break
    cmd = 'python -m unittest discover -s {} -p "*_test.py" -v'
    for a in set(matches):
        ret = RunCmd(cmd.format(a))
        if(ret != 0):
            overall_success = ret

    sys.exit(overall_success)
