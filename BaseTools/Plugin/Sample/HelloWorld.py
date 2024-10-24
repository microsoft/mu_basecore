## @file HelloWorld.py
# Sample Project Mu pre/post build plugin
##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
### 
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
import logging

class HelloWorld(IUefiBuildPlugin):

    def do_post_build(self, thebuilder):
        t = "PLUGIN HelloWorld: Hello World! - Post Build Plugin Hook"
        print(t)
        logging.debug(t)
        return 0

    def do_pre_build(self, thebuilder):
        t ="PLUGIN HelloWorld: Hello World! - Pre Build Plugin Hook"
        print(t)
        logging.debug(t)
        return 0
