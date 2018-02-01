#
## Utility Functions to support re-use in python scripts.  
##
## Includes functions for running external commands, etc
##
## Copyright Microsoft Corporation, 2017
##
from __future__ import print_function  #support Python3 and 2 for print
import os
import logging
import datetime
import shutil
import threading
import subprocess


####
# Helper to allow Enum type to be used which allows better code readability
#
# ref: http://stackoverflow.com/questions/36932/how-can-i-represent-an-enum-in-python
####
class Enum(tuple): __getattr__ = tuple.index


####
# Class to support running commands from the shell in a python environment. 
# Don't use directly.  
# 
# PropagatingThread copied from sample here:
# https://stackoverflow.com/questions/2829329/catch-a-threads-exception-in-the-caller-thread-in-python
####
class PropagatingThread(threading.Thread):
    def run(self):
        self.exc = None
        try:
            if hasattr(self, '_Thread__target'):
                # Thread uses name mangling prior to Python 3.
                self.ret = self._Thread__target(*self._Thread__args, **self._Thread__kwargs)
            else:
                self.ret = self._target(*self._args, **self._kwargs)
        except BaseException as e:
            self.exc = e

    def join(self, timeout=None):
        super(PropagatingThread, self).join()
        if self.exc:
            raise self.exc
        return self.ret


####
# Helper functions for running commands from the shell in python environment
# Don't use directly
#
# process output stream and write to log.
# part of the threading pattern.
#
#  http://stackoverflow.com/questions/19423008/logged-subprocess-communicate
####
def reader(filepath, outstream, stream):
    f = None
    #open file if caller provided path
    if(filepath):
        f = open(filepath, "w")

    while True:
        s = stream.readline().decode()
        if not s:
            break
        if(f is not None):
            #write to file if caller provided file
            f.write(s)
        if(outstream is not None):
            #write to stream object if caller provided object
            outstream.write(s)
        logging.info(s.rstrip())
    stream.close()
    if(f is not None):
        f.close()

####
# Run a shell commmand and print the output to the log file
# This is the public function that should be used to run commands from the shell in python environment
# @param cmd - cmd string to run including parameters
# @param capture - boolean to determine if caller wants the output captured in any format.
# @param workingdir - path to set to the working directory before running the command.
# @param outfile - capture output to file of given path.
# @param outstream - capture output to a stream.  
#
# @return returncode of called cmd
####
def RunCmd(cmd, capture=True, workingdir=None, outfile=None, outstream=None):
    starttime = datetime.datetime.now()
    logging.debug("Cmd to run is: " + cmd)
    logging.info("------------------------------------------------")
    logging.info("--------------Cmd Output Starting---------------")
    logging.info("------------------------------------------------")
    c = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=workingdir)
    if(capture):
        outr = PropagatingThread(target=reader, args=(outfile, outstream, c.stdout,))
        outr.start()
        c.wait()
        outr.join()
    else:
        c.wait()
            
    endtime = datetime.datetime.now()
    delta = endtime - starttime
    logging.info("------------------------------------------------")
    logging.info("--------------Cmd Output Finished---------------")
    logging.info("--------- Running Time (mm:ss): {0[0]:02}:{0[1]:02} ----------".format(divmod(delta.seconds, 60)))
    logging.info("------------------------------------------------")
    return c.returncode

####
# Locally Sign input file using Windows SDK signtool.  This will use a local Pfx file.  
# WARNING!!! : This should not be used for production signing as that process should follow stronger security practices (HSM / smart cards / etc)
# 
#  Signing is in format specified by UEFI authentacted variables 
####
def DetachedSignWithSignTool(SignToolPath, ToSignFilePath, SignatureOutputFile, PfxFilePath, PfxPass=None, Oid="1.2.840.113549.1.7.2"):

    #check signtool path
    if not os.path.exists(SignToolPath):
        logging.error("Path to signtool invalid.  %s" % SignToolPath)
        return -1

    OutputDir = os.path.dirname(SignatureOutputFile)
    #Signtool docs https://docs.microsoft.com/en-us/dotnet/framework/tools/signtool-exe
    #Signtool parameters from https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/secure-boot-key-generation-and-signing-using-hsm--example
    # Search for "Secure Boot Key Generation and Signing Using HSM"
    cmd = SignToolPath + ' sign /fd sha256 /p7ce DetachedSignedData /p7co ' + Oid + ' /p7 "' + OutputDir + '" /f "' + PfxFilePath + '"'
    if PfxPass is not None:
        #add password if set
        cmd = cmd + ' /p ' + PfxPass
    cmd = cmd + ' /debug /v "' + ToSignFilePath + '" '
    ret = RunCmd(cmd)
    if(ret != 0):
        logging.error("Signtool failed %d" % ret)
        return ret
    signedfile = os.path.join(OutputDir, os.path.basename(ToSignFilePath) + ".p7")
    if(not os.path.isfile(signedfile)):
        raise Exception("Output file doesn't eixst %s" % signedfile)

    shutil.move(signedfile, SignatureOutputFile)
    return ret

####
# Locally Sign input file using Windows SDK signtool.  This will use a local Pfx file.  
# WARNING!!! : This should not be used for production signing as that process should follow stronger security practices (HSM / smart cards / etc)
# 
#  Signing is catalog format which is an attached signature
####
def CatalogSignWithSignTool(SignToolPath, ToSignFilePath, PfxFilePath, PfxPass=None):

    #check signtool path
    if not os.path.exists(SignToolPath):
        logging.error("Path to signtool invalid.  %s" % SignToolPath)
        return -1

    OutputDir = os.path.dirname(ToSignFilePath)
    #Signtool docs https://docs.microsoft.com/en-us/dotnet/framework/tools/signtool-exe
    #todo: link to catalog signing documentation
    cmd = SignToolPath + " sign /a /fd SHA256 /f " + PfxFilePath
    if PfxPass is not None:
        #add password if set
        cmd = cmd + ' /p ' + PfxPass
    cmd = cmd + ' /debug /v "' + ToSignFilePath + '" '
    ret = RunCmd(cmd, workingdir=OutputDir)
    if(ret != 0):
        logging.error("Signtool failed %d" % ret)
    return ret


###
# Function to print a byte list as hex and optionally output ascii as well as
# offset within the buffer
###
def PrintByteList(ByteList, IncludeAscii=True, IncludeOffset=True, IncludeHexSep=True, OffsetStart=0):
    Ascii = ""
    for index in range(len(ByteList)):
        #Start of New Line
        if(index % 16 == 0):
            if(IncludeOffset):
                print("0x%04X -" % (index + OffsetStart), end='')

        #Midpoint of a Line
        if(index % 16 == 8):
            if(IncludeHexSep):
                print(" -", end='')

        #Print As Hex Byte
        print(" 0x%02X" % ByteList[index], end='')

        #Prepare to Print As Ascii
        if(ByteList[index] < 0x20) or (ByteList[index] > 0x7E):
            Ascii += "."
        else:
            Ascii += ("%c" % ByteList[index])

        #End of Line
        if(index % 16 == 15):
            if(IncludeAscii):
                print(" %s" % Ascii, end='')
            Ascii = ""
            print("")

    #Done - Lets check if we have partial
    if(index % 16 != 15):
        #Lets print any partial line of ascii
        if(IncludeAscii) and (Ascii != ""):
            #Pad out to the correct spot
            
            while(index % 16 != 15):
                print("     ", end='')
                if(index % 16 == 7):  #acount for the - symbol in the hex dump
                    if(IncludeOffset):
                        print("  ", end='')
                index += 1
            #print the ascii partial line
            print(" %s" % Ascii, end='')
            #print a single newline so that next print will be on new line
        print("")



if __name__ == '__main__':
    pass
    # Test code for printing a byte buffer
    # a = [0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d]
    # index = 0x55
    # while(index < 0x65):
    #     a.append(index)
    #     PrintByteList(a)
    #     index += 1