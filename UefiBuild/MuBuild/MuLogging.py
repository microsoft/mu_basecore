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

    
    if filename is None:
        filename = "BUILDLOG_MASTER.txt"
    
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
    filelogger.setLevel(logging.DEBUG)
    filelogger.setFormatter(fileformatter)
    logger.addHandler(filelogger)
    logging.info("Log Started: " + datetime.strftime(datetime.now(), "%A, %B %d, %Y %I:%M%p" ))
    logging.info("Running Python version: " + str(sys.version_info))

    return logfile,filelogger

def stop_logging(loghandle):
    loghandle.close()
    logging.getLogger('').removeHandler(loghandle)
