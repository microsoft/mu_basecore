##
## Script to Build CORE UEFI firmware
##
##
## Copyright Microsoft Corporation, 2018
##
import os, sys
import logging

#
#==========================================================================
# PLATFORM BUILD ENVIRONMENT CONFIGURATION
#
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
WORKSPACE_PATH = os.path.dirname(os.path.dirname(SCRIPT_PATH))
PROJECT_SCOPE = ('corebuild',)

# MODULE_PKGS = ('SM_UDK', "SM_UDK_INTERNAL", "SURF_KBL", "SM_INTEL_KBL")
# MODULE_PKG_PATHS = ";".join(os.path.join(WORKSPACE_PATH, pkg_name) for pkg_name in MODULE_PKGS)
#
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

    # Now that we've got the environment updated, we can bring in the worker.
    import PlatformBuildWorker

    # Tear down logging so the following script doesn't trample it.
    # NOTE: This uses some non-standard calls.
    default_logger = logging.getLogger('')
    while default_logger.handlers:
        default_logger.removeHandler(default_logger.handlers[0])

    # Finally, jump to the main routine.
    PlatformBuildWorker.main(WORKSPACE_PATH, PROJECT_SCOPE)
