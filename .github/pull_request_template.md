# Preface

Please ensure you have read the [contribution docs](https://github.com/microsoft/mu/blob/master/CONTRIBUTING.md) prior
to submitting the pull request. In particular,
[pull request guidelines](https://github.com/microsoft/mu/blob/master/CONTRIBUTING.md#pull-request-best-practices).

## Description

<_Please include a description of the change and why this change was made._>

For each item, place an "x" in between `[` and `]` if true. Example: `[x]`.
_(you can also check items in the GitHub UI)_

- [ ] Impacts functionality?
  - **Functionality** - Does the change ultimately impact how firmware functions?
  - Examples: Add a new library, publish a new PPI, update an algorithm, ...
- [ ] Impacts security?
  - **Security** - Does the change have a direct security impact on an application,
    flow, or firmware?
  - Examples: Crypto algorithm change, buffer overflow fix, parameter
    validation improvement, ...
- [ ] Breaking change?
  - **Breaking change** - Will anyone consuming this change experience a break
    in build or boot behavior?
  - Examples: Add a new library class, move a module to a different repo, call
    a function in a new library class in a pre-existing module, ...
- [ ] Includes tests?
  - **Tests** - Does the change include any explicit test code?
  - Examples: Unit tests, integration tests, robot tests, ...
- [ ] Includes documentation?
  - **Documentation** - Does the change contain explicit documentation additions
    outside direct code modifications (and comments)?
  - Examples: Update readme file, add feature readme file, link to documentation
    on an a separate Web page, ...

## How This Was Tested

<_Please describe the test(s) that were run to verify the changes._>

## Integration Instructions

<_Describe how these changes should be integrated. Use N/A if nothing is required._>
