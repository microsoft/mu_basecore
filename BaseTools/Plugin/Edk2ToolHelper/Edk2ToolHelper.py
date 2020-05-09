from edk2toolext.environment.plugintypes.uefi_helper_plugin import IUefiHelperPlugin
import logging
import os
import uuid
from edk2toollib.utility_functions import RunCmd
from edk2toollib.utility_functions import RunPythonScript
from edk2toollib.utility_functions import CatalogSignWithSignTool
import shutil
import datetime
from Common.Edk2.Capsule.FmpPayloadHeader  import FmpPayloadHeaderClass
from Common.Uefi.Capsule.FmpCapsuleHeader  import FmpCapsuleHeaderClass
from Common.Uefi.Capsule.CapsuleDependency  import CapsuleDependencyClass
from edk2toollib.windows.locate_tools import FindToolInWinSdk


class Edk2ToolHelper(IUefiHelperPlugin):

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
    # Depex: (optional) dependency expressions.
    #   - Format see: https://github.com/tianocore/tianocore.github.io/wiki/Fmp-Capsule-Dependency-Introduction
    ##
    @staticmethod
    def PackageMsFmpHeader(InputBin, OutputBin, VersionInt, LsvInt, Depex=''):
        # NOTE: Crash if deps are passed. Return a useful error.
        # Should not take a capsule whose Version <= LSV
        if (VersionInt < LsvInt):
            logging.error("Version number 0x%08x lower than Lowest supported version 0x%08x is not allowed!" % (VersionInt, LsvInt))
            return -1

        # Attempt to write the payload to the file.
        # This would normally
        with open(InputBin, 'rb') as in_file:
            payload_data = in_file.read()

            fmp_header = FmpPayloadHeaderClass()
            fmp_header.FwVersion              = VersionInt
            fmp_header.LowestSupportedVersion = LsvInt
            fmp_header.Payload                = payload_data

            output = fmp_header.Encode()

            #append depedency if supplied
            if len(Depex) > 0:
                fmp_depex = CapsuleDependencyClass()
                fmp_depex.Payload = output
                fmp_depex.DepexExp = Depex
                output = fmp_depex.Encode()

            with open(OutputBin, 'wb') as out_file:
                out_file.write(output)

        return 0

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
            SignToolPath = FindToolInWinSdk("signtool.exe")
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
        with open(InputBin, 'rb') as in_file:
            capsule_data = in_file.read()

            fmp_capsule = FmpCapsuleHeaderClass()
            fmp_capsule.AddPayload(uuid.UUID(FmpGuid), capsule_data)

            with open(OutputBin, 'wb') as out_file:
                out_file.write(fmp_capsule.Encode())

        return 0

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
