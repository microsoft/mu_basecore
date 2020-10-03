# NugetPublishing

Tool to help create and publish nuget packages for Project Mu resources

## Usage

See NugetPublishing.py -h  
  

## Authentication

For publishing most service providers require authentication.  The **--ApiKey** parameter allows the caller to supply a unique key for authorization.  There are numerous ways to authenticate. 
For example
* Azure Dev Ops:
  * VSTS credential manager.  In an interactive session a dialog will popup for the user to login
  * Tokens can also be used as the API key.  Go to your account page to generate a token that can push packages
* NuGet.org
  * Must use an API key.  Go to your account page and generate a key.  

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
  NugetProducerSupport.py --Operation PackAndPush --ConfigFilePath iasl.config.json --Version 20180209.0.0 --InputFolderPath "C:\temp\iasl-win-20180209\new"  --ApiKey <your key here>
  ```

