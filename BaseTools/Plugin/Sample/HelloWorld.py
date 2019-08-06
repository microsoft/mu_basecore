## @file HelloWorld.py
# Sample Project Mu pre/post build plugin
##
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
###
from MuEnvironment import PluginManager
import logging

class HelloWorld(PluginManager.IUefiBuildPlugin):

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
