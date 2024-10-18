##
# This plugin generates policy header files
# from platform supplied YAML policy.
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import logging
import os
import shutil
from collections import OrderedDict
from copy import deepcopy
import xml.etree.ElementTree
import hashlib
import json
import time
import re
import xml.etree.ElementTree as ET
from edk2toolext.environment import shell_environment
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toollib.utility_functions import RunPythonScript
from edk2toollib.uefi.edk2.path_utilities import Edk2Path

import sys

import yaml
sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'Tools'))
from GenCfgData import CGenCfgData

class UpdatePolicyHdr(IUefiBuildPlugin):

    def trimTreeBaseOnMinver (self, tree, list):

        if type(tree) is not OrderedDict:
            raise Exception ("Incorrect tree type!!!")

        try:
            ver = int(tree["minver"], 0)
        except:
            ver = 0

        trim_list = []
        for idx in range(len(list)):
            if idx < ver and list[idx] != None:
                # trim the entry if this minver is higher than it belongs
                list[idx] = None
                trim_list.append(idx)

        for value in tree:
            if type(tree[value]) is OrderedDict:
                sub_list = []
                for idx in range(len(list)):
                    if list[idx] != None:
                        sub_list.append(list[idx][value])
                    else:
                        sub_list.append(None)
                sub_trim_list = self.trimTreeBaseOnMinver (tree[value], sub_list)
                for item in sub_trim_list:
                    del list[item][value]

        return trim_list

    # in-place prettyprint formatter
    @staticmethod
    def indent(elem, level=0):
      i = "\n" + level*"  "
      if len(elem):
          if not elem.text or not elem.text.strip():
              elem.text = i + "  "
          if not elem.tail or not elem.tail.strip():
              elem.tail = i
          for elem in elem:
              UpdatePolicyHdr.indent(elem, level+1)
          if not elem.tail or not elem.tail.strip():
              elem.tail = i
      else:
          if level and (not elem.tail or not elem.tail.strip()):
              elem.tail = i

    # Attempt to run GenCfgData to generate C header files
    #
    # Consumes build environement variables: "BUILD_OUTPUT_BASE", "UPDATE_SETTINGS",
    # and either of "POLICY_REPORT_FOLDER" or "ACTIVE_PLATFORM"
    def do_pre_build(self, thebuilder):
        need_check = thebuilder.env.GetValue("UPDATE_SETTINGS")
        if need_check is not None and need_check.upper() == "FALSE":
            logging.warn ("Platform indicated as not checking YAML file changes, will not be updated!")
            return 0

        yaml_list = []
        exception_list = []
        ws = thebuilder.ws
        pp = thebuilder.pp.split(os.pathsep)
        edk2 = Edk2Path(ws, pp)

        # Form the exception list of formatted absolute paths. And always ignore our own samples.
        exception_list.append (thebuilder.mws.join (thebuilder.ws, "PolicyServicePkg", "Samples"))
        platform_exception = thebuilder.env.GetValue("POLICY_IGNORE_PATHS")
        if platform_exception is not None:
          plat_list = platform_exception.split(';')
          for each in plat_list:
            exception_list.append(os.path.normpath (thebuilder.mws.join (thebuilder.ws, each)))

        # Look for *_policy_def.yaml files in all package paths.
        for pkg_path in pp:
            for subdir, dirs, files in os.walk(pkg_path):
                for file in files:
                    if file.endswith ("_policy_def.yaml") or file.endswith ("_policy_def.yml"):
                        yaml_path = os.path.normpath(os.path.join (subdir, file))
                        ignore = False
                        for exception in exception_list:
                          if yaml_path.startswith (exception):
                            ignore = True
                            break
                        if ignore:
                          continue
                        yaml_list.append (yaml_path)
                        logging.debug (yaml_path)

        err_count = 0
        type = 'POLICY'
        report_dir = thebuilder.env.GetValue("%s_REPORT_FOLDER" % type)
        if report_dir is None:
            report_dir = edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath (
                          edk2.GetContainingPackage(
                            edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                              thebuilder.env.GetValue("ACTIVE_PLATFORM"))))

        report_file = os.path.join (report_dir, "%s_REPORT.xml" % type)

        if os.path.isfile (report_file):
            tree = ET.parse(report_file).getroot()
        else:
            tree = None

        comment = ET.Comment(' === Auto-Generated. Please do not change anything!!! === ')
        root = ET.Element('Settings')
        root.insert(0, comment)

        for setting in yaml_list:

            if not os.path.normcase(setting).startswith(os.path.normcase(report_dir.rstrip(os.sep)) + os.sep):
                continue

            logging.info ("Processing settings from %s" % setting)

            final_dir = os.path.join (edk2.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                          edk2.GetContainingPackage (setting)), "Include")
            if not os.path.isdir(final_dir):
              os.mkdir (final_dir)

            # Set up a playground first
            op_dir = thebuilder.mws.join(thebuilder.ws, thebuilder.env.GetValue("BUILD_OUTPUT_BASE"), "ConfPolicy")
            if not os.path.isdir(op_dir):
                os.makedirs(op_dir)

            cmd = thebuilder.mws.join(thebuilder.ws, "PolicyServicePkg", "Tools", "GenCfgData.py")

            conf_file = setting
            if conf_file is None:
                logging.warn ("YAML file not specified, system might not work as expected!!!")
                return 0
            if not os.path.isfile(conf_file):
                logging.error ("YAML file specified is not found!!!")
                return 1

            gen_cfg_data = CGenCfgData()

            if gen_cfg_data.load_yaml(conf_file, shallow_load=True) != 0:
                raise Exception(gen_cfg_data.get_last_error())

            merged_cfg_tree = gen_cfg_data.get_cfg_tree()

            minor_tree_list = []
            max_minver = gen_cfg_data.findMaxMinver (merged_cfg_tree)
            # each minor version needs a spot, thus plus 1 here
            for _ in range(max_minver + 1):
                new_tree = deepcopy (merged_cfg_tree)
                minor_tree_list.append (new_tree)
            self.trimTreeBaseOnMinver (merged_cfg_tree, minor_tree_list)

            target = merged_cfg_tree['PolicyHeader']['category']
            major_version = int (merged_cfg_tree['PolicyHeader']['majver']['value'], 0)

            # Insert xml leaf for this conf/policy/etc
            leaf = ET.Element(target)
            leaf.set("MajorVersion", '0x%04X' % major_version)
            leaf.set("MinorVersion", '0x%04X' % max_minver)

            for idx in range(len(minor_tree_list)):
                minhash_item = ET.Element("Hash-v%x.%x" % (major_version, idx))
                hash_obj = hashlib.md5()
                tree_js = json.dumps(minor_tree_list[idx])
                hash_obj.update(tree_js.encode('utf-8'))
                result = hash_obj.hexdigest()
                minhash_item.text = result
                leaf.append (minhash_item)

            cached_root = None
            if tree != None:
                cached_root = tree.find (target)
            if cached_root != None:
                cached_maj_ver = int (cached_root.get("MajorVersion"), 0)

                if cached_maj_ver == None or major_version != cached_maj_ver:
                    # Print error message here and we will fail the build later on
                    logging.error ("Platform major verison does not match YAML files. Please update the %s descriptor file." % type)
                    err_count = err_count + 1

                count = 0

                for idx in range(len(minor_tree_list)):
                    saved_res = cached_root.find("Hash-v%x.%x" % (major_version, idx))
                    calc_ret = leaf.find("Hash-v%x.%x" % (major_version, idx))
                    if saved_res == None or saved_res.text != calc_ret.text:
                        count = count + 1
                        if idx == 0:
                            logging.error ("Minor version 0 has changed, please consider bumping up major version")
                        logging.error ("%d minor version fields have changed, please update your report file" % idx)
                        err_count = err_count + 1

                # Just to check if the cached hash file has extra entries compared to reality
                for res in cached_root:
                    calc_ret = leaf.find(res.tag)
                    if calc_ret == None:
                        logging.error ("A tag from cached xml (%s) is not found" % res.tag)
                        err_count = err_count + 1

                tree.remove (cached_root)
            else:
                logging.error ("%s report file not found, please add the autogen xml file to your %s_REPORT_FOLDER" % (type, type))
                err_count = err_count + 1

            # Now that we have the PKL file, output the header files
            params = ["GENHDR"]
            params.append(conf_file)
            params.append("PolicyDataStruct%s.h" % target)

            ret = RunPythonScript(cmd, " ".join(params), workingdir=final_dir)
            if ret != 0:
                return ret

            root.append (leaf)

        if tree != None and 0 != len(tree):
            logging.error ("There is stale policy from cached xml %s, please remove them or use the newly created report." % (str([i.tag for i in tree])))
            err_count = err_count + len(tree)

        if err_count != 0:
            UpdatePolicyHdr.indent(root)
            hash_obj = hashlib.md5()
            tree_xml = ET.tostring(root, encoding="utf-8", xml_declaration=True)
            hash_obj.update(tree_xml)
            xml_hash = hash_obj.hexdigest()
            new_file = os.path.join (report_dir, "%s_REPORT_%s.xml" % (type, xml_hash))
            xml_file = open(new_file, 'wb')
            xml_file.write(tree_xml)
            xml_file.close()
            logging.info ("New %s report xml was generated at %s, please replace %s with this new file." % (type, report_file, new_file))

        return err_count
