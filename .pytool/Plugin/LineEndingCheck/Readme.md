# Line Ending Check Plugin

This CiBuildPlugin scans all the files in a package to verify that the line endings are CRLF.

> _Note:_ If you encounter a line ending issue found by this plugin, update your development environment to avoid
> issues again in the future.
>
> Most problems are caused by `autocrlf=true` in git settings, which will automatically adjust line endings upon
> checkout and commit which distorts the actual line endings from being consistent locally and remotely. In
> other cases, developing within a Linux workspace will natively use LF by default.
>
> It is simplest to set `autocrlf=false` to prevent manipulation of line endings outside of the actual values and set
> up your editor to use CRLF line endings within the project.

## Configuration

The plugin can be configured to ignore certain files.

``` yaml
"LineEndingCheck": {
    "IgnoreFiles": []
    "IgnoreFilesWithNoExtension": False
}
```

### IgnoreFiles

An **optional** list of git ignore patterns relative to the package root used to exclude files from being checked.

### IgnoreFilesWithNoExtension

An **optional** value that, if True, will insert the gitignore rules necessary to have this check ignore files
that do not contain a file extension. Necessary for binary files and/or POSIX like executables.
