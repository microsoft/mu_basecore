
###
# Class that builds the XML consumable object for visual studio online
###
class XmlOutput(object):
    def __init__(self):
        self.testsuites = dict()
        self.testid = 0

    def add_testsuite(self, testsuite, package=None):
        if testsuite not in self.testsuites:
            self.testsuites[testsuite] = [  package,         #package name
                                            self.testid,     #test suite id
                                            list(),          #testcases
                                            0,               #errors
                                            0,               #tests
                                            0,               #failures
                                            0 ]              #skipped

            self.testid = self.testid + 1
            return 0
        else:
            return -1

    def add_testcase(self, testsuite, testcasename, testclassname, time=0, failure = None, error = None, system_out = None, package = None, skipped = False):
        if not testsuite in self.testsuites:
            self.add_testsuite(testsuite, package)

        testcase = [    testcasename,
                        testclassname,
                        time,
                        failure,
                        error,
                        system_out,
                        skipped]


        self.testsuites[testsuite][2].append(testcase)

        self.testsuites[testsuite][4] = self.testsuites[testsuite][4] + 1  #tests
        if error is not None:
            self.testsuites[testsuite][3] = self.testsuites[testsuite][3] + 1 #errors
        if failure is not None:
            self.testsuites[testsuite][5] = self.testsuites[testsuite][5] + 1 #failures
        if skipped is not False:
            self.testsuites[testsuite][6] = self.testsuites[testsuite][6] + 1 #skipped

    ## Function to add a successful test case to a test suite
    def add_success(self, testsuite, testcasename, testclassname, time=0, system_out = None, package = None):
        self.add_testcase(testsuite, testcasename, testclassname, time, None, None, system_out, package)

    ## Function to add a failed test case to a test suite
    def add_failure(self, testsuite, testcasename, testclassname, failure, time=0, system_out = None, package = None):
        self.add_testcase(testsuite, testcasename, testclassname, time, failure, None, system_out, package)

    ## Function to add a skipped test case to a test suite
    def add_skipped(self, testsuite, testcasename, testclassname, time = 0, system_out = None, package = None):
        self.add_testcase(testsuite, testcasename, testclassname, time, None, None, system_out, package, True)

    def write_file(self, filename="Testsuite.xml"):
        self.outfile = open(filename, 'w')
        self.outfile.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites>\n")

        for testsuite in self.testsuites:

            ### Test Suite Header
            outline = "    <testsuite id=\"" + str(self.testsuites[testsuite][1]) + "\"" \
                            " name=\"" + testsuite + "\""
            if self.testsuites[testsuite][0] is not None:
                outline = outline + " package=\"" + self.testsuites[testsuite][0] + "\""
            outline = outline + " errors=\"" + str(self.testsuites[testsuite][3]) + "\""
            outline = outline + " tests=\"" + str(self.testsuites[testsuite][4]) + "\""
            outline = outline + " failures=\"" + str(self.testsuites[testsuite][5]) + "\""
            outline = outline + " skipped=\"" + str(self.testsuites[testsuite][6]) + "\""

            self.outfile.write(outline + ">\n")

            ### Test Case
            for testcase in self.testsuites[testsuite][2]:
                outline = "        <testcase classname=\"" + testcase[1] + "\"" \
                                    " name=\"" + testcase[0] + "\"" \
                                    " time=\"" + str(testcase[2]) + "\">\n"
                if testcase[3] is not None:  #Failure
                    outline = outline + "            <failure message=\"" + testcase[3][0] + "\" type=\"" + testcase[3][1] + "\"/>\n"
                if testcase[4] is not None:  #Error
                    pass
                if testcase[5] is not None:
                    outline = outline + "            <system_out>" + testcase[5] + "</system_out>\n"

                self.outfile.write(outline)
                self.outfile.write("        </testcase>\n")

            self.outfile.write("    </testsuite>\n")

        self.outfile.write("</testsuites>")
        self.outfile.close()