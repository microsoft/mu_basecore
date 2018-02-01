##
## Script to Build CORE UEFI firmware
##
##
## Copyright Microsoft Corporation, 2015
##

IgnoreList = [  "nt32pkg.dsc",                      #NT32 pkg requires windows headers which are not supplied on build system
                "Nt32PkgMsCapsule.dsc",             #NT32 capsule pkg requires windows headers which are not supplied on build system
                "IntelFrameworkModulePkg.dsc",      #Plan to depricate
                "IntelFrameworkPkg.dsc",            #Plan to depricate
                "ArmCrashDumpDxe.dsc",               ## 
                "ArmPkg.dsc",
                "ArmPlatformPkg.dsc",
                "EmbeddedPkg.dsc",
                "MsSampleFmpDevicePkg.dsc",         #Sample package requires user input to build
                "MicrocodeCapsulePdb.dsc",          #Not buildable
                "MicrocodeCapsuleTxt.dsc",          #Not buildable
                "vtf.inf",                          #Not buildable - Shares GUID with resetvector
                "vtf0.inf",                         #Not buildable - Shares GUID with resetvector
                "microcode.inf",                    #Not buildable - No sources
                "useridentifymanagerdxe.inf",       #Template with #error
                "pwdcredentialproviderdxe.inf",     #Template with #error
                "usbcredentialproviderdxe.inf",     #Template with #error
                "openssllib",                       #Third party lib that does not follow library header practice
                "intrinsiclib",                     #Lib that does not follow library header practice
                "logodxe.inf",                      #Temporarily ignored due to idf file
                "opalpassworddxe.inf",              #Temproarily ignored awaiting refactor
                "tcg2configdxe.inf",                #Temproarily ignored awaiting refactor
                "IntSafeLibUnitTests.inf",          #Ignore this unit test for now, in future we'll ignore all of them
                "ArmMmuLib",                        #Remove this once ArmPkg is added to code tree
                "ArmPkg/ArmPkg.dec"                 #Remove this once ArmPkg is added to code tree
]


Test_List = list() #Default test list


import os, sys
import stat
from optparse import OptionParser
import logging
import subprocess
import shutil
import struct
from datetime import datetime
from datetime import date
import time
import copy
import csv
import io
import time
import pkgutil
import importlib
from UefiBuild import UefiBuilder
import Tests.BaseTestLib
from Tests.XmlArtifact import XmlOutput
import ShellEnvironment

logfile = None
loghandle = None

#--------------------------------------------------------------------------------------------------------
# Subclass the UEFI builder and add platform specific functionality.  
#
class PlatformBuilder(UefiBuilder):

    def __init__(self, workspace, packagespath, args, ignore = None, buildvars = None):
        UefiBuilder.__init__(self, workspace, packagespath, args)
        self.ignorelist = ignore
        if buildvars:
            self.env = buildvars

    def SetPlatformEnv(self):
        logging.debug("PlatformBuilder SetPlatformEnv")

        self.env.SetValue("PRODUCT_NAME", "CORE", "Platform Hardcoded")
        self.env.SetValue("TARGET_ARCH", "IA32 X64", "Platform Hardcoded")
        self.env.SetValue("LaunchBuildLogProgram", "Notepad", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnSuccess", "False", "default - do not log when successful")
        self.env.SetValue("LaunchLogOnError", "True", "default - will fail if already set", True)
        return 0

    #Run tests for platform
    def RunTests(self, test_list, summary = None, xmlartifact = None):
        overall_success = True
        if len(test_list) == 0:
            return 0

        logging.critical("\n\n------------------------------------------------------------------------------------------------------------------------------------\n")
        
        for testname,test in test_list:
            test_object = test(self.ws, self.pp, self.Args, self.ignorelist, self.env, summary, xmlartifact)
            ret = test_object.RunTest()
            if (ret != 0):
                logging.critical("Test Failure")
                overall_success = False
            else:
                logging.critical("Test Success")

            logging.critical("\n\n__________________________________________________________________\n")

        if overall_success:
            return 0
        else:
            return -1

    #------------------------------------------------------------------
    #
    # Method for the platform to check if a gated build is needed 
    # This is part of the build flow.  
    # return:
    #  True -  Gated build is needed (default)
    #  False - Gated build is not needed for this platform
    #------------------------------------------------------------------
    def PlatformGatedBuildShouldHappen(self):
        #for this product we use GIT.  
        # When a gated build is requested we check the diff between
        # HEAD and requested branch (usually master)
        # if there are changes in this platformpkg or other changes outside the 
        # that might impact this platform then a build should be done for this products
        directoriesICareAbout = list()
        directoriesICareAbout.append(os.path.dirname(self.env.GetValue("ACTIVE_PLATFORM")))

        #To optimize add directories within the cared about directories that should be ignored. . Be sure to use / instead of \
        directoriesToIgnore = ["MsBaseTools/vNextBuild"]  
        for item in self.ignorelist:
            if os.path.isdir(os.path.join(ws, item)):
                directoriesToIgnore.append(item)

        for index,item in enumerate(directoriesToIgnore):
            directoriesToIgnore[index] = item.lower()


        comparebranch = self.env.GetValue("GATEDBUILD_COMPARE_BRANCH")
        if comparebranch is None:
            logging.error("GATEDBUILD_COMPARE_BRANCH is not set")
            return True

        ret = self.GitDiffGatedBuild(directoriesICareAbout, directoriesToIgnore, comparebranch)
        if(ret == 0):
            logging.debug("All changes ignored")
            return False

        logging.debug("Return value from GitDiffGatedBuild %d" % ret)
        return True

    #
    # Use Git diff command to check the changed files for 
    # a match.  If changes occurred in matched file then 
    # return greather than 0.  If no changes where in included
    # dirs then return.  If error occurred return -1
    #
    def GitDiffGatedBuild(self, IncludeDir, ExcludeDir, CompareBranch):
        #for this product we use GIT.  
        # When a gated build is requested we check the diff between
        # HEAD and requested branch (usually master)
        # if there are changes in files that might impact this platform 
        # then a build should be done for this products.  

        #To optimize add directories within the cared about directories that should be ignored. . Be sure to use / instead of \
        logging.debug("Comparing HEAD with %s" % CompareBranch)
        cmd = "git diff --name-only HEAD.." + CompareBranch
        outp = io.BytesIO()            # git command outputs 8-bit characters rather than unicode
        rc = self.RunCmd(cmd, outstream=outp)
        if(rc == 0):
            logging.debug("git diff command returned successfully!")
        else:
            logging.critical("git diff returned error return value: %s" % str(rc))
            return -1
        if(outp.getvalue() is None):
            logging.debug("No files listed in diff")
            return 0

        files = outp.getvalue().split()
        for a in files:
            file = a.decode(sys.getdefaultencoding())
            for b in IncludeDir:
                if file.startswith(b):
                    ignoreIt = False
                    #cared about
                    for c in ExcludeDir:
                        if file.lower().startswith(c):
                            #ignore
                            ignoreIt = True
                            logging.debug("%s - Ignored" % file)
                            break
                    if(not ignoreIt):  
                        #not ignored
                        logging.debug("%s - Not ignored.  Do Gated Build" % file)
                        return 1
        #In the diff there wasn't a file that changed in a directory this
        #platform cares about that isn't ignored.  Therefore a gated build isn't needed. 
        return 0


#END OF CLASS

class Summary():
    def __init__(self):
        self.errors = list()
        self.warnings = list()
        self.results = list()
        self.layers = 0

    def PrintStatus(self, loghandle = None):
        logging.critical("\n\n\n************************************************************************************************************************************\n" + \
                            "************************************************************************************************************************************\n" + \
                            "************************************************************************************************************************************\n\n")

        logfile,loghandle = SetLogFile("BUILDLOG_SUMMARY.txt", loghandle)

        logging.critical("\n_______________________RESULTS_______________________________\n")
        for layer in self.results:
            logging.critical("")
            for result in layer:
                logging.critical(result)

        logging.critical("\n_______________________ERRORS_______________________________\n")
        for layer in self.errors:
            logging.critical("")
            for error in layer:
                logging.critical("ERROR: " + error)

        logging.critical("\n_______________________WARNINGS_____________________________\n")
        for layer in self.warnings:
            logging.critical("")
            for warning in layer:
                logging.critical("WARNING: " + warning)

    def AddError(self, error, layer = 0):
        if len(self.errors) <= layer:
            self.AddLayer(layer)
        self.errors[layer].append(error)

    def AddWarning(self, warning, layer = 0):
        if len(self.warnings) <= layer:
            self.AddLayer(layer)
        self.warnings[layer].append(warning)

    def AddResult(self, result, layer = 0):
        if len(self.results) <= layer:
            self.AddLayer(layer)
        self.results[layer].append(result)

    def AddLayer(self, layer):
        self.layers = layer
        while len(self.results) <= layer:
            self.results.append(list())

        while len(self.errors) <= layer:
            self.errors.append(list())

        while len(self.warnings) <= layer:
            self.warnings.append(list())

    def NumLayers(self):
        return self.layers

#Implementation for platform wide tests
def RunPlatformTests(test_list, workpath, packagepath, ignore_list = None):
    pass


##
# Add a filehandler to the current logger
# If a loghandle is passed in then close that loghandle
##
def SetLogFile(filename, loghandle = None):
    if loghandle is not None:
        loghandle.close()
        logging.getLogger('').removeHandler(loghandle)

    logfile = os.path.join(ws, "Build", "BuildLogs", filename)
    if(not os.path.isdir(os.path.dirname(logfile))):
        os.makedirs(os.path.dirname(logfile))

    filelogger = logging.FileHandler(filename=(logfile), mode='w')
    filelogger.setLevel(logging.DEBUG)
    filelogger.setFormatter(logging.Formatter("%(levelname)s - %(message)s"))
    logging.getLogger('').addHandler(filelogger)
    logging.info("Log Started: " + datetime.strftime(datetime.now(), "%A, %B %d, %Y %I:%M%p" ))
    logging.info("Running Python version: " + str(sys.version_info))
    return logfile,filelogger
    
#--------------------------------------------------------------------------------------------------------
#
# main script function.  Setup logging and init the platform builder and go
#
def main(my_workspace_path):
    global ws, pp, Test_List, logfile, loghandle, IgnoreList

    ws = my_workspace_path
    pp = my_workspace_path

    # Make sure that we have a clean environment.
    if os.path.isdir(os.path.join(ws, "Build", "BuildLogs")):
        shutil.rmtree(os.path.join(ws, "Build", "BuildLogs"))

    #Import all tests
    pkg_dir = os.path.join(os.path.dirname(__file__), "Tests")
    for (module_loader, name, ispkg) in pkgutil.iter_modules([pkg_dir]):
        importlib.import_module('.' + name, package="Tests")

    #Create list of tests
    All_Tests = list()
    for item in Tests.BaseTestLib.BaseTestLibClass.__subclasses__():
        All_Tests.append((item.__name__, item))

    #Convert ignore list to lowercase
    for index,item in enumerate(IgnoreList):
        IgnoreList[index] = item.lower()

    #setup main console as logger
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()

    #Create summary object
    summary_log = Summary()
    #Generate consumable XML object
    xml_artifact = XmlOutput()

    #Setup the main console logger differently if VSMODE is on.  
    # This allows more debug messages out
    # 
    levelset = False
    ignorefile_given = False
    testfile_given = False
    for a in sys.argv:
        if (a == "--VSMODE"):
            console.setLevel(logging.DEBUG)
            console.setFormatter(logging.Formatter("%(message)s"))
            levelset = True
            sys.argv.remove(a)
        #Check if ignore file list is provided
        elif ("ignore_file=" in a.lower()):
            try:
                IgnoreFile = open(a.split('=', 1)[1], 'r')
                CsvReader = csv.reader(IgnoreFile)
            except:
                logging.error("Could not read Ignore File")
                sys.exit(-1)
            else:
                ignorefile_given = True
                IgnoreList_Not_Flat = list(CsvReader)
                #Flatten all rows into single list and lowercase
                IgnoreList_Flat = [Item for List_Sub in IgnoreList_Not_Flat for Item in List_Sub]
                for item in IgnoreList_Flat:
                    IgnoreList.append(item.lower())
                sys.argv.remove(a)
        #Check if test list file is probided
        elif ("test_list=" in a.lower()):
            try:
                TestFile = open(a.split('=', 1)[1], 'r')
                CsvReader = csv.reader(TestFile)
            except:
                logging.error("Could not read Test File")
                sys.exit(-1)
            else:
                testfile_given = True
                TestList_Not_Flat = list(CsvReader)
                #Flatten all rows into single list and lowercase
                TestList_Flat = [Item for List_Sub in TestList_Not_Flat for Item in List_Sub]
                for item in TestList_Flat:
                    for index,test in enumerate(All_Tests):
                        if item.lower() == test[0].lower():
                            Test_List.append(All_Tests[index])

                sys.argv.remove(a)
    #Run all tests
    if "--alltests" in str(sys.argv).lower():
        Test_List = copy.copy(All_Tests)

    #Setup default logging
    if(not levelset):
        console.setLevel(logging.CRITICAL)
        console.setFormatter(formatter)
    logger.addHandler(console)

    overall_success = True
    logfile = os.path.join(ws, "Build", "BuildLogs", "BUILDLOG_MASTER.txt")
    logfile_master = copy.copy(logfile)
    if(not os.path.isdir(os.path.dirname(logfile))):
        os.makedirs(os.path.dirname(logfile))

    filelogger = logging.FileHandler(filename=(logfile), mode='w')
    filelogger.setLevel(logging.DEBUG)
    filelogger.setFormatter(formatter)
    logging.getLogger('').addHandler(filelogger)
    logging.info("Log Started: " + datetime.strftime(datetime.now(), "%A, %B %d, %Y %I:%M%p" ))
    logging.info("Running Python version: " + str(sys.version_info))

    #If Active Platform is defined no need to walk ws for DSC files
    if("active_platform" in str(sys.argv).lower()):
        #Get Filename Suffix
        AP = [x for x in sys.argv if "active_platform" in x.lower()]
        AP_String = (AP[0].strip().split("=")[1]).lower()
        File_Suffix = os.path.splitext(os.path.basename(AP_String))[0]

        #Create logfile for active platform
        logfile,loghandle = SetLogFile("BUILDLOG_" + File_Suffix + ".txt", loghandle)

        #Build platform
        PB = PlatformBuilder(ws, pp, sys.argv, IgnoreList)
        starttime = time.time()
        retcode = PB.Go()
        if not "--skipbuild" in str(sys.argv).lower():
            if(retcode != 0):
                logging.critical("Error")
                overall_success = False

                ##Add result to log/xml object
                summary_log.AddResult('{:50} {:>40}'.format(File_Suffix, "Compilation Error"))
                summary_log.AddError('{:50} {:>40}'.format(File_Suffix, "Compilation Error"))
                xml_artifact.add_failure("Compilation", "Compilation " + os.path.basename(AP_String) + " " + PB.env.GetValue("TARGET"), "Compilation." + os.path.basename(AP_String), (AP_String + " compilation failure", "COMPILATION_FAILED"), time.time()-starttime)
            else:
                logging.critical("Success")

                ##Add result to log/xml object
                summary_log.AddResult('{:50} {:>40}'.format(File_Suffix, "Compilation Success"))
                xml_artifact.add_success("Compilation", "Compilation " + os.path.basename(AP_String) + " " + PB.env.GetValue("TARGET"), "Compilation." + os.path.basename(AP_String), time.time()-starttime, "Compilation Success")
        else:
            xml_artifact.add_skipped("Compilation", "Compilation " + os.path.basename(AP_String) + " " + PB.env.GetValue("TARGET"), "Compilation." + os.path.basename(AP_String), 0, "Compilation Skipped")

            
        #Run Tests
        retcode = PB.RunTests(Test_List, summary_log, xml_artifact)
        if(retcode != 0):
            logging.critical("Test Error")
            summary_log.AddResult('{:50} {:>31}'.format(File_Suffix, "Test Error"), 1)
            overall_success = False
        else:
            logging.critical("All Tests Success")
            summary_log.AddResult('{:50} {:>41}'.format(File_Suffix, "All Tests Successful"), 1)
        

    #Walk ws for DSC files and run build on each of them
    #Unless they are specified in ignore file
    else:
        #Find all DSC files
        logging.critical("Walking " + ws + " for DSC files")
        DSCFiles = list()
        for Root, Dirs, Files in os.walk(ws):
            for File in Files:
                if File.lower().endswith('.dsc'):
                    if(File.lower() in IgnoreList):
                        logging.debug("%s - Ignored" % File)
                        continue
                    logging.critical(os.path.join(Root, File))
                    DSCFiles.append(os.path.join(Root, File)[len(ws)+1:])


        for File in DSCFiles:
            File_Suffix = os.path.splitext(os.path.basename(str(File)))[0]
            (logfile, loghandle) = SetLogFile("BUILDLOG_" + File_Suffix + ".txt", loghandle)
            logging.critical("\n\n====================================================================================================================================\n")
            logging.critical("Building " + File)
            
            #Set active platform to current DSC
            package_success = True
            temp_argv = copy.copy(sys.argv)
            temp_argv.append("ACTIVE_PLATFORM="+File)

            # Override the global build vars so each is local to the builder.
            local_build_vars = copy.copy(ShellEnvironment.GetBuildVars())

            #Build Platform
            PB = PlatformBuilder(ws, pp, temp_argv, IgnoreList, local_build_vars)
            starttime = time.time()
            retcode = PB.Go()
            if not "--skipbuild" in str(sys.argv).lower():
                if(retcode != 0):
                    logging.critical("Compile Error")
                    overall_success = False
                    package_success = False

                    ##Add result to log/xml object
                    summary_log.AddResult('{:50} {:>40}'.format(File, "Compilation Error"))
                    summary_log.AddError('{:50} {:>40}'.format(File,"Compilation Error"))
                    xml_artifact.add_failure("Compilation", "Compilation " + os.path.basename(File) + " " + PB.env.GetValue("TARGET"), "Compilation." + os.path.basename(File), (File + " compilation failure", "COMPILATION_FAILED"), time.time()-starttime)
                else:
                    logging.critical("Compile Success")

                    ##Add result to log/xml object
                    summary_log.AddResult('{:50} {:>40}'.format(File, "Compilation Success"))
                    xml_artifact.add_success("Compilation", "Compilation " + os.path.basename(File) + " " + PB.env.GetValue("TARGET"), "Compilation." + os.path.basename(File), time.time()-starttime, "Compilation Success")
            else:
                xml_artifact.add_skipped("Compilation", "Compilation " + os.path.basename(File) + " " + PB.env.GetValue("TARGET"), "Compilation." + os.path.basename(File), 0, "Compilation Skipped")

            #Run Tests
            retcode = PB.RunTests(Test_List, summary_log, xml_artifact)
            if(retcode != 0):
                logging.critical("Test Error")
                overall_success = False
                package_success = False
                summary_log.AddResult('{:50} {:>31}'.format(File, "Test Error"), 1)
            else:
                logging.critical("All Tests Success")
                summary_log.AddResult('{:50} {:>41}'.format(File, "All Tests Successful"), 1)

            if(package_success == False):
                logging.critical("Package Error")
                overall_success = False
            else:
                logging.critical("Package Success")
    
    
    #Print Overall Success
    if(overall_success == False):
        logging.critical("Overall Build Status: Error")
        logging.critical("Log file at " + logfile)
    else:
        logging.critical("Overall Build Status: Success")

    #Print summary struct
    summary_log.PrintStatus(loghandle)
    xml_artifact.write_file(os.path.join(ws, "Build", "BuildLogs", "TestSuites.xml"))

    #get all vars needed as we can't do any logging after shutdown otherwise our log is cleared.  
    #Log viewer
    ep = PB.env.GetValue("LaunchBuildLogProgram")
    LogOnSuccess = PB.env.GetValue("LaunchLogOnSuccess")
    LogOnError = PB.env.GetValue("LaunchLogOnError")
    
    #end logging
    logging.shutdown()
    #no more logging

    if(ep != None):
        cmd = ep + " " + logfile_master

    #
    # Conditionally launch the shell to show build log
    #
    #
    if( ((overall_success == False) and (LogOnError.upper() == "TRUE")) or (LogOnSuccess.upper() == "TRUE")):
        subprocess.Popen(cmd, shell=True)
    
    if overall_success:
        sys.exit(0)
    else:
        sys.exit(-1)


if __name__ == '__main__':
    raise RuntimeError("This script should not be called directly!")
