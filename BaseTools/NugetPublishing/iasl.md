# IASL Notes

This tool is the open source ACPI compiler.  More information can be found at https://acpica.org/

## Where

For Windows Binary tools: https://acpica.org/downloads/binary-tools

## What

iasl.exe is the compiler.  

## Version

``` cmd
iasl.exe -v
```
Nuget version is AA.BB.CC

* If the version is a single number then make it the _AA_ field and use zeros for _BB.CC_
  * Example:  version command is **20160912**  then NuGet version is **20160912.0.0**
* If a version has two numbers partitioned by a "-" then make those the _AA.BB_ fields and use zero for the _CC_
  * Example: version command is **1234-56** then NuGet version is **1234.56.0**


## Process to publish new version of tool

1. Download desired version from acpica.org
2. Unzip 
3. Make a new folder (for my example I will call it "new")
4. Copy the assets to publish into this new folder (in this case just iasl.exe)
5. Run the iasl.exe -v command to see the version.
6. Open cmd prompt in the NugetPublishing dir
7. Pack and push
  ```cmd
  NugetProducerSupport.py --Operation PackAndPush --ConfigFilePath iasl.config.json --Version <nuget version here> --InputFolderPath <path to newly created folder here>  --ApiKey <your key here>
  ```

