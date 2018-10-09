# this attemps to replicate some of the interface of gitpython
import os
import logging
import subprocess
from UtilityFunctions import RunCmd

try:
    from io import StringIO
except ImportError:
    from StringIO import StringIO

class GitCommand(object):
    def __init__(self,command=""):
        self.command = command

class Repo(object):

    def __init__(self,path=None, gitCommand=GitCommand(),bare=True):
        self._path = path
        self.active_branch = None
        self.bare = bare
        self.url = None
        self.head = None
        self._update_from_git()
        
    # Updates the .git file
    def _update_from_git(self):
        if os.path.isdir(os.path.join(self._path,".git")):
            self.bare = False
        self.active_branch = self._get_branch()
        self.url = self._get_url()
        self.head = self._get_head()

    def _get_url(self):
        return_buffer = StringIO()
        cmd = "git config --get remote.origin.url"
        ret = RunCmd(cmd, workingdir=self._path,outstream=return_buffer)

        p1 = return_buffer.getvalue().strip()
        return_buffer.close()        
        return p1
    
    def _get_branch(self):        
        return_buffer = StringIO()
        cmd = "git rev-parse --abbrev-ref HEAD"
        ret = RunCmd(cmd, workingdir=self._path,outstream=return_buffer)

        p1 = return_buffer.getvalue().strip()
        return_buffer.close()        
        return p1
    
    def _get_head(self):        
        return_buffer = StringIO()
        cmd = "git rev-parse HEAD"
        ret = RunCmd(cmd, workingdir=self._path,outstream=return_buffer)

        p1 = return_buffer.getvalue().strip()
        return_buffer.close()        
        return p1

    def checkout(self,branch):
        return_buffer = StringIO()
        cmd = "git checkout %s" % branch
        ret = RunCmd(cmd, workingdir=self._path,outstream=return_buffer)

        p1 = return_buffer.getvalue().strip()
        if ret != 0:
            logging.debug(p1)
            return False

        return True

    @classmethod
    def clone_from(self,url, to_path, progress=None, env=None,shallow =False, **kwargs):
        logging.debug("Cloning {0} into {1}".format(url,to_path))
        #make sure we get the commit if 
        # use run command from utilities
        cmd = ""
        if shallow:
            cmd = "git clone --depth 1 --shallow-submodules --recurse-submodules %s %s " % (url, to_path)
        else:
            cmd = "git clone --recurse-submodules %s %s " % (url, to_path)
        ret = RunCmd(cmd)

        return Repo(to_path)

    
