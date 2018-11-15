## @file UefiStatusCode.py
# Code to help convert an Int to StatusCode string
##
# Copyright (c) 2016, Microsoft Corporation
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
### 


##
## UefiStatusCode
##
class UefiStatusCode(object):
    #string Array
    StatusCodeStrings = ["Success", "Load Error", "Invalid Parameter", "Unsupported", "Bad BufferSize", "Buffer Too Small", "Not Ready", "Device Error", "Write Protected", "Out of Resources", "Volume Corrupt", "Volume Full",
                         "No Media", "Media Changed", "Not Found", "Access Denied", "No Response", "No Mapping", "Time Out", "Not Started", "Already Started", "Aborted", "ICMP Error", "TFTP Error", 
                         "Protocol Error", "Incompatible Error", "Security Violation", "CRC Error", "End of Media", "Reserved(29)", "Reserved(30)", "End of File", "Invalid Language", "Compromised Data"]
    def Convert32BitToString(self, i):
        #convert a 32bit value to string
        return UefiStatusCode.StatusCodeStrings[(i & 0xFFF)]

    def Convert64BitToString(self, l):
        if(l > len(UefiStatusCode.StatusCodeStrings)):
            return ""
        return UefiStatusCode.StatusCodeStrings[(l & 0xFFF)]

    def ConvertHexString64ToString(self, hexstring):
        l = long(hexstring, 16)
        return self.Convert64BitToString(l)

    def ConvertHexString32ToString(self, hexstring):
        i = long(hexstring, 16)
        return self.Convert32BitToString(i)