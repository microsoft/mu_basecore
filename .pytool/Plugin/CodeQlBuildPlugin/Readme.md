# CodeQL Build Plugin

This build plugin generates a CodeQL database of the package being built.

## Usage

The plugin is activated by including the `codeql` scope in the list of active scopes.

Once active, the plugin will set up CodeQL tracing of the `build` command such that build output is captured into a
CodeQL database that is in a directory unique to the package and target being built:

  `Build/codeql-db-<package>-<target>`
