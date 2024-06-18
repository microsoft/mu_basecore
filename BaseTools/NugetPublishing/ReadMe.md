# NugetPublishing

Tool to help create and publish nuget packages for Project Mu resources

## Usage

See NugetPublishing.py -h  

## OPTIONAL: host_specific folders

The possible different setups for the host are: OS: Linux, Windows, Java
Architecture: x86 or ARM Highest Order Bit: 32 or 64

Before the path to the NuGet package contents is published, the Python
environment can look inside at several subfolders and decide which one to use
based on the Host OS, highest order bit available, and the architecture of the
processor. To do so, add "separated" to your flags like so:

```inf
"flags": ["host_specific"],
```

If this flag is present, the environment will make a list possible subfolders
that would be acceptable for the host machine. For this example, a 64 bit
Windows machine with an x86 processor was used:

1. Windows-x86-64
2. Windows-x86
3. Windows-64
4. x86-64
5. Windows
6. x86
7. 64

The environment will look for these folders, following this order, and select
the first one it finds. If none are found, the flag will be ignored.

## Authentication

For publishing most service providers require authentication.  The **--ApiKey**
parameter allows the caller to supply a unique key for authorization.  There are
numerous ways to authenticate. For example

* Azure Dev Ops:
  * VSTS credential manager.  In an interactive session a dialog will popup for
    the user to login
  * Tokens can also be used as the API key.  Go to your account page to generate
    a token that can push packages
* NuGet.org
  * Must use an API key.  Go to your account page and generate a key.  

## Example: Creating new config file for first use

This will create the config files and place them in the current directory:

```cmd
NugetPublishing.py --Operation New --Name iasl --Author ProjectMu --ConfigFileFolderPath . --Description "Description of item." --FeedUrl https://api.nuget.org/v3/index.json --ProjectUrl http://aka.ms/projectmu --LicenseType BSD2
```

For help run: `NugetPublishing.py --Operation New --help`

## Example: Publishing new version of tool

Using an existing config file publish a new iasl.exe.  See the example file
**iasl.config.json**

1. Download version from acpica.org
2. Unzip
3. Make a new folder (for my example I will call it "new")
4. Copy the assets to publish into this new folder (in this case just iasl.exe)
5. Run the iasl.exe -v command to see the version.
6. Open cmd prompt in the NugetPublishing dir
7. Pack and push (here is my example command. )

  ```cmd
  NugetPublishing.py --Operation PackAndPush --ConfigFilePath iasl.config.json --Version 20180209.0.0 --InputFolderPath "C:\temp\iasl-win-20180209\new"  --ApiKey <your key here>
  ```
