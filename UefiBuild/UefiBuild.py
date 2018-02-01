import logging
from MultipleWorkspace import *
import ConfMgmt
import subprocess
import traceback
import shutil
import threading
import time
from datetime import datetime
from datetime import timedelta
import ShellEnvironment
from Uefi.EdkII.Parsers.TargetTxtParser import *
from Uefi.EdkII.Parsers.DscParser import *
from UtilityFunctions import RunCmd


class UefiBuilder(object):

    def __init__(self, WorkSpace, PackagesPath, args, BuildConfigFile=None):
        self.env = ShellEnvironment.GetBuildVars()
        self.mws = MultipleWorkspace()
        self.mws.setWs(WorkSpace, PackagesPath)
        self.ws = WorkSpace
        self.pp = PackagesPath
        self.Args = args
        self.SkipBuild = False
        self.SkipPreBuild = False
        self.SkipPostBuild = False
        self.FlashImage = False
        self.ShowHelpOnly = False
        self.OutputBuildEnvBeforeBuildToFile = None
        self.Clean = False
        self.CleanConf = True
        if(BuildConfigFile != None):
            self.BuildConfig = BuildConfigFile
        else:
            self.BuildConfig = os.path.join(self.ws, "BuildConfig.conf")
        self.RunCmd = RunCmd
        
        

    def Go(self):
        try:
            self.ParseForHelp()
            if(self.ShowHelpOnly == True):
                self.ShowHelp()
                return 0

            ret = self.SetEnv()
            if(ret != 0):
                logging.critical("SetEnv failed")
                return ret

            #clean
            if(self.Clean == True):
                logging.critical("Cleaning")
                ret = self.CleanTree(self.CleanConf)
                if(ret != 0):
                    logging.critical("Clean failed")
                    return ret
            
            #prebuild
            if(self.SkipPreBuild == True):
                logging.critical("Skipping Pre Build")
            else:
                ret = self.PreBuild()
                if(ret != 0):
                    logging.critical("Pre Build failed")
                    return ret

            # Output Build Environment to File - this is mostly for debug of build
            # issues or adding other build features using existing variables
            if(self.OutputBuildEnvBeforeBuildToFile is not None):
                logging.critical("Writing Build Env Info out to File")
                logging.debug("Found an Output Build Env File: " + self.OutputBuildEnvBeforeBuildToFile)
                self.env.PrintAll(self.OutputBuildEnvBeforeBuildToFile)

            if(self.env.GetValue("GATEDBUILD") is not None) and (self.env.GetValue("GATEDBUILD").upper() == "TRUE"):
                ShouldGatedBuildRun = self.PlatformGatedBuildShouldHappen()
                logging.debug("Platform Gated Build Should Run returned: %s" % str(ShouldGatedBuildRun))
                if( self.SkipBuild == False):
                    self.SkipBuild = not ShouldGatedBuildRun
                if(self.SkipPostBuild == False):
                    self.SkipPostBuild = not ShouldGatedBuildRun

            #build
            if(self.SkipBuild == True):
                logging.critical("Skipping Build")
            else:
                ret = self.Build()

                if(ret != 0):
                    logging.critical("Build failed")
                    return ret
                
                #Copy PDBs over to flat dir for symbol server transfer
                self.FlattenPdb()

            #postbuild
            if(self.SkipPostBuild == True):
                logging.critical("Skipping Post Build")
            else:
                ret = self.PostBuild()
                if(ret != 0):
                    logging.critical("Post Build failed")
                    return ret

            #flash
            if(self.FlashImage == True):
                logging.critical("Flashing Image")
                ret = self.FlashRomImage()
                if(ret != 0):
                    logging.critical("Flash Image failed")
                    return ret

        except:            
            logging.critical("Build Process Exception")
            logging.debug(traceback.format_exc())
            return -1

        return 0

    def FlattenPdb(self):
        #Debug/Release
        BuildType = self.env.GetValue("TARGET")
        #Path to Build output
        BuildPath = self.env.GetValue(BuildType + "_BUILD_OUTPUT_BASE")
        #Path to where the PDBs will be stored
        PDBpath = os.path.join(BuildPath, "PDB")

        IgnorePdbs = ['vc1']  #make lower case

        try:
            if not os.path.isdir(PDBpath):
                os.mkdir(PDBpath)
        except:
            logging.critical("Error making PDB directory")

        logging.critical("Copying PDBs to flat directory")
        for dirpath, dirnames, filenames in os.walk(BuildPath):
            if PDBpath in dirpath:
                continue
            for filename in filenames:
                fnl = filename.strip().lower()
                if(fnl.endswith(".pdb")):
                    if(any(e for e in IgnorePdbs if e in fnl)):
                        logging.debug("Flatten PDB - Ignore Pdb: %s" % filename)
                    else:
                        shutil.copy(os.path.join(dirpath, filename), os.path.join(PDBpath, filename))
                    

    def CleanTree(self, RemoveConfTemplateFilesToo=False):
        ret = 0
        #loop thru each build target set.  
        BuildType = self.env.GetValue("TARGET")
        logging.critical("Cleaning All Output for %s Build" % BuildType)

        d = self.env.GetValue(BuildType + "_BUILD_OUTPUT_BASE")
        if(os.path.isdir(d)):
            logging.debug("Removing [%s]", d)
            # if the folder is opened in Explorer do not fail the entire Rebuild
            try:
                shutil.rmtree(d)
            except WindowsError as wex:
                logging.debug(wex)

        else:
            logging.debug("Directory [%s] already clean" % d)

        #delete the conf .dbcache       
        #this needs to be removed in case build flags changed
        d = os.path.join(self.ws, "Conf", ".cache")
        if(os.path.isdir(d)):
            shutil.rmtree(d)
            logging.debug("Removing [%s]" % d)

        if(RemoveConfTemplateFilesToo):
            for a in ["target.txt", "build_rule.txt", "tools_def.txt"]:
                d = os.path.join(self.ws, "Conf", a)
                if(os.path.isfile(d)):
                    os.remove(d)
                    logging.debug("Removing [%s]" % d)

        return ret
                

    #
    # Build step
    #
    def Build(self): 
        BuildType = self.env.GetValue("TARGET")
        logging.critical("Running Build %s" % BuildType)
            
        cmd = "Build "
        #set target, arch toolchain
        cmd = cmd + " -p " + self.env.GetValue("ACTIVE_PLATFORM")
        cmd = cmd + " -b " + BuildType
        cmd = cmd + " -t " + self.env.GetValue("TOOL_CHAIN_TAG")

        #Set the arch flags.  Multiple are split by space
        rt = self.env.GetValue("TARGET_ARCH").split(" ")
        for t in rt:
            cmd = cmd + " -a " + t
            
        #get the report options and setup the build command
        if(self.env.GetValue("BUILDREPORTING") == "TRUE"):
            cmd = cmd + " -y " + self.env.GetValue("BUILDREPORT_FILE")
            rt = self.env.GetValue("BUILDREPORT_TYPES").split(" ")
            for t in rt:
                cmd = cmd + " -Y " + t

        #add special processing to handle building a single module
        mod = self.env.GetValue("BUILDMODULE")
        if( mod != None and len(mod.strip()) > 0):
            cmd = cmd + " -m " + mod
            logging.critical("Single Module Build: " + mod )
            self.SkipPostBuild = True
            self.FlashImage = False

        #attach the generic build vars
        buildvars = self.env.GetAllBuildKeyValues(BuildType)
        for key,value in buildvars.items():
            cmd = cmd + " -D " + key + "=" + value
        ret = self.RunCmd(cmd)
        if(ret != 0):
            return ret

        return 0

    def PreBuild(self):
        logging.critical("Running Pre Build")

        return self.PlatformPreBuild()

        

    def PostBuild(self):
        logging.critical("Running Post Build")

        # Default to a successful return value.
        ret = 0

        #Create size report if build reporting is on
        if(self.env.GetValue("BUILDREPORTING") == "TRUE"):
          
            cmd = "python.exe"
            cmd += " " + os.path.join(self.env.GetValue("MS_PYTHON_TOOLS_PATH"), "FdSizeReport", "FdSizeReportGenerator.py")
            cmd += " -l " + os.path.join(self.env.GetValue("BUILD_OUT_TEMP"), "FD_REPORT_VISUALIZATION.LOG") 
            cmd += " -o " + self.env.GetValue("FDSIZEREPORT_FILE")
            cmd += " -i " + self.env.GetValue("BUILDREPORT_FILE")
            if(self.env.GetValue("FLASH_DEFINITION") is not None):
                cmd += " -f " + self.mws.join(self.ws, self.env.GetValue("FLASH_DEFINITION"))
            cmd += " -w " + self.ws
            if(self.env.GetValue("PRODUCT_NAME")):
                cmd += " --product " + self.env.GetValue("PRODUCT_NAME")
            if(self.env.GetBuildValue("BUILDID_STRING")):
                cmd += " --fwVersion " + self.env.GetBuildValue("BUILDID_STRING")
            cmd += " --debug"
            self.RunCmd(cmd)

        #
        # Run the plaform post-build steps.
        #
        ret = self.PlatformPostBuild()

        return ret
    

    def SetEnv(self):
        logging.critical("Setting up the Environment")
        ShellEnvironment.GetEnvironment().set_shell_var("WORKSPACE", self.ws)
        ShellEnvironment.GetBuildVars().SetValue("WORKSPACE", self.ws, "Set in SetEnv")

        #1. Process command line parameters
        ret = self.ParseInputArgs()
        if (ret != 0):
            logging.critical("Parse Input Args Failed")
            return ret

        #look for config file and parse each line
        ret = self.ParseCustomConfigFile()
        if(ret != 0):
            logging.critical("Parse custom config file failed")
            return ret

        ShellEnvironment.GetEnvironment().set_shell_var("PACKAGES_PATH", self.pp)
        ShellEnvironment.GetBuildVars().SetValue("PACKAGES_PATH", self.pp, "Set in SetEnv")

        #process platform parameters defined in platform build file
        ret = self.SetPlatformEnv()
        if(ret != 0):
            logging.critical("Set Platform Env failed")
            return ret

        #set some basic defaults 
        self.SetBasicDefaults()

        #Handle all the template files for workspace/conf 
        e = ConfMgmt.ConfMgmt()

        #parse target file
        ret = self.ParseTargetFile()
        if(ret != 0):
            logging.critical("ParseTargetFile failed")
            return ret

        ret = e.ToolsDefConfigure()
        if(ret != 0):
            logging.critical("ParseTargetFile failed")
            return ret

        #parse DSC file
        ret = self.ParseDscFile()
        if(ret != 0):
            logging.critical("ParseDscFile failed")
            return ret

        #parse FDF file
        ret = self.ParseFdfFile()
        if(ret != 0):
            logging.critical("ParseFdfFile failed")
            return ret
        
        #set build output base envs for all builds
        self.env.SetValue("BUILD_OUT_TEMP", os.path.join(self.ws, self.env.GetValue("OUTPUT_DIRECTORY")), "Computed in SetEnv")
        
        target = self.env.GetValue("TARGET")
        self.env.SetValue(target.strip() + "_BUILD_OUTPUT_BASE", os.path.join(self.env.GetValue("BUILD_OUT_TEMP"), target + "_" + self.env.GetValue("TOOL_CHAIN_TAG")), "Computed in SetEnv")

        #We have our build target now.  Give platform build one more chance for target specific settings.
        ret = self.SetPlatformEnvAfterTarget()
        if(ret != 0):
            logging.critical("SetPlatformEnvAfterTarget failed")
            return ret

        #set the build report file
        self.env.SetValue("BUILDREPORT_FILE", os.path.join(self.env.GetValue(target + "_BUILD_OUTPUT_BASE"), "BUILD_REPORT.TXT"), True)
        #set the FdReportVisualization file
        self.env.SetValue("FDSIZEREPORT_FILE", os.path.join(self.env.GetValue(target + "_BUILD_OUTPUT_BASE"), "FD_REPORT.HTML"), True)
        
        #set environment variables for the build process
        os.environ["EFI_SOURCE"] = self.ws
        
        return 0

    def ParseCustomConfigFile(self):
        fp = self.BuildConfig
        if(os.path.isfile(fp)):
            f = open(fp, "r")
            for l in f.readlines():
                line = l.strip().partition("#")[0]
                if(len(line) < 1):
                    continue

                (key,sep,value) = line.partition('=')
                if(len(key) < 1):
                    logging.error("Key invalid in Custom Config file: " + l)
                    continue

                if(len(value) < 1):
                    logging.error("Value invalid in Custom Config File: " + l)
                    continue
                
                self.env.SetValue(key.strip().upper(), value.strip(), "From MyBuildConfig.conf")

            f.close()
            
        return 0
    
    def FlashRomImage(self):
        return self.PlatformFlashImage()
        
        

    #-----------------------------------------------------------------------
    # Methods that will be overridden by child class
    #-----------------------------------------------------------------------
    
    @classmethod
    def PlatformPreBuild(self):
        return 0

    @classmethod
    def PlatformPostBuild(self):
        return 0

    @classmethod
    def SetPlatformEnv(self):
        return 0

    @classmethod
    def SetPlatformEnvAfterTarget(self):
        return 0

    @classmethod
    def PlatformBuildRom(self):
        return 0

    @classmethod
    def PlatformFlashImage(self):
        return 0

    @classmethod
    def PlatformGatedBuildShouldHappen(self):
        return True

    #------------------------------------------------------------------------
    #  HELPER FUNCTIONS
    #------------------------------------------------------------------------
    #

    #
    # Parse the TargetText file and add them as env settings.
    # set them so they can be overridden. 
    #
    def ParseTargetFile(self):
        if(os.path.isfile(self.mws.join(self.ws, "Conf", "target.txt"))):
             #parse TargetTxt File
            logging.debug("Parse Target.txt file")
            ttp = TargetTxtParser()
            ttp.ParseFile(self.mws.join(self.ws, "Conf", "Target.txt"))
            for key,value in ttp.Dict.items():
                #set env as overrideable
                self.env.SetValue(key, value, "From Target.txt", True)

        else:
            logging.error("Failed to find target.txt file")
            return -1

        return 0


    #
    # Parse the Active platform DSC file.  This will get lots of variable info to
    # be used in the build.  This makes it so we don't have to define things twice
    #
    def ParseDscFile(self):
        if(os.path.isfile(self.mws.join(self.ws, self.env.GetValue("ACTIVE_PLATFORM")))):
             #parse DSC File
            logging.debug("Parse Active Platform DSC file")
            dscp = DscParser().SetBaseAbsPath(self.ws).SetPackagePaths(self.pp.split(";")).SetInputVars(self.env.GetAllBuildKeyValues())
            pa = self.mws.join(self.ws, self.env.GetValue("ACTIVE_PLATFORM"))
            dscp.ParseFile(pa)
            for key,value in dscp.LocalVars.items():
                #set env as overrideable
                self.env.SetValue(key, value, "From Platform DSC File", True)

        else:
            logging.error("Failed to find DSC file")
            return -1

        return 0

    #
    # Parse the Active platform FDF file.  This will get lots of variable info to
    # be used in the build.  This makes it so we don't have to define things twice
    # the FDF file usually comes from the Active Platform DSC file so it needs to 
    # be parsed first.  
    #
    def ParseFdfFile(self):
        if(self.env.GetValue("FLASH_DEFINITION") is None):
            logging.debug("No flash definition set")
            return 0
        if(os.path.isfile(self.mws.join(self.ws, self.env.GetValue("FLASH_DEFINITION")))):
            #parse the FDF file- fdf files have similar syntax to DSC and therefore parser works for both.
            logging.debug("Parse Active Flash Definition (FDF) file")
            fdfp = DscParser().SetBaseAbsPath(self.ws).SetPackagePaths(self.pp.split(";")).SetInputVars(self.env.GetAllBuildKeyValues())
            pa = self.mws.join(self.ws, self.env.GetValue("FLASH_DEFINITION"))
            fdfp.ParseFile(pa)
            for key, value in fdfp.LocalVars.items():
                self.env.SetValue(key, value, "From Platform FDF File", True)

        else:
            logging.error("Failed to find FDF file")
            return -2

        return 0

    #
    # Function used to set default values for numerous build
    # flow control variables
    #
    def SetBasicDefaults(self):
        self.env.SetValue("WORKSPACE", self.ws, "DEFAULT")
        if(self.pp is not None):
            self.env.SetValue("PACKAGES_PATH", self.pp, "DEFAULT")
        return 0

    #
    # Show the help
    #
    def ShowHelp(self):
        logging.critical("Showing help")
        print("------------------------------------------")
        print("  UefiBuild Help")
        print("------------------------------------------")
        print(" -h, --Help, -?              - Show this")
        print(" <key>=<value>               - Set an env variable for the pre/post build process")
        print(" BLD_*_<key>=<value>         - Set a build flag for all build types.  Key=value will get passed to build process")
        print(" BLD_<TARGET>_<key>=<value>  - Set a build flag for build type of <target>.  Key=value will get passed to build process for given build type")
        print(" --skipbuild                 - Skip the build process ")
        print(" --skipprebuild              - Skip prebuild process")
        print(" --skippostbuild             - Skip postbuild process")
        print(" --FlashRom                  - Flash rom after build.  Only works with single target")
        print(" --FlashOnly                 - Flash rom.  Rom must be built previously.  Only works with single target")

    #
    # Parse args looking for the help flag
    #
    def ParseForHelp(self):
        for a in self.Args[1:]:
            if(a.startswith("-")):
                if(a.upper() == "--HELP"):
                    self.ShowHelpOnly = True
                elif(a.upper() == "-H"):
                    self.ShowHelpOnly = True
                elif(a.upper() == "-?"):
                    self.ShowHelpOnly = True           
        
    #
    # Parse out any arguments so they can be set into the env
    #
    def ParseInputArgs(self):
        #skip first arg as its program name
        for a in self.Args[1:]:
            if(a.startswith("--")):
                if(a.upper() == "--SKIPBUILD"):
                    self.SkipBuild = True
                elif(a.upper() == "--SKIPPREBUILD"):
                    self.SkipPreBuild = True
                elif(a.upper() == "--SKIPPOSTBUILD"):
                    self.SkipPostBuild = True
                elif(a.upper() == "--FLASHONLY"):
                    self.SkipPostBuild = True
                    self.SkipBuild = True
                    self.SkipPreBuild = True
                    self.FlashImage = True
                elif(a.upper() == "--FLASHROM"):
                    self.FlashImage = True
                elif(a.upper() == "--CLEAN"):
                    self.Clean = True
                elif(a.upper() == "--CLEANALL"):
                    self.Clean = True
                    self.CleanConf = True
                elif(a.upper() == "--CLEANONLY"):
                    self.Clean = True
                    self.SkipBuild = True
                    self.SkipPreBuild = True
                    self.SkipPostBuild = True
                    self.FlashImage = False
                elif(a.upper().startswith("--BUILD_ENV_OUT_FILE")) and (a.count("=") == 1):
                    self.OutputBuildEnvBeforeBuildToFile = a.partition("=")[2]
                    
            elif(a.count("=") == 1):
                tokens = a.strip().split("=")
                self.env.SetValue(tokens[0], tokens[1], "From CmdLine")
            else:
                logging.critical("Unknown build parameter!!  Parameter: %s" % a)
                return -1
        return 0

    #
    # Function used to wait for file to exits before continuing.
    # Will loop until either timeout or file exists
    # @param filepath       file to wait for
    # @param timeout        max number of minutes to wait
    #
    def WaitOnBuildFile(self, filepath, timeout):
        #
        StartTime = datetime.now()
        EndTime = StartTime + timedelta(seconds=(timeout*60))
        logging.critical("Waiting up to %d minutes for file: %s" % (timeout, filepath))
        logging.critical("Time Now: " + datetime.strftime(StartTime, "%A, %B %d, %Y %I:%M%p" ))
        logging.critical("Timeout:  " + datetime.strftime(EndTime, "%A, %B %d, %Y %I:%M%p" ))
        while(True):
            if(os.path.isfile(filepath)):
                logging.debug("File Found!")
                return 0
            delta = datetime.now() - StartTime
            if(delta.seconds > (timeout *60)):
                logging.critical("Timeout waiting on %s" % filepath)
                return -100
            ms = divmod(delta.seconds, 60)
            logging.debug("File not found yet. Time: {0[0]:02}:{0[1]:02} ".format(divmod(delta.seconds, 60)))
            time.sleep(20)

