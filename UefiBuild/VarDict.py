## @file VarDict.py
# This module contains code for the old EnvDict.
# Why is it here... and renamed? Why do you ask so many questions?
#
##
# Copyright (c) 2017, Microsoft Corporation
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
from __future__ import print_function
import logging


class EnvEntry(object):
    def __init__(self, value, comment, overridable=False):
        self.Value = value
        self.Comment = comment
        self.Overrideable = overridable

    def PrintEntry(self, f=None):
        if(f == None):
            print("Value: %s" % self.Value, file=f)
            print("Comment: %s" % self.Comment, file=f)
            if(self.Overrideable):
                print("Value overridable", file=f)
            print("**********************", file=f)
    #
    # Function used to override the value if option allows it
    #
    def SetValue(self, value, comment, overridable = False):
        if (value == self.Value):
            return True

        if(not self.Overrideable):
            logging.debug("Can't set value [%s] as it isn't overrideable. Previous comment %s" % (value,self.Comment))
            return False

        self.Value = value
        self.Comment = comment
        self.Overrideable = overridable
        return True

    def GetValue(self):
        return self.Value


class VarDict(object):
    def __init__(self):
        self.Logger = logging.getLogger("EnvDict")
        self.Dstore = {}     #a set of envs

    def GetEntry(self, key):
        return self.Dstore.get(key.upper())

    def __copy__(self):
        new_copy = VarDict()
        new_copy.Logger = self.Logger

        new_copy.Dstore = {}
        for key in self.Dstore:
            entry = self.GetEntry(key)
            value = entry.Value
            comment = entry.Comment
            override = entry.Overrideable
            new_copy.SetValue(key,value,comment,override)
        return new_copy


    def GetValue(self, k):
        key = k.upper()
        en = self.GetEntry(key)
        if(en != None):
            self.Logger.debug("Key %s found.  Value %s" % (key, en.GetValue()))
            return en.GetValue()
        else:
            self.Logger.debug("Key %s not found" % key)
            return None

    def SetValue(self, k, v, comment, overridable=False):
        key = k.upper()
        en = self.GetEntry(key)
        value = str(v)
        self.Logger.debug("Trying to set key %s to value %s" % (k, v))
        if(en == None):
            #new entry
            en = EnvEntry(value, comment, overridable)
            self.Dstore[key] = en
            return True
        
        return en.SetValue(value, comment, overridable)


    #
    # function used to get a build var value for given key and buildtype
    #
    # if BuildType is None 
    # Build vars are defined by vars that start with BLD_
    #  BLD_*_<YOUR KEY HERE> means all build types
    #  BLD_DEBUG_<YOUR KEY HERE> means build of debug type
    #  BLD_RELEASE_<YOUR KEY HERE> means build of release type
    #  etc
    #
    def GetBuildValue(self, key, BuildType=None):
        rv = None
        
        if(BuildType == None):
            BuildType = self.GetValue("TARGET")
        
        if(BuildType == None):
            logging.debug("GetBuildValue - Invalid Parameter BuildType is None and Target Not set. Key is: " + key)
            return None

        if(key == None):
            logging.debug("GetBuildValue - Invalid Parameter key is None. BuildType is: " + BuildType)
            return None

        
        ty = BuildType.upper().strip()
        tk = key.upper().strip()
        # see if specific 
        k = "BLD_" + ty + "_" + tk
        rv = self.GetValue(k)
        if(rv == None):
            #didn't fine build type specific so check for generic
            k = "BLD_*_" + tk
            rv = self.GetValue(k)

        #return value...if not found should return None
        return rv

    #
    # function used to get a dictionary for all build vars
    #
    # Build vars are defined by vars that start with BLD_
    #  BLD_*_<YOUR VAR HERE> means all build types
    #  BLD_DEBUG_<YOUR VAR HERE> means build of debug type
    #  BLD_RELEASE_<YOUR VAR HERE> means build of release type
    #  etc
    #
    def GetAllBuildKeyValues(self, BuildType=None):
        l = {}
        if(BuildType == None):
            BuildType = self.GetValue("TARGET")

        if(BuildType == None):
            logging.debug("GetAllBuildKeyValues - Invalid Parameter BuildType is None and Target Not Set.")
            return l

        ty = BuildType.upper().strip()
        logging.debug("Getting all build keys for build type " + ty)

        #get all the generic build options
        for key,value in self.Dstore.items():
            if(key.startswith("BLD_*_")):
                k = key[6:]
                l[k] = value.GetValue()
                
        #will override with specific for this build type
        #figure out offset part of key name to strip
        ks = len(ty) + 5
        for key,value in self.Dstore.items():
            if(key.startswith("BLD_" + ty + "_")):
                k = key[ks:]
                l[k] = value.GetValue()

        return l

    def PrintAll(self, fp=None):
        f = None
        if(fp is not None):
            f = open(fp, 'w')
        for key,value in self.Dstore.items():
            print("Key = %s"% key, file=f)
            value.PrintEntry(f)
        if(f):
            f.close()