from Tests.BaseTestLib import *
import copy


class DependencyCheckClass(BaseTestLibClass):

    def __init__(self, workspace, packagespath, args, ignorelist = None, environment = None, summary = None, xmlartifact = None):
        BaseTestLibClass.__init__(self, workspace, packagespath, args, ignorelist, environment, summary, xmlartifact)
        logging.critical("Dependency Test Loaded")

    def RunTest(self):
        overall_status = 0
        starttime = time.time()
        logging.critical("RUNNING DEPENDENCY CHECK")

        #Get current platform
        AP = self.GetActivePlatform()
        AP_Root = os.path.dirname(AP)

        DEC_Dict = dict()
        DEC_Used = list()

        #Get INF Files
        INFFiles = self.WalkDirectoryForExtension([".inf"], AP_Root, self.ignorelist)

        #For each INF file
        for file in INFFiles:
            if not file.lower() in self.ignorelist:
                #Reset parser lists and parse file
                self.ip.__init__()
                self.ip.ParseFile(file)

                Protocols = copy.copy(self.ip.ProtocolsUsed)
                Packages = copy.copy(self.ip.PackagesUsed)
                Libraries = copy.copy(self.ip.LibrariesUsed)
                Guids = copy.copy(self.ip.GuidsUsed)
                PCDs = copy.copy(self.ip.PcdsUsed)
                Ppis = copy.copy(self.ip.PpisUsed)


                #Get text of DECs used and add to dictionary for later use
                for DEC in Packages:
                    if DEC not in DEC_Dict:
                        if not DEC.lower().strip() in self.ignorelist:
                            self.decp.__init__()
                            try:
                                self.decp.ParseFile(os.path.join(self.ws, DEC))
                            except Exception as e:
                                if self.summary is not None:
                                    self.summary.AddError("DEPENDENCY: Failed to parse DEC %s. Exception: %s" % (DEC,str(e)),  2)
                                logging.error("DEPENDENCY: Failed to parse DEC %s. Exception: %s" % (DEC,str(e)))
                                continue

                            DEC_Dict[DEC] = (copy.copy(self.decp.ProtocolsUsed), 
                                            copy.copy(self.decp.LibrariesUsed),
                                            copy.copy(self.decp.GuidsUsed),
                                            copy.copy(self.decp.PcdsUsed), 
                                            copy.copy(self.decp.PPIsUsed))

                #Make sure libraries exist within DEC
                for Library in Libraries:
                    if not Library.lower().strip() in self.ignorelist:
                        found = False
                        for Package in DEC_Dict:
                            if any(s.startswith(Library.strip()) for s in DEC_Dict[Package][1]):
                                found = True
                                if Package not in DEC_Used:
                                    DEC_Used.append(Package)
                        if not found:
                            logging.critical(Library + " defined in " + file + " but not found in packages")
                            if self.summary is not None:
                                self.summary.AddError("DEPENDENCY: " + Library + " defined in " + file + " but not found in packages", 2)
                            overall_status = overall_status + 1

                #Make sure protocol exists within DEC
                for Protocol in Protocols:
                    if not Protocol.lower().strip() in self.ignorelist:
                        found = False
                        for Package in DEC_Dict:
                            if any(s.startswith(Protocol.strip()) for s in DEC_Dict[Package][0]):
                                found = True
                                if Package not in DEC_Used:
                                    DEC_Used.append(Package)
                        if not found:
                            logging.critical(Protocol + " defined in " + file + " but not found in packages")
                            if self.summary is not None:
                                self.summary.AddError("DEPENDENCY: " + Protocol + " defined in " + file + " but not found in packages", 2)
                            overall_status = overall_status + 1

                #Make sure GUID exist within DEC
                for GUID in Guids:
                    if not GUID.lower().strip() in self.ignorelist:
                        found = False
                        for Package in DEC_Dict:
                            if any(s.startswith(GUID.strip()) for s in DEC_Dict[Package][2]):
                                found = True
                                if Package not in DEC_Used:
                                    DEC_Used.append(Package)
                        if not found:
                            logging.critical(GUID + " defined in " + file + " but not found in packages")
                            if self.summary is not None:
                                self.summary.AddError("DEPENDENCY: " + GUID + " defined in " + file + " but not found in packages", 2)
                            overall_status = overall_status + 1

                #Make sure PCD exist within DEC
                for PCD in PCDs:
                    if not PCD.lower().strip() in self.ignorelist:
                        if('|' in PCD.lower().strip()):
                            continue #This is a PCD line that is setting the value. No dependency to check
                        found = False
                        for Package in DEC_Dict:
                            if any(s.startswith(PCD.strip()) for s in DEC_Dict[Package][3]):
                                found = True
                                if Package not in DEC_Used:
                                    DEC_Used.append(Package)
                        if not found:
                            logging.critical(PCD + " defined in " + file + " but not found in packages")
                            if self.summary is not None:
                                self.summary.AddError("DEPENDENCY: " + PCD + " defined in " + file + " but not found in packages", 2)
                            overall_status = overall_status + 1

                #Make sure Ppi exist within DEC
                for PPI in Ppis:
                    if not PPI.lower().strip() in self.ignorelist:
                        found = False
                        for Package in DEC_Dict:
                            if any(s.startswith(PPI.strip()) for s in DEC_Dict[Package][4]):
                                found = True
                                if Package not in DEC_Used:
                                    DEC_Used.append(Package)
                        if not found:
                            logging.critical(PPI + " defined in " + file + " but not found in packages")
                            if self.summary is not None:
                                self.summary.AddError("DEPENDENCY: " + PPI + " defined in " + file + " but not found in packages", 2)
                            overall_status = overall_status + 1

        #List all packages used in Pkg
        logging.critical("Packages declared in " + AP)
        for item in DEC_Dict:
            logging.critical(item)

        #Check that every package declared is actually used
        for Package in DEC_Dict:
            if Package not in DEC_Used:
                logging.critical(Package + " declared but never used in " + AP)
                if self.summary is not None:
                    self.summary.AddWarning("DEPENDENCY: " + Package + " declared but never used in " + AP, 3)

        # If summary object exists, add results
        if self.summary is not None:
            temp = str()
            for index,item in enumerate(DEC_Dict):
                if index == 0:
                    temp = item
                else:
                    temp = temp + ", " + item + " "
            self.summary.AddResult("Packages declared in " + AP + ": " + temp, 3)
            self.summary.AddResult(str(overall_status) + " error(s) in " + AP + " Dependency Check", 2)

        # If XML object exists, add results
        if overall_status is not 0 and self.xmlartifact is not None:
            self.xmlartifact.add_failure("DependencyCheck", "DependencyCheck " + os.path.basename(AP) + " " + self.GetTarget(),"DependencyCheck." + os.path.basename(AP), (AP + " DependencyCheck failed with " + str(overall_status) + " errors", "DEPENDENCYCHECK_FAILED"), time.time()-starttime)
        elif self.xmlartifact is not None:
            self.xmlartifact.add_success("DependencyCheck", "DependencyCheck " + os.path.basename(AP) + " " + self.GetTarget(),"DependencyCheck." + os.path.basename(AP), time.time()-starttime, "DependencyCheck Success")

        return overall_status

