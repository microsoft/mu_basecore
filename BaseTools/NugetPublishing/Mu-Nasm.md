# Mu-Nasm Notes

This tool is the open source NASM assembler.  More information can be found at <https://nasm.us/>

## Where

Go to <https://nasm.us> and find the desired download.

## What

nasm.exe is the assembler.

## Version

``` cmd
nasm.exe -v
```

Nuget version is AA.BB.CC

* The version command generally outputs a version in AA.BB.CC format.

## Process to publish new version of tool

1. Download desired version from nasm.us (Windows .exe and Linux .rpm)
2. Unzip (unzipping RPM requires fork of 7z that supports Zstandard compression)
3. Make a new folder (for my example I will call it "new")
4. Make proper subfolders for each host. (Details in NugetPublishing/ReadMe.md)
5. Copy the assets to publish into this new folder (in this case just nasm and ndisasm)
6. Run the nasm.exe -v command to see the version.
7. Open cmd prompt in the NugetPublishing dir
8. Pack and push

  ```cmd
  NugetPublishing.py --Operation PackAndPush --ConfigFilePath Mu-Nasm.config.json --Version <nuget version here> --InputFolderPath <path to newly created folder here>  --ApiKey <your key here>
  ```
