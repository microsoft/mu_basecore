import os
import logging
import shutil
import ShellEnvironment
import time
from UtilityFunctions import RunCmd
try: 
    from StringIO import StringIO
except ImportError:
    from io import StringIO



class ConfMgmt():
    
    def __init__(self, OverrideConf, AdditionalTemplateConfDir):
        self.Logger = logging.getLogger("ConfMgmt")
        self.env = ShellEnvironment.GetBuildVars()
        self.__PopulateConf(OverrideConf, AdditionalTemplateConfDir)

    #
    # Compare the version of the existing conf file to the template file
    #
    def __OlderVersion(self, confFile, confTemplateFile):
        conf = 0
        template = 0

        f = open(confFile, "r")
        for l in f.readlines():
            if(l.startswith("#!VERSION=")):
                conf = float(l.split("=")[1].split()[0].strip())
                logging.debug("Conf version: %s", str(conf))
                break

        f.close()
        f = open(confTemplateFile, "r")
        for l in f.readlines():
            if(l.startswith("#!VERSION=")):
                template = float(l.split("=")[1].split()[0].strip())
                logging.debug("Template Version: %s", str(template))
                break
        f.close()

        return (conf < template)

                    
        

    def __PopulateConf(self, OverrideConf, AdditionalTemplateConfDir):
        ws = self.env.GetValue("WORKSPACE")
        #Copy Conf template files to conf if not present
        target = os.path.join(ws, "Conf", "target.txt")
        buildrules = os.path.join(ws, "Conf", "build_rule.txt")
        toolsdef = os.path.join(ws, "Conf", "tools_def.txt")

        #BaseTools Template files
        target_template = os.path.join("Conf", "target.template")
        tools_def_template = os.path.join("Conf", "tools_def.template")
        build_rules_template = os.path.join("Conf", "build_rule.template")

        outfiles = [target, toolsdef, buildrules]
        tfiles = [target_template, tools_def_template, build_rules_template]

        #check if conf exists
        if( not os.path.isdir(os.path.join(ws, "Conf"))):
            os.mkdir(os.path.join(ws, "Conf"))
        
        x = 0
        while(x < len(outfiles)):
            #check if the conf file already exists
            #don't overwrite if exists.  Popup if version is older in conf
            TemplateFilePath = ""
            ConfFilePath = outfiles[x]


            #
            # Get the Override template if it exist
            #
            if(AdditionalTemplateConfDir is not None):
                fp = os.path.join(AdditionalTemplateConfDir, tfiles[x] + ".ms")
                if os.path.isfile(fp):
                    TemplateFilePath = fp

            #
            # If not found above try MS templates
            #
            if(TemplateFilePath == ""):
                fp = os.path.join(self.env.GetValue("EDK2_BASE_TOOLS_DIR"), tfiles[x] + ".ms")
                if os.path.isfile(fp):
                    TemplateFilePath = fp

            #
            # If not found above try TianoCore Template
            #
            if(TemplateFilePath == ""):
                fp = os.path.join(self.env.GetValue("EDK2_BASE_TOOLS_DIR"), tfiles[x])
                if TemplateFilePath == "" and os.path.isfile(fp):
                    TemplateFilePath = fp

            #
            # Check to see if found yet -- No more options so now we are broken
            #
            if(TemplateFilePath == ""):  
                self.Logger.critical("Failed to find Template file for %s" % outfiles[x])
                raise Exception("Template File Missing" , outfiles[x])
            else:
                self.Logger.debug("Conf file template: [%s]", TemplateFilePath)

            #Check to see if we need the template
            if(not os.path.isfile(outfiles[x])):
                #file doesn't exist.  copy template
                self.Logger.debug("%s file not found.  Creating from Template file %s" % (outfiles[x], TemplateFilePath ))
                shutil.copy2(TemplateFilePath, outfiles[x])

            elif(OverrideConf):
                self.Logger.debug("%s file replaced as requested" % outfiles[x])
                shutil.copy2(TemplateFilePath, outfiles[x])
            else:
                #Both file exists.  Do a quick version check
                if(self.__OlderVersion(outfiles[x], TemplateFilePath)):
                    #Conf dir is older.  Warn user.
                    self.Logger.critical("Conf file [%s] out-of-date.  Please update your conf files!  Sleeping 30 seconds to encourage update....", outfiles[x])
                    time.sleep(30)
                else:
                    self.Logger.debug("Conf file [%s] up-to-date", outfiles[x])
                    
            x = x + 1
        #end of while loop

    def ToolsDefConfigure(self):
        Tag = self.env.GetValue("TOOL_CHAIN_TAG")
        if (Tag is not None) and (Tag.upper().startswith("VSLATEST")):
            self.Logger.debug("Must find latest VS toolchain")
            cmd = "VsWhere -latest -nologo -property installationPath"
            a = StringIO()
            ret = RunCmd(cmd, outstream=a)
            if(ret != 0):
                self.Logger.critical("Failed in VsWhere %d to get install dir" % ret)
                a.close()
                return ret
            p1 = a.getvalue().strip()
            a.close()
            self.Logger.debug("VS150INSTALLPATH is %s" % p1)
            os.environ["VS150INSTALLPATH"] = p1
            #now get vc version
            p2 = os.path.join(p1, "VC", "Tools", "MSVC")
            if not os.path.isdir(p2):
                self.Logger.critical("Failed to find VC tools.  Might need to check for VS install")
                return -5
            newest = os.listdir(p2)[-1]  #get last in list
            self.Logger.debug("VS150TOOLVER is %s" % newest)
            os.environ["VS150TOOLVER"] = newest.strip()
        else:
            self.Logger.debug("Tool Chain Tag not set or not vs latest")
        return 0

