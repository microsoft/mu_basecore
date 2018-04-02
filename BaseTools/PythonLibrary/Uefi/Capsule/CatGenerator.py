import os
import logging
from UtilityFunctions import RunCmd


class CatGenerator(object):
  SUPPORTED_OS = ["win10"]

  def __init__(self, arch, os):
    self.Arch = arch
    self.OperatingSystem = os

  @property
  def Arch(self):
    return self._arch

  @Arch.setter
  def Arch(self, value):
    value = value.lower()
    if(value == "x64") or (value == "amd64"):  #support amd64 value so INF and CAT tools can use same arch value
      self._arch = "X64"
    elif(value == "arm"):
      self._arch = "ARM"
    else:
      logging.critical("Unsupported Architecture: %s", value)
      raise ValueError("Unsupported Architecture")

  @property
  def OperatingSystem(self):
    return self._operatingsystem

  @OperatingSystem.setter
  def OperatingSystem(self, value):
    value = value.lower()
    if(value == "win10") or (value == "10"):
      self._operatingsystem = "10"
    else:
      logging.critical("Unsupported Operating System: %s", value)
      raise ValueError("Unsupported Operating System")

  def MakeCat(self, OutputCatFile, PathToInf2CatTool=None):
    #Find Inf2Cat tool
    if(PathToInf2CatTool is None):
      PathToInf2CatTool = os.path.join(os.getenv("ProgramFiles(x86)"), "Windows Kits", "10", "bin", "x86", "Inf2Cat.exe")
      if not os.path.exists(PathToInf2CatTool):
        logging.debug("Windows Kit 10 not Found....trying 8.1")
        #Try 8.1 kit
        PathToInf2CatTool.replace("10", "8.1")

    #check if exists
    if not os.path.exists(PathToInf2CatTool):
        raise Exception("Can't find Inf2Cat on this machine.  Please install the Windows 10 WDK - https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit")
    
    OutputFolder = os.path.dirname(OutputCatFile)
    #Make Cat file
    cmd = PathToInf2CatTool + " /driver:. /os:" + self.OperatingSystem + "_" + self.Arch + " /verbose"
    ret = RunCmd(cmd, workingdir=OutputFolder)
    if(ret != 0):
        raise Exception("Creating Cat file Failed with errorcode %d" % ret)
    if(not os.path.isfile(OutputCatFile)):
        raise Exception("CAT file (%s) not created" % OutputCatFile)

    return 0
