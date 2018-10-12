## @file MuLogging.py
# Handle basic logging config for Project Mu Builds
# MuBuild splits logs into a master log and per package.  
#
##
# Copyright (c) 2018, Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
import logging
import sys
from datetime import datetime
from datetime import date
import os
import shutil

def clean_build_logs(ws):
     # Make sure that we have a clean environment.
    if os.path.isdir(os.path.join(ws, "Build", "BuildLogs")):
        shutil.rmtree(os.path.join(ws, "Build", "BuildLogs"))

def setup_logging(workspace, filename=None, loghandle = None):

    if loghandle is not None:
        stop_logging(loghandle)

    logging_level = logging.DEBUG
    
    if filename is None:
        filename = "BUILDLOG_MASTER.txt"
        logging_level = logging.INFO
    
    #setup logger
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)

    if len(logger.handlers) == 0:
        #Create the main console as logger
        formatter = logging.Formatter("%(levelname)s- %(message)s")
        console = logging.StreamHandler()
        console.setLevel(logging.WARNING)
        console.setFormatter(formatter)
        logger.addHandler(console)

    
    logfile = os.path.join(workspace, "Build", "BuildLogs", filename)
    if(not os.path.isdir(os.path.dirname(logfile))):
        os.makedirs(os.path.dirname(logfile))
    
    #Create master file logger
    fileformatter = logging.Formatter("%(levelname)s - %(message)s")
    filelogger = logging.FileHandler(filename=(logfile), mode='w')
    filelogger.setLevel(logging_level)
    filelogger.setFormatter(fileformatter)
    logger.addHandler(filelogger)
    logging.info("Log Started: " + datetime.strftime(datetime.now(), "%A, %B %d, %Y %I:%M%p" ))
    logging.info("Running Python version: " + str(sys.version_info))

    return logfile,filelogger

def stop_logging(loghandle):
    loghandle.close()
    logging.getLogger('').removeHandler(loghandle)
