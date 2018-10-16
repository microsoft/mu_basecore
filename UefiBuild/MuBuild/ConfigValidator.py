## @file ConfigValidator.py
# This module contains support for validating .mu.json config files
#
# Used to support CI/CD and exporting test results for other tools.
# This does test report generation without being a test runner. 
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
import os
import urllib.request as req
import urllib.parse as p

'''
Example MU CONFIG FILE
{
    "Name": "Project Mu BaseCore Repo CI Build",
    "GroupName": "BaseCore",
    "RelativeWorkspaceRoot": "",
    "Scopes": ["basecore", "corebuild"],
    "Dependencies": [
         {
            "Path": "Silicon/Arm/Tiano",
            "Url": "https://github.com/Microsoft/mu_silicon_arm_tiano.git",
            "Branch": "release/20180529"
        }
    ],
    "PackagesPath": [
        "self"
    ],
    "Packages":[
        "MdeModulePkg",
        "MdePkg",
        "MsUnitTestPkg",
        "NetworkPkg",
        "PcAtChipsetPkg",
        "PerformancePkg",
        "SecurityPkg",
        "UefiCpuPkg"
    ],
    "ArchSupported": [
        "IA32",
        "X64"
    ],
    "DependencyCheckPlugin": {
        "skip":true
    },
    "DscCheckPlugin" : {
        "Skip":true
    }
}
'''


#Checks the top level MU Config
def check_mu_confg(config,workspace,pluginList):
    def _mu_error(message):
        raise Exception("Mu Config Error: {0}".format(message))

    def _is_valid_dir(path,name):
        path = os.path.join(workspace,path)
        if not os.path.isdir(path):
            _mu_error("{0} isn't a valid directory".format(path))

    def _check_url(url):
        request = req.Request(url)
        try:
            response = req.urlopen(request)
            return True
        except:
            #The url wasn't valid
            return False
    
    def _check_packages(packages,name):
        for package in packages:            
            path = os.path.join(workspace,package)
            if not os.path.isdir(path):
                _mu_error("{0} isn't a valid package to build".format(path))
        return True

    def _is_valid_arch(targets,name):
        valid_targets = ["AARCH64","IA32","X64"]
        for target in targets:
            if not target in valid_targets:
                _mu_error("{0} is not a valid target".format(target))

    def _check_dependencies(dependencies,name):
        valid_attributes = ["Path","Url","Branch","Commit"]
        for dependency in dependencies:
            # check to make sure we have a path
            if not "Path" in dependency:
                _mu_error("Path not found in dependency {0}".format(dependency))
            # check to sure we have a valid url and we can reach it
            if not "Url" in dependency:
                _mu_error("Url not found in dependency {0}".format(dependency))
            if not _check_url(dependency["Url"]):
                _mu_error("Invalid URL {0}".format(dependency["Url"]))            
            # make sure we have a valid branch or commit
            if not "Branch" in dependency and not "Commit" in dependency:
                _mu_error("You must have a commit or a branch dependency {0}".format(dependency))
            if  "Branch" in dependency and "Commit" in dependency:
                _mu_error("You cannot have both a commit or a branch dependency {0}".format(dependency))
            #check to make sure we don't have something else in there
            for attribute in dependency:
                if not attribute in valid_attributes:
                    _mu_error("Unknown attribute {0} in dependecy".format(attribute))
        
        return True
    
    

    config_rules = {
        "required": {
            "Name": {
                "type":"str"
            }, 
            "GroupName": {
                "type":"str"
            }, 
            "Scopes": {
                "type":"list",
                "items":"str"
            },
            "ArchSupported": {
                "type":"list",
                "validator":_is_valid_arch
            },
            "RelativeWorkspaceRoot": {
                "type":"str",
                "validator":_is_valid_dir
            },
            "Targets": {
                "type":"list"
            }
        },
        "optional": {
            "Packages":{
                "type":"list",
                "items":"str",
                "validator":_check_packages
            },
            "PackagesPath": {
                "type":"list",
                "items":"str"
            },
            "Dependencies": {
                "type":"list",
                "validator":_check_dependencies
            }
        }
    }

    for plugin in pluginList:
        if "module" in plugin.descriptor:
            plugin_name = plugin.descriptor["module"]
        if "config_name" in plugin.descriptor:
            plugin_name = plugin.descriptor["config_name"]
        config_rules["optional"][plugin_name] = {
            "validator" : plugin.Obj.ValidateConfig
        }

    #check if all the requires are satisified
    for rule in config_rules["required"]:
        if not rule in config:
            _mu_error("{0} is a required attribute in your MU Config".format(rule))
        
        if "type" in config_rules["required"][rule]:
            config_type = str(type(config[rule]).__name__)
            wanted_type = config_rules["required"][rule]["type"]
            if config_type != wanted_type:
                _mu_error("{0} is a required attribute and is not the correct type. We are expecting a {1} and got a {2}".format(rule,config_type,wanted_type))

        if "validator" in config_rules["required"][rule]:
            validator = config_rules["required"][rule]["validator"]
            validator(config[rule], "Base Mu.json")

    # check optional types
    for rule in config_rules["optional"]:
        if not rule in config:
           continue
        
        if "type" in config_rules["optional"][rule]:
            config_type = str(type(config[rule]).__name__)
            wanted_type = config_rules["optional"][rule]["type"]
            if config_type != wanted_type:
                _mu_error("{0} is a optional attribute and is not the correct type. We are expecting a {1} and got a {2}".format(rule,config_type,wanted_type))

        if "validator" in config_rules["optional"][rule]:
            validator = config_rules["optional"][rule]["validator"]
            validator(config[rule],"Base mu.json")
    
    #check to make sure we don't have any stray keys in there
    for rule in config:
        if not rule in config_rules["optional"] and not rule in config_rules["required"]:
            _mu_error("Unknown parameter {0} is unexpected".format(rule))
            
    return True

'''
{
    "Defines": {
        "PLATFORM_NAME": "MdeModule",
        "DSC_SPECIFICATION": "0x00010005",
        "SUPPORTED_ARCHITECTURES": "IA32|X64|ARM|AARCH64",
        "BUILD_TARGETS": "DEBUG|RELEASE"        
    },
    "CompilerPlugin": {
        "skip":false,
        "IgnoreInf": []
    },
    "DependencyCheckPlugin":{
        "AcceptableDependencies": [
            "MdePkg/MdePkg.dec",
            "MdeModulePkg/MdeModulePkg.dec",
            "MsUnitTestPkg/MsUnitTestPkg.dec"
        ], 
        "IgnoreInf": {

        },
        "skip": false
    }
}
'''
##
#   Checks the package configuration for errors
##
def check_package_confg(name,config,pluginList):
    def _mu_error(message):
        raise Exception("Package {0} Config Error: {1}".format(name,message))

    config_rules = {
        "required": {
        },
        "optional": {            
            "Defines": {
                "type":"dict",
                "items":"str"
            }
        }
    }

    for plugin in pluginList:
        if "module" in plugin.descriptor:
            plugin_name = plugin.descriptor["module"]
        if "config_name" in plugin.descriptor:
            plugin_name = plugin.descriptor["config_name"]
        # add the validator
        config_rules["optional"][plugin_name] = {
            "validator" : plugin.Obj.ValidateConfig
        }

     #check if all the requires are satisified
    for rule in config_rules["required"]:
        if not rule in config:
            _mu_error("{0} is a required attribute in your MU Config".format(rule))
        
        if "type" in config_rules["required"][rule]:
            config_type = str(type(config[rule]).__name__)
            wanted_type = config_rules["required"][rule]["type"]
            if config_type != wanted_type:
                _mu_error("{0} is a required attribute and is not the correct type. We are expecting a {1} and got a {2}".format(rule,config_type,wanted_type))

        if "validator" in config_rules["required"][rule]:
            validator = config_rules["required"][rule]["validator"]
            validator(config[rule],name)

    # check optional types
    for rule in config_rules["optional"]:
        if not rule in config:
           continue
        
        if "type" in config_rules["optional"][rule]:
            config_type = str(type(config[rule]).__name__)
            wanted_type = config_rules["optional"][rule]["type"]
            if config_type != wanted_type:
                _mu_error("{0} is a optional attribute and is not the correct type. We are expecting a {1} and got a {2}".format(rule,config_type,wanted_type))

        if "validator" in config_rules["optional"][rule]:
            validator = config_rules["optional"][rule]["validator"]
            validator(config[rule],name)
    
    #check to make sure we don't have any stray keys in there
    for rule in config:
        if not rule in config_rules["optional"] and not rule in config_rules["required"]:
            _mu_error("Unknown parameter {0} is unexpected".format(rule))
    