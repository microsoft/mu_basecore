# Nested Package Check Plugin

This CiBuildPlugin detects nested packages and fails if the nested package is due to the currently tested package

## Plugin Configuration

``` yaml
    "NestedPackageCheck": {
        "AuditOnly": False,
    }

```

### AuditOnly

Boolean - Default is False.
If True, run the test in an Audit only mode which will log all errors but instead of failing the
build, it will set the test as skipped.  This allows visibility into failures without breaking the
build.
