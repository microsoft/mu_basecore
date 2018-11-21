from MuEnvironment import PluginManager
import logging
import os
from MuPythonLibrary.UtilityFunctions import RunCmd
from MuPythonLibrary.UtilityFunctions import RunPythonScript
from MuPythonLibrary.UtilityFunctions import CatalogSignWithSignTool
import shutil
import datetime


class Edk2ToolHelper(PluginManager.IUefiHelperPlugin):

    def RegisterHelpers(self, obj):
        fp = os.path.abspath(__file__)
        obj.Register("PackageMsFmpHeader", Edk2ToolHelper.PackageMsFmpHeader, fp)
        obj.Register("PackageFmpImageAuth", Edk2ToolHelper.PackageFmpImageAuth, fp)
        obj.Register("PackageFmpCapsuleHeader", Edk2ToolHelper.PackageFmpCapsuleHeader, fp)
        obj.Register("PackageCapsuleHeader", Edk2ToolHelper.PackageCapsuleHeader, fp)


    ##
    # Function to Create binary with MsFmp Header prepended with data supplied
    # InputBin: Input binary to wrap with new header (file path)
    # OutputBin: file path to write Output binary to
    # VersionInt: integer parameter for the version
    # LsvInt: Integer parameter for the lowest supported version
    # DepList: (optional) list of dependences. Dep format is tuple (FmpGuidForDep, FmpIndex, IntFmpMinVersion, IntFlags ) 
    ### Dep format can change overtime.  Flags can be added for new behavior.  See the version and library implementing behavior.  
    ### V1 details.  
    ####Flag bit 0: dep MUST be in system if 1.  Otherwise dep only applied if fmp found in system.
    ####Flag bit 1: dep version MUST be exact match if 1.  Otherwise dep must be equal or greater than version.    
    ##
    @staticmethod
    def PackageMsFmpHeader(InputBin, OutputBin, VersionInt, LsvInt, DepList = []):
        logging.debug("CapsulePackage: Fmp Header")
        params = "-o " + OutputBin
        params = params + " --version " + hex(VersionInt).rstrip("L")
        params = params + " --lsv " + hex(LsvInt)
        params = params + " -p " + InputBin + " -v"
        #append depedency if supplied
        for dep in DepList:
            depGuid = dep[0]
            depIndex = int(dep[1])
            depMinVer = hex(dep[2])
            depFlag = hex(dep[3])
            logging.debug("Adding a Dependency:\n\tFMP Guid: %s \nt\tFmp Descriptor Index: %d \n\tFmp DepVersion: %s \n\tFmp Flags: %s\n" % (depGuid, depIndex, depMinVer, depFlag))
            params += " --dep " + depGuid + " " + str(depIndex) + " " + depMinVer + " " + depFlag
        ret = RunCmd("genmspayloadheader.exe", params)
        if(ret != 0):
            raise Exception("GenMsPayloadHeader Failed with errorcode %d" % ret)
        return ret

    ##
    # Function to create binary wrapped with FmpImage Auth using input supplied
    # InputBin: Input binary to wrap with new fmp image auth header (file path)
    # OutputBin: file path to write final output binary to
    # DevPfxFilePath: (optional) file path to dev pfx file to sign with.  If not supplied production signing is assumed. 
    # 
    ##
    @staticmethod
    def PackageFmpImageAuth(InputBin, OutputBin, DevPfxFilePath = None, DevPfxPassword = None, DetachedSignatureFile = None, Eku = None):
        logging.debug("CapsulePackage: Fmp Image Auth Header/Signing")
  
        #temp output dir is in the outputbin folder
        ret = 0
        TempOutDir = os.path.join(os.path.dirname(os.path.abspath(OutputBin)), "_Temp_FmpImageAuth_" + str(datetime.datetime.now().time()).replace(":", "_"))
        logging.debug("Temp Output dir for FmpImageAuth: %s" % TempOutDir)
        os.mkdir(TempOutDir)
        cmd =  "GenFmpImageAuth.py"
        params = "-o " + OutputBin
        params = params + " -p " + InputBin + " -m 1"
        params = params + " --debug"
        params = params + " -l " + os.path.join(TempOutDir, "GenFmpImageAuth_Log.log")
        if(DevPfxFilePath is not None):
            logging.debug("FmpImageAuth is dev signed. Do entire process in 1 step locally.")

             #Find Signtool 
            SignToolPath = os.path.join(os.getenv("ProgramFiles(x86)"), "Windows Kits", "8.1", "bin", "x64", "signtool.exe")
            if not os.path.exists(SignToolPath):
                SignToolPath = SignToolPath.replace('8.1', '10')
            if not os.path.exists(SignToolPath):
                raise Exception("Can't find signtool on this machine.")

            params = params + " --SignTool \"" + SignToolPath + "\""

            params = params + " --pfxfile " + DevPfxFilePath
            if( DevPfxPassword is not None):
                params += " --pfxpass " + DevPfxPassword
            if (Eku is not None):
                params += " --eku " + Eku
            ret = RunPythonScript(cmd, params, workingdir=TempOutDir)
            #delete the temp dir
            shutil.rmtree(TempOutDir, ignore_errors=True)
        else:
            #production
            logging.debug("FmpImageAuth is Production signed")

            if(DetachedSignatureFile is None):
                logging.debug("FmpImageAuth Step1: Make ToBeSigned file for production") 
                params = params + " --production"  
                ret = RunPythonScript(cmd, params, workingdir=TempOutDir)
                if(ret != 0):
                    raise Exception("GenFmpImageAuth Failed production signing: step 1.  Errorcode %d" % ret)
                #now we have a file to sign at 
                TBS = os.path.join(os.path.dirname(OutputBin), "payload.Temp.ToBeSigned")
                if(not os.path.exists(TBS)):
                    raise Exception("GenFmpImageAuth didn't create ToBeSigned file")
                os.rename(TBS, OutputBin)
            
            else:
                logging.debug("FmpImageAuth Step3: Final Packaging of production signed")
                params = params + " --production -s " + DetachedSignatureFile
                ret = RunPythonScript(cmd, params, workingdir=TempOutDir)
                #delete the temp dir
                shutil.rmtree(TempOutDir, ignore_errors=True)

        if(ret != 0):
            raise Exception("GenFmpImageAuth Failed with errorcode %d" % ret)
        return ret

    @staticmethod
    def PackageFmpCapsuleHeader(InputBin, OutputBin, FmpGuid):
        logging.debug("CapsulePackage: Fmp Capsule Header")
        params = "-o " + OutputBin
        params = params + " -p " + InputBin + " " + FmpGuid + " 1 0 -V"
        ret = RunCmd("genfmpcap.exe", params)
        if(ret != 0):
            raise Exception("GenFmpCap Failed with errorcode" % ret)
        return ret

    @staticmethod
    def PackageCapsuleHeader(InputBin, OutputBin, FmpDeviceGuid=None):
        logging.debug("CapsulePackage: Final Capsule Header")
        if(FmpDeviceGuid == None):
            logging.debug("CapsulePackage: Using default industry standard FMP guid")
            FmpDeviceGuid = "6dcbd5ed-e82d-4c44-bda1-7194199ad92a"

        params = "-o " + OutputBin
        params = params + " -g " + FmpDeviceGuid
        params = params + " --capsule -v -f " + InputBin
        params = params + " --capFlag PersistAcrossReset --capFlag InitiateReset"
        ret = RunCmd("genfv", params)
        if(ret != 0):
            raise Exception("GenFv Failed with errorcode" % ret)
        return ret
