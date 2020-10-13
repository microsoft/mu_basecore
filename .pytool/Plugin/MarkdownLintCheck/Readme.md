# Markdown Lint Plugin

This CiBuildPlugin scans all the md files in a given package and
checks for linter errors.

This plugin requires NodeJs and markdownlint-cli.  If the plugin doesn't find
its required tools then it will mark the test as skipped.

* NodeJS: <https://nodejs.org/en/>
* markdownlint-cli: <https://www.npmjs.com/package/markdownlint-cli>
  * Src available:  <https://github.com/igorshubovych/markdownlint-cli>
  * Rules are described here: <https://github.com/DavidAnson/markdownlint/blob/main/doc/Rules.md>

## Configuration

The plugin has a few configuration options to support the UEFI codebase.

``` yaml
  "MarkdownLintCheck": {
    "AuditOnly": False,          # If True, log all errors and then mark as skipped
    "IgnoreFiles": []            # package root relative file, folder, or glob pattern to ignore
  }
```

### IgnoreFiles

This supports files, folders, and glob patterns

## Rule Overrides

There are times when a certain rule should not apply to part of a markdown file.
Markdownlint has numerous ways to configure this.
See the Configuration options here <https://github.com/DavidAnson/markdownlint#configuration>
