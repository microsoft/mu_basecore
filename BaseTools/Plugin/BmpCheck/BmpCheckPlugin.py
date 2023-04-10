# @file BmpCheck.py
# Plugin to support checking BMP's included in the FDF for proper usage
# This makes symbol publishing easier and with the usage of
# ALT PDB PATH can shrink the size of each module.
#
##
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
###

import logging
import os
import time
try:
    from edk2toollib.uefi.edk2.path_utilities import Edk2Path
    from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
    from edk2toollib.uefi.edk2.parsers.fdf_parser import FdfParser
    from edk2toollib.uefi.edk2.parsers.dsc_parser import DscParser
    from edk2toollib.uefi import bmp_object
except Exception:
    pass


def timing(f):
    def wrap(*args):
        time1 = time.time()
        ret = f(*args)
        time2 = time.time()
        logging.debug('{:s} function took {:.3f} ms'.format(f.__name__, (time2-time1)*1000.0))

        return ret
    return wrap


# the tests that we run on the BMP object
class UefiBmpSupportTests(object):

    def __init__(self, BmpObject, max_width=0, max_height=0):
        self.Bmp = BmpObject
        self.logger = logging.getLogger(__name__)
        self.max_width = max_width
        self.max_height = max_height

    def Test1(self):
        self.logger.info("Test1: Pixel Data Size in file matches computed Size of Pixel data")
        #test1
        DataSizePerLine = ((self.Bmp.PixelWidth * self.Bmp.BitPerPixel + 31) >> 3) & (~0x3)
        DataSize2 = ((self.Bmp.PixelWidth * self.Bmp.BitPerPixel +31) / 32) * 4
        self.logger.debug("DataSize2 = {}".format(DataSize2))
        self.logger.debug(" DataSizePerLine: {}".format(DataSizePerLine))
        RawDataSize = self.Bmp.PixelHeight * DataSizePerLine
        self.logger.debug(" RawDataSize: 0x%X" % RawDataSize)
        ComputeSize = (self.Bmp.Size - self.Bmp.ImageOffset)
        self.logger.debug(" File Calculated Data Size: 0x%X" % ComputeSize)

        if(ComputeSize != RawDataSize):
            self.logger.error(" BMP Test1 - Fail")
            return 1
        else:
            self.logger.info(" BMP Test1 - Pass")
            return 0

    def Test2(self):
        self.logger.info(" BMP Test2: File Header and Img Header as expected")
        #test2
        if self.Bmp.CharB != b'B' and self.Bmp.CharB != b'B':
          self.logger.error("Logo check - B header failed {}".format(self.Bmp.CharB))
          return 1
        if self.Bmp.CharM != b'M' and self.Bmp.CharM != 'M':
          self.logger.error("Logo check - M header failed {}".format(self.Bmp.CharM))
          return 1

        self.logger.info(" Test2 - Pass")
        return 0

    def Test3(self):
        if self.max_width > 0 and BmpObj.PixelWidth > self.max_width:
            self.logger.critical("Image {} is too wide".format(BmpFilePath))
            return 1
        if self.max_height > 0 and BmpObj.PixelHeight > self.max_height:
            self.logger.critical("Image {} is too height".format(BmpFilePath))
            return 1
        return 0



class BmpCheckPlugin(IUefiBuildPlugin):

    def __init__(self):
        self.logger = logging.getLogger(__name__)

    @staticmethod
    def CheckBmp(BmpFilePath, max_width=0, max_height=0):
        if not os.path.isfile(BmpFilePath):
            return 1
        bmp = open(BmpFilePath, "rb")
        BmpObj = bmp_object.BmpObject(bmp)
        bmp.close()
        #run tests
        Tests = UefiBmpSupportTests(BmpObj)
        ret = Tests.Test1()
        ret += Tests.Test2()
        ret += Tests.Test3()
        return ret

    @timing
    def do_pre_build(self, thebuilder):
        try:
            error_count = 0
            '''
            # this scans the whole build directory for bmp's
            bmp_search_path = os.path.join(thebuilder.ws,"**","*.bmp");
            for found_item in glob.iglob(bmp_search_path, recursive=True):
              if CheckBmp(found_item):
                logging.error("{} failed image check".format(found_item))
                error_count += 1
            return error_count
            '''

            fp = FdfParser()
            dp = DscParser()

            ws = thebuilder.ws
            pp = thebuilder.pp.split(";")
            edk2 = Edk2Path(ws, pp)

            ActiveDsc = edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                thebuilder.env.GetValue("ACTIVE_PLATFORM"))
            ActiveFdf = edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                thebuilder.env.GetValue("FLASH_DEFINITION"))

            if ActiveFdf is None:
                self.logger.info("No FDF found- BMP check skipped")
                return 0
            # parse the DSC and the FDF
            dp.SetEdk2Path(edk2)
            dp.SetInputVars(thebuilder.env.GetAllBuildKeyValues()).ParseFile(ActiveDsc)  # parse the DSC for build vars
            fp.SetEdk2Path(edk2)
            fp.SetInputVars(dp.LocalVars).ParseFile(ActiveFdf)  # give FDF parser the vars from DSC

            # for each FV section in the DSC
            for FV_name in fp.FVs:
                FV_files = fp.FVs[FV_name]["Files"]
                # now look for images in each file of this FV
                for fv_file_name in FV_files:
                    fv_file = FV_files[fv_file_name]
                    if fv_file["type"].upper() != 'FREEFORM':
                        continue
                    fv_file_raw = fv_file['RAW']
                    fv_file_raw_list = []
                    if isinstance(fv_file_raw, list):
                        fv_file_raw_list = fv_file_raw
                    else:
                        fv_file_raw_list.append(fv_file_raw)
                    # for each file that is RAW type
                    for fv_file_raw_item in fv_file_raw_list:
                        # check if it ends with a bmp
                        if fv_file_raw_item.lower().endswith(".bmp"):
                            logging.debug(fv_file_raw_item)
                            BmpFilePath = edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(fv_file_raw_item)
                            logging.debug(BmpFilePath)
                            if BmpCheckPlugin.CheckBmp(BmpFilePath):  # do the check
                              self.logger.error("{} failed image check".format(fv_file_raw_item))
                              error_count += 1
            return error_count
        except:
            self.logger.warning(
                "Unable to read the FDF. Please update your Edk2-Pytools-* Packages")
            return 0
