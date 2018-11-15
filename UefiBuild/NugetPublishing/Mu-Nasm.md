# Mu-Nasm Notes

This tool is the open source NASM assembler.  More information can be found at https://nasm.us/

## Where

Go to https://nasm.us and find the desired download.

## What

nasm.exe is the assembler.

## Version

``` cmd
nasm.exe -v
```
Nuget version is AA.BB.CC

* The version command generally outputs a version in AA.BB.CC format.  


## Process to publish new version of tool

1. Download desired version from nasm.us
2. Unzip 
3. Make a new folder (for my example I will call it "new")
4. Copy the assets to publish into this new folder (in this case just nasm.exe and ndisasm.exe)
5. Run the nasm.exe -v command to see the version.
6. Open cmd prompt in the NugetPublishing dir
7. Pack and push
  ```cmd
  NugetProducerSupport.py --Operation PackAndPush --ConfigFilePath Mu-Nasm.config.json --Version <nuget version here> --InputFolderPath <path to newly created folder here>  --ApiKey <your key here>
  ```

