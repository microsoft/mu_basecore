##
##
## Copyright Microsoft Corporation, 2015
##


import struct
import uuid
from UtilityFunctions import *  #for printing buffer

class WinCertPkcs1(object):
    STATIC_STRUCT_SIZE= (4 + 2 + 2 + 16)
    EFI_HASH_SHA256 = uuid.UUID("{51AA59DE-FDF2-4EA3-BC63-875FB7842EE9}")  #EFI_HASH_SHA256 guid defined by UEFI spec

    def __init__(self, filestream=None):
        if(filestream == None):
            self.Hdr_dwLength = WinCertPkcs1.STATIC_STRUCT_SIZE
            self.Hdr_wRevision = WinCert.REVISION
            self.Hdr_wCertificateType = WinCert.WIN_CERT_TYPE_EFI_PKCS115
            self.HashAlgorithm = None
            self.CertData = None
        else:
            self.PopulateFromFileStream(filestream)

    def AddCertData(self, fs):
        if(self.CertData != None):
            raise Exception("Cert Data not 0")
        if(self.HashAlgorithm == None):
            raise Exception("You must set the Hash Algorithm first")
        self.CertData = fs.read()
        self.Hdr_dwLength = self.Hdr_dwLength + len(self.CertData)
    #
    # Method to un-serialize from a filestream
    #
    def PopulateFromFileStream(self, fs):
        if(fs == None):
            raise Exception("Invalid File stream")

        #only populate from file stream those parts that are complete in the file stream
        offset = fs.tell()
        fs.seek(0,2)
        end = fs.tell()
        fs.seek(offset)

        if((end - offset) < WinCertPkcs1.STATIC_STRUCT_SIZE): #size of the static header data
            raise Exception("Invalid file stream size")
        
        self.Hdr_dwLength = struct.unpack("=I", fs.read(4))[0]
        self.Hdr_wRevision = struct.unpack("=H", fs.read(2))[0]
        self.Hdr_wCertificateType = struct.unpack("=H", fs.read(2))[0]
        self.HashAlgorithm = uuid.UUID(bytes_le=fs.read(16))
        self.CertData = None

        if((end - fs.tell()) < 1):
            raise Exception("Invalid File stream. No data for signature cert data")

        if((end - fs.tell()) < (self.Hdr_dwLength - WinCertPkcs1.STATIC_STRUCT_SIZE)):
            raise Exception("Invalid file stream size")

        self.CertData = memoryview(fs.read(self.Hdr_dwLength - WinCertPkcs1.STATIC_STRUCT_SIZE))

    def Print(self):
        print("WinCertPKCS115")
        print("  Hdr_dwLength:         0x%X" % self.Hdr_dwLength)
        print("  Hdr_wRevision:        0x%X" % self.Hdr_wRevision)
        print("  Hdr_wCertificateType: 0x%X" % self.Hdr_wCertificateType)
        print("  Hash Guid:            %s" % str(self.HashAlgorithm))
        print("  CertData:             ")
        cdl = self.CertData.tolist()
		PrintByteList(cdl)
        

    def Write(self, fs):
        fs.write(struct.pack("=I", self.Hdr_dwLength))
        fs.write(struct.pack("=H", self.Hdr_wRevision))
        fs.write(struct.pack("=H", self.Hdr_wCertificateType))
        fs.write(self.HashAlgorithm.bytes_le)
        fs.write(self.CertData)

##
## WIN_CERT_UEFI_GUID
##
class WinCertUefiGuid(object):
    STATIC_STRUCT_SIZE= (4 + 2 + 2 + 16)
    PKCS7Guid = uuid.UUID("{4aafd29d-68df-49ee-8aa9-347d375665a7}")  #PKCS7 guid defined by UEFI spec

    def __init__(self, filestream=None):
        if(filestream == None):
            self.Hdr_dwLength = WinCertUefiGuid.STATIC_STRUCT_SIZE
            self.Hdr_wRevision = WinCert.REVISION
            self.Hdr_wCertificateType = WinCert.WIN_CERT_TYPE_EFI_GUID
            self.CertType = WinCertUefiGuid.PKCS7Guid
            self.CertData = None
        else:
            self.PopulateFromFileStream(filestream)

    def AddCertData(self, fs):
        if(self.CertData != None):
            raise Exception("Cert Data not 0")
        self.CertData = memoryview(fs.read())
        self.Hdr_dwLength = self.Hdr_dwLength + len(self.CertData)
    #
    # Method to un-serialize from a filestream
    #
    def PopulateFromFileStream(self, fs):
        if(fs == None):
            raise Exception("Invalid File stream")

        #only populate from file stream those parts that are complete in the file stream
        offset = fs.tell()
        fs.seek(0,2)
        end = fs.tell()
        fs.seek(offset)

        if((end - offset) < WinCertUefiGuid.STATIC_STRUCT_SIZE): #size of the static header data
            raise Exception("Invalid file stream size")
        
        self.Hdr_dwLength = struct.unpack("=I", fs.read(4))[0]
        self.Hdr_wRevision = struct.unpack("=H", fs.read(2))[0]
        self.Hdr_wCertificateType = struct.unpack("=H", fs.read(2))[0]
        self.CertType = uuid.UUID(bytes_le=fs.read(16))
        self.CertData = None

        if((end - fs.tell()) < 1):
            raise Exception("Invalid File stream. No data for signature cert data")

        if((end - fs.tell()) < (self.Hdr_dwLength - WinCertUefiGuid.STATIC_STRUCT_SIZE)):
            raise Exception("Invalid file stream size ")

        self.CertData = memoryview(fs.read(self.Hdr_dwLength - WinCertUefiGuid.STATIC_STRUCT_SIZE))

    def Print(self):
        print("WinCertUefiGuid")
        print("  Hdr_dwLength:         0x%X" % self.Hdr_dwLength)
        print("  Hdr_wRevision:        0x%X" % self.Hdr_wRevision)
        print("  Hdr_wCertificateType: 0x%X" % self.Hdr_wCertificateType)
        print("  CertType:             %s" % str(self.CertType))
        print("  CertData:             ")
        cdl = self.CertData.tolist()
		PrintByteList(cdl)
        

    def Write(self, fs):
        fs.write(struct.pack("=I", self.Hdr_dwLength))
        fs.write(struct.pack("=H", self.Hdr_wRevision))
        fs.write(struct.pack("=H", self.Hdr_wCertificateType))
        fs.write(self.CertType.bytes_le)
        fs.write(self.CertData)


class WinCert(object):
    STATIC_STRUCT_SIZE= 8
    # WIN_CERTIFICATE.wCertificateTypes UEFI Spec defined
    WIN_CERT_TYPE_NONE             = 0x0000
    WIN_CERT_TYPE_PKCS_SIGNED_DATA = 0x0002
    WIN_CERT_TYPE_EFI_PKCS115      = 0x0EF0
    WIN_CERT_TYPE_EFI_GUID         = 0x0EF1
    # Revision
    REVISION                       = 0x200

    #
    #this method is a factory 
    #
    @staticmethod
    def Factory(fs):
        if(fs == None):
            raise Exception("Invalid File stream")

        #only populate from file stream those parts that are complete in the file stream
        offset = fs.tell()
        fs.seek(0,2)
        end = fs.tell()
        fs.seek(offset)

        if((end - offset) < WinCert.STATIC_STRUCT_SIZE): #size of the static header data
            raise Exception("Invalid file stream size")
        # 1 read len
        # 2 read revision
        # 3 read cert type
        Hdr_dwLength = struct.unpack("=I", fs.read(4))[0]
        Hdr_wRevision = struct.unpack("=H", fs.read(2))[0]
        Hdr_wCertificateType = struct.unpack("=H", fs.read(2))[0]

        fs.seek(offset)

        if(Hdr_wCertificateType == WinCert.WIN_CERT_TYPE_EFI_GUID):
            return WinCertUefiGuid(fs)
        elif(Hdr_wCertificateType == WinCert.WIN_CERT_TYPE_EFI_PKCS115):
            return WinCertPkcs1(fs)
        else:
            return None