# Mu BaseTools Win32 Notes

This is a set of compiled tools for Edk2 development on Windows.  This set has both the standard Edk2 tools as well as additional tools created for Project Mu.  

## Where

Information about the TianoCore Edk2 Basetools can be found here: 
* https://tianocore.org
* https://github.com/tianocore/edk2
* https://github.com/tianocore/edk2-BaseTools-win32

Information about Project Mu can be found here:
* https://microsoft.github.io/mu/
* https://github.com/Microsoft/mu
* https://github.com/microsoft/mu_basecore

## What

TianoCore/Project Mu Edk2 Build tools

## Version

====== TODO ======



Nuget version is AA.BB.CC

* If the version is a single number then make it the _AA_ field and use zeros for _BB.CC_
  * Example:  version command is **20160912**  then NuGet version is **20160912.0.0**
* If a version has two numbers partitioned by a "-" then make those the _AA.BB_ fields and use zero for the _CC_
  * Example: version command is **1234-56** then NuGet version is **1234.56.0**


## Process to publish new version of tool

1. Download desired version from
2. Unzip 
3. Make a new folder (for my example I will call it "new")
4. Copy the assets to publish into this new folder
5. Run the <_TOOL_> -v command to see the version.
6. Open cmd prompt in the NugetPublishing dir
7. Pack and push
  ```cmd
  NugetProducerSupport.py --Operation PackAndPush --ConfigFilePath Mu-Basetools-Win32.config.json --Version <nuget version here> --InputFolderPath <path to newly created folder here>  --ApiKey <your key here>
  ```

