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

##
# finds the module requested from the workspace- we assume that it is here at some point
def find_module(ws,module, url):
    currentDir = ws
    if os.path.isfile(currentDir):
        currentDir = os.path.dirname(currentDir)
    #only look if it explicitly exists in the workspace
    lookingPath = os.path.join(currentDir,module)
    if os.path.isdir(lookingPath):
        return lookingPath
    else:
        return None

#Gets the details of a particular repo

def get_details(abs_file_system_path):
    repo = GitPython.Repo(abs_file_system_path)
    return {"Url": repo.url, "Branch": repo.active_branch, "Commit": repo.head}

#Clones the repo in the folder we need using the dependency object from the json
def clone_repo(abs_file_system_path, DepObj):
    dest = abs_file_system_path
    if not os.path.isdir(dest):
        os.mkdirs(dest)
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

