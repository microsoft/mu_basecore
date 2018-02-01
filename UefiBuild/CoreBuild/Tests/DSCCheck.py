from Tests.BaseTestLib import *

class DSCCheckClass(BaseTestLibClass):

    def __init__(self, workspace, packagespath, args, ignorelist = None, environment = None, summary = None, xmlartifact = None):
        BaseTestLibClass.__init__(self, workspace, packagespath, args, ignorelist, environment, summary, xmlartifact)
        logging.critical("DSCCheck Test Loaded")

    def RunTest(self):
        overall_status = 0
        starttime = time.time()

        logging.critical("RUNNING DSC CHECK")
        AP = self.GetActivePlatform()
        AP_Root = os.path.dirname(AP)

        #Get INF Files
        INFFiles = self.WalkDirectoryForExtension([".inf"], AP_Root, self.ignorelist)
        INFFiles = [x.lower() for x in INFFiles]
        INFFiles = [os.path.basename(x) for x in INFFiles]

        self.dp.__init__()
        self.dp.ParseFile(os.path.join(self.ws, AP))

        #lowercase for matching
        self.dp.Libs = [x.lower() for x in self.dp.Libs]
        self.dp.ThreeMods = [x.lower() for x in self.dp.ThreeMods]
        self.dp.SixMods = [x.lower() for x in self.dp.SixMods]
        self.dp.OtherMods = [x.lower() for x in self.dp.OtherMods]

        #Check if INF in lib or component section
        for INF in INFFiles:
            if not any(INF.lower().strip() in x for x in self.dp.ThreeMods) \
            and not any(INF.lower().strip() in x for x in self.dp.SixMods) and not any(INF.lower().strip() in x for x in self.dp.OtherMods):
                if self.summary is not None:
                    self.summary.AddError("DSC: " + INF + " not in " + AP, 2)
                logging.critical(INF + " not in " + AP)
                overall_status = overall_status + 1

        # If summary object exists, add result
        if self.summary is not None:
            self.summary.AddResult(str(overall_status) + " error(s) in " + AP + " DSC Check", 2)

        # If XML object esists, add result
        if overall_status is not 0 and self.xmlartifact is not None:
            self.xmlartifact.add_failure("DSCCheck", "DSCCheck " + os.path.basename(AP) + " " + self.GetTarget(),"DSCCheck." + os.path.basename(AP), (AP + " DSCCheck failed with " + str(overall_status) + " errors", "DSCCHECK_FAILED"), time.time()-starttime)
        elif self.xmlartifact is not None:
            self.xmlartifact.add_success("DSCCheck", "DSCCheck " + os.path.basename(AP) + " " + self.GetTarget(),"DSCCheck." + os.path.basename(AP), time.time()-starttime, "DSCCheck Success")

        return overall_status
