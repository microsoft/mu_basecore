## @file MuJunitReport.py
# This module contains support for Outputting Junit xml.
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
import time

class MuError(object):
    def __init__(self, type, msg):
        self.Message = msg
        self.Type = type

class MuFailure(object):
    def __init__(self, type, msg):
        self.Message = msg
        self.Type = type

##
# Test Case class
#
## 
class MuTestCase(object):
    NEW = 1
    SKIPPED = 2
    FAILED = 3
    ERROR = 4
    SUCCESS = 5

    def __init__(self, Name, ClassName):
        self.Name = Name
        self.ClassName = ClassName
        self.Time = 0
        self.Status = MuTestCase.NEW
        
        self.FailureMsg = None
        self.ErrorMsg = None
        self._TestSuite = None
        self.StdErr = ""
        self.StdOut = ""
        self._StartTime = time.time()


    def SetFailed(self, Msg, Type):
        if(self.Status != MuTestCase.NEW):
            raise Exception("Can't Set to failed.  State must be in NEW")
        self.Time = time.time() - self._StartTime
        self.Status = MuTestCase.FAILED
        self.FailureMsg = MuFailure(Type, Msg)
    

    def SetError(self, Msg, Type):
        if(self.Status != MuTestCase.NEW):
            raise Exception("Can't Set to error.  State must be in NEW")
        self.Time = time.time() - self._StartTime
        self.Status = MuTestCase.ERROR
        self.ErrorMsg = MuError(Type, Msg)

    def SetSuccess(self):
        if(self.Status != MuTestCase.NEW):
            raise Exception("Can't Set to success.  State must be in NEW")    
        self.Status = MuTestCase.SUCCESS
        self.Time = time.time() - self._StartTime

    def SetSkipped(self):
        if(self.Status != MuTestCase.NEW):
            raise Exception("Can't Set to skipped.  State must be in NEW")    
        self.Status = MuTestCase.SKIPPED
        self.Time = time.time() - self._StartTime

    def LogStdOut(self, msg):
        self.StdOut += msg.strip() + "\n "
    
    def LogStdError(self, msg):
        self.StdErr += msg.strip() + "\n "

    def Output(self, outstream):
        outstream.write('<testcase classname="{0}" name="{1}" time="{2}">'.format(self.ClassName, self.Name, self.Time))
        if self.Status == MuTestCase.SKIPPED:
            outstream.write('<skipped />')
        elif self.Status == MuTestCase.FAILED:
            outstream.write('<failure message="{0}" type="{1}" />'.format(self.FailureMsg.Message, self.FailureMsg.Type))
        elif self.Status == MuTestCase.ERROR:
            outstream.write('<error message="{0}" type="{1}" />'.format(self.ErrorMsg.Message, self.ErrorMsg.Type))
        elif self.Status != MuTestCase.SUCCESS:
            raise Exception("Can't output a testcase {0}.{1} in invalid state {2}".format(self.ClassName, self.Name, self.Status))
        
        outstream.write('<system-out>' + self.StdOut + '</system-out>')
        outstream.write('<system-err>' + self.StdErr + '</system-err>')
        outstream.write('</testcase>')


##
# Test Suite class.  Create new suites by using the MuTestReport Object
#
#
##
class MuTestSuite(object):
    def __init__(self, Name, Package, Id):
        self.Name = Name
        self.Package = Package
        self.TestId = Id
        self.TestCases = []

    def create_new_testcase(self, name, classname):
        tc = MuTestCase(name, classname)
        self.TestCases.append(tc)
        tc._TestSuite = self
        return tc
    
    def Output(self, outstream):
        Errors = 0
        Failures = 0
        Skipped  = 0
        Tests = len(self.TestCases)

        for a in self.TestCases:
            if(a.Status == MuTestCase.FAILED):
                Failures += 1
            elif(a.Status == MuTestCase.ERROR):
                Errors += 1
            elif(a.Status == MuTestCase.SKIPPED):
                Skipped += 1
        
        outstream.write('<testsuite id="{0}" name="{1}" package="{2}" errors="{3}" tests="{4}" failures="{5}" skipped="{6}">'.format
        (self.TestId, self.Name, self.Package, Errors, Tests, Failures, Skipped))

        for a in self.TestCases:
            a.Output(outstream)
        
        outstream.write('</testsuite>')

##
# Test Report.  Top level object test repoting.
#
#
##
class MuJunitReport(object):
    def __init__(self):
        self.TestSuites = []

    def create_new_testsuite(self, name, package):
        id = len(self.TestSuites)
        ts = MuTestSuite(name, package, id)
        self.TestSuites.append(ts)
        return ts

    def Output(self, filepath):
        f = open(filepath, "w")
        f.write('')
        f.write('<?xml version="1.0" encoding="UTF-8"?>')
        f.write('<testsuites>')
        for a in self.TestSuites:
            a.Output(f)
        f.write('</testsuites>')
        f.close()
