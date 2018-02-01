##
##
## Copyright Microsoft Corporation, 2015
##
# Code to help convert an Int to StatusCode string
#


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