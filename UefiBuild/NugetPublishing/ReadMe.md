# NugetPublishing

Tool to help create and publish nuget package for Project Mu resources

## Usage

See NugetPublishing.py -h

## Example: Creating new config file for first use

TODO

## Example: Publishing new version of tool

Using an existing config file publish a new iasl.exe.  See the example file **iasl.config.json**
1. Download version from acpica.org
2. Unzip 
3. Make a new folder (for my example I will call it "new")
4. Copy the assets to publish into this new folder (in this case just iasl.exe)
5. Run the iasl.exe -v command to see the version.
6. Open cmd prompt in the NugetPublishing dir
7. Pack and push (here is my example command. )
  ```cmd
  NugetProducerSupport.py --Operation PackAndPush --ConfigFilePath iasl.config.json --Version 20180209.0.0 --InputFolderPath "C:\temp\iasl-win-20180209\new"
  ```

