## @file RepoResolver.py
# This module supports Project Mu Builds 
# and gathering external dependencies (git repos). 
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
import os
import logging
import GitPython


##
# dependencies is a list of objects - it has Path, Commit, Branch,
def resolve(WORKSPACE_PATH,dependencies):
    packages = []
    for a in dependencies:
        logging.info("Checking for dependency {0}".format(a["Path"]))
        fsp = os.path.join(WORKSPACE_PATH, a["Path"])
        if os.path.isdir(fsp):
            logging.info("Dependency Exists - Leave it alone")
        else:
            #get it
            os.makedirs(fsp)
            clone_repo(fsp, a)
        #print out details
        packages.append(fsp)
        GitDetails = get_details(fsp)
        logging.info("Git Details: Url: {0} Branch {1} Commit {2}".format(GitDetails["Url"], GitDetails["Branch"], GitDetails["Commit"]))

    return packages

#Gets the details of a particular repo

def get_details(abs_file_system_path):
    repo = GitPython.Repo(abs_file_system_path)
    return {"Url": repo.url, "Branch": repo.active_branch, "Commit": repo.head}

#Clones the repo in the folder we need using the dependency object from the json
def clone_repo(abs_file_system_path, DepObj):
    dest = abs_file_system_path
    if not os.path.isdir(dest):
        os.makedirs(dest)
    shallow = True
    if "Commit" in DepObj:
        shallow = False
    repo = GitPython.Repo.clone_from(DepObj["Url"],dest, shallow = shallow)
    if "Commit" in DepObj:
        if DepObj["Commit"] == "*" or DepObj["Commit"] =="latest":
            logging.warning("Invalid commit id- please remove the commit id")
        elif not repo.checkout(DepObj["Commit"]):
            repo.checkout(DepObj["Branch"])
    elif "Branch" in DepObj:
        repo.checkout(DepObj["Branch"])
   
    return dest

