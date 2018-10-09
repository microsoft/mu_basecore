##
## Script to Build Project Mu compliant packages
##
##
## Copyright Microsoft Corporation, 2018
##
import os
import sys
import logging
import json
import argparse

#get path to self and then find SDE path and PythonLibrary path
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__)) 
SDE_PATH = os.path.dirname(SCRIPT_PATH) #Path to SDE build env
PL_PATH = os.path.join(os.path.dirname(SDE_PATH), "BaseTools", "PythonLibrary")
sys.path.append(SDE_PATH)
sys.path.append(PL_PATH)

#BASECORE_PATH = os.path.dirname(SDE_PATH) # we assume that 

import SelfDescribingEnvironment
import PluginManager
from MuJunitReport import MuJunitReport
import CommonBuildEntry
import ShellEnvironment
import MuLogging
from Uefi.EdkII.PathUtilities import Edk2Path
import RepoResolver

PROJECT_SCOPES = ("project_mu",)
TEMP_MODULE_DIR = "temp_modules"

#
# To support json that has comments it must be preprocessed
#
def strip_json_from_file(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()
        out = ""
        for a in lines:
            a = a.partition("#")[0]
            a = a.rstrip()
            out += a
        return out

def get_mu_config():
    parser = argparse.ArgumentParser(description='Run the Mu Build')
    parser.add_argument ('-c', '--mu_config', dest = 'mu_config', required = True, type=str, help ='Provide the Mu config relative to the current working directory')
    parser.add_argument (
    '-p', '--pkg','--pkg-dir', dest = 'pkg', required = False, type=str,help = 'The package or folder you want to test/compile relative to the Mu Config'
    )
    args, sys.argv = parser.parse_known_args() 
    return args

#
# Main driver of Project Mu Builds
#
if __name__ == '__main__':

    #Parse command line arguments
    buildArgs = get_mu_config()
    mu_config_filepath = os.path.abspath(buildArgs.mu_config)
    mu_pk_path = buildArgs.pkg

    if mu_config_filepath is None or not os.path.isfile(mu_config_filepath):
        raise Exception("Invalid path to mu.json file for build: ", mu_config_filepath)
    
    #have a build config file
    mu_config = json.loads(strip_json_from_file(mu_config_filepath))
    WORKSPACE_PATH = os.path.realpath(os.path.join(os.path.dirname(mu_config_filepath), mu_config["RelativeWorkspaceRoot"]))

    #Setup the logging to the file as well as the console
    MuLogging.clean_build_logs(WORKSPACE_PATH)
    MuLogging.setup_logging(WORKSPACE_PATH)

    DepList = list()
    #Check Dependencies for Repo
    if "Dependencies" in mu_config:
        DepList.extend(RepoResolver.resolve(WORKSPACE_PATH,mu_config["Dependencies"]))
    
    #Get scopes from config file
    if "Scopes" in mu_config:
        PROJECT_SCOPES += tuple(mu_config["Scopes"])

    # Get Package Path from config file
    pplist = list()
    #Include packages from the config file
    if "PackagesPath" in mu_config:
        for a in mu_config["PackagesPath"]:
            # special entry that puts the directory of the repo config file in the package path list
            if(a.lower() == 'self'):
                pplist.append(os.path.dirname(mu_config_filepath))
            else:
                pplist.append(a)

    #add any deps to package path
    pplist.extend(DepList)


    #make Edk2Path object to handle all path operations 
    edk2path = Edk2Path(WORKSPACE_PATH, pplist)

    logging.info("Running ProjectMu Build: {0}".format(mu_config["Name"]))
    logging.info("WorkSpace: {0}".format(edk2path.WorkspacePath))
    logging.info("Package Path: {0}".format(edk2path.PackagePathList))
    
    #which package to build
    packageList = mu_config["Packages"]

    #
    # If mu pk path supplied lets see if its a file system path
    # If so convert to edk2 relative path
    #
    #
    if mu_pk_path:
        #if abs path lets convert
        if os.path.isabs(mu_pk_path):
            temp = edk2path.GetEdk2RelativePathFromAbsolutePath(mu_pk_path)
            if(temp is not None):
                mu_pk_path = temp
        else: 
            #Check if relative path
            temp = os.path.join(os.getcwd(), mu_pk_path)
            temp = edk2path.GetEdk2RelativePathFromAbsolutePath(temp)
            if(temp is not None):
                mu_pk_path = temp

    # if a package is specified lets confirm its valid
    if mu_pk_path:
        if mu_pk_path in packageList:
            packageList = [mu_pk_path]

        else:
            logging.critical("Supplied Package {0} not Found".format(mu_pk_path))
            raise Exception("Supplied Package {0} not Found".format(mu_pk_path))
    
    # Bring up the common minimum environment.
    (build_env, shell_env) = SelfDescribingEnvironment.BootstrapEnvironment(edk2path.WorkspacePath, PROJECT_SCOPES)
    CommonBuildEntry.update_process(edk2path.WorkspacePath, PROJECT_SCOPES)
    env = ShellEnvironment.GetBuildVars()

    
    archSupported = " ".join(mu_config["ArchSupported"])
    env.SetValue("TARGET_ARCH", archSupported, "Platform Hardcoded")
    
    
    #Generate consumable XML object- junit format
    JunitReport = MuJunitReport()

    #Keep track of failures
    failure_num = 0
    total_num = 0

    #Load plugins
    pluginManager = PluginManager.PluginManager()
    pluginManager.SetListOfEnvironmentDescriptors(build_env.plugins)
    helper = PluginManager.HelperFunctions()
    helper.LoadFromPluginManager(pluginManager)

    for pkgToRunOn in packageList:
        #
        # run all loaded MuBuild Plugins/Tests
        #
        ts = JunitReport.create_new_testsuite(pkgToRunOn, "MuBuild.{0}.{1}".format( mu_config["GroupName"], pkgToRunOn) )
        _, loghandle = MuLogging.setup_logging(WORKSPACE_PATH,"BUILDLOG_{0}.txt".format(pkgToRunOn))
        logging.info("Package Running: {0}".format(pkgToRunOn))
        ShellEnvironment.CheckpointBuildVars()
        env = ShellEnvironment.GetBuildVars()

        pkg_config_file = edk2path.GetAbsolutePathOnThisSytemFromEdk2RelativePath(os.path.join(pkgToRunOn, pkgToRunOn + ".mu.json"))
        if(pkg_config_file):
            pkg_config = json.loads(strip_json_from_file(pkg_config_file))
        else:
            logging.info("No Pkg Config file for {0}".format(pkgToRunOn))
            pkg_config = dict()

        for Descriptor in pluginManager.GetPluginsOfClass(PluginManager.IMuBuildPlugin):
            #Get our targets
            targets = ["DEBUG"]
            if Descriptor.Obj.IsTargetDependent() and "Targets" in mu_config:
                targets = mu_config["Targets"]


            for target in targets:
                logging.info("Running {0} {1}".format(Descriptor.Name,target))
                total_num +=1
                ShellEnvironment.CheckpointBuildVars()
                env = ShellEnvironment.GetBuildVars()
            
                env.SetValue("TARGET", target, "Platform Hardcoded",)
                (testcasename, testclassname) = Descriptor.Obj.GetTestName(pkgToRunOn, env)
                tc = ts.create_new_testcase(testcasename, testclassname)
                try:
                    #   - package is the edk2 path to package.  This means workspace/packagepath relative.  
                    #   - edk2path object configured with workspace and packages path
                    #   - any additional command line args
                    #   - RepoConfig Object (dict) for the build
                    #   - PkgConfig Object (dict)
                    #   - EnvConfig Object 
                    #   - Plugin Manager Instance
                    #   - Plugin Helper Obj Instance
                    #   - testcase Object used for outputing junit results
                    rc = Descriptor.Obj.RunBuildPlugin(pkgToRunOn, edk2path, sys.argv, mu_config, pkg_config, env, pluginManager, helper, tc)
                except Exception as exp:
                    logging.critical(exp)
                    tc.SetError("Exception: {0}".format(exp), "UNEXPECTED EXCEPTION")
                    rc = 1

                if(rc != 0):
                    failure_num += 1
                    if(rc is None):
                        logging.error("Test Failed: %s returned NoneType" % Descriptor.Name)
                    else:
                        logging.error("Test Failed: %s returned %d" % (Descriptor.Name, rc))
                else:
                    logging.info("Test Success {0} {1}".format(Descriptor.Name,target))
           
                #revert to the checkpoint we created previously
                ShellEnvironment.RevertBuildVars()
        #Finished plugin loop
        
        MuLogging.stop_logging(loghandle) #stop the logging for this particularbuild file
        ShellEnvironment.RevertBuildVars()
    #Finished buildable file loop


    JunitReport.Output(os.path.join(WORKSPACE_PATH, "Build", "BuildLogs", "TestSuites.xml"))

      #Print Overall Success
    if(failure_num != 0):
        logging.critical("Overall Build Status: Error")
        logging.critical("There were {0} failures out of {1} attempts".format(failure_num,total_num))        
    else:
        logging.critical("Overall Build Status: Success")
    
