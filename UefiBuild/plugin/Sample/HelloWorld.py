import yapsy.UefiBuildPluginTypes as UefiBuildPluginTypes
import logging

class HelloWorld(UefiBuildPluginTypes.IUefiBuildPlugin):

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