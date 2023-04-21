# Project Mu Basecore Repository

??? info "Git Details"
    Repository Url: {{mu_basecore.url}}
    Branch:         {{mu_basecore.branch}}
    Commit:         [{{mu_basecore.commit}}]({{mu_basecore.commitlink}})
    Commit Date:    {{mu_basecore.date}}

This repository is considered foundational and fundamental to Project Mu. The
guiding philosophy is that this any code within this repository should be one or
more of the following

* Part of the build system
* Common to any silicon architecture
* Part of the "API layer" that contains protocol and library definitions
  including
  * Industry Standards
  * UEFI Specifications
  * ACPI Specifications
* Part of the "PI" layer that contains driver dispatch logic, event/signaling
  logic, or memory management logic
  * This can also include central technologies like variable services

## More Info

Please see the Project Mu docs (<https://github.com/Microsoft/mu>) for more
information.

This project has adopted the [Microsoft Open Source Code of
Conduct](https://opensource.microsoft.com/codeofconduct/).

For more information see the [Code of Conduct
FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional
questions or comments.

## Releases

It is recommended to consume binaries built from the code in this repo (outside of development purposes) through a
versioned release.

Releases are tagged in the repository and are visible in the [Releases](https://github.com/microsoft/mu_basecore/releases)
page. Each release contains release notes describing the changes since the last release with important changes such as
breaking changes highlighted.

A semantic versioning process (version is `<major.minor.patch>`) is followed with the following rules:

- Major Version

  - A major version change indicates a breaking change. This means that the release is not backward
    compatible with the previous release. This is typically a change to the API or ABI of a component.

  - The major version in Mu Basecore is broken down into the following components:
    - `<Mu Release Version><Major Version>`
      - `Mu Release Version` is 6 digits and corresponds to the edk2 stable tag the Mu release branch is based on.
      - `Major Version` is 4 digits and tracks the monotonically incrementing major version within the branch.
      - For example, `2022080001` would be the first major version value in the `release/202208` branch.

- Minor Version

  - A minor version change indicates a new feature or enhancement. This means that the release is backward
    compatible with the previous release but includes new functionality or a major rework of existing functionality.

- Patch Version

  - A patch version change indicates a bug fix or any other change. This means that the release is backward compatible
    with the previous release and contains no new functionality.

## Issues

Please open any issues in the Project Mu GitHub tracker. [More
Details](https://microsoft.github.io/mu/How/contributing/)

## Contributing Code or Docs

Please follow the general Project Mu Pull Request process.  [More
Details](https://microsoft.github.io/mu/How/contributing/)

* [Code Requirements](https://microsoft.github.io/mu/CodeDevelopment/requirements/)
* [Doc Requirements](https://microsoft.github.io/mu/DeveloperDocs/requirements/)

## Builds

Please follow the steps in the Project Mu docs to build for CI and local
testing. [More Details](https://microsoft.github.io/mu/CodeDevelopment/compile/)

## Copyright & License

Copyright (C) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

### Upstream License (TianoCore)

Copyright (c) 2019, TianoCore and contributors.  All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

Subject to the terms and conditions of this license, each copyright holder and
contributor hereby grants to those receiving rights under this license a
perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable
(except for failure to satisfy the conditions of this license) patent license to
make, have made, use, offer to sell, sell, import, and otherwise transfer this
software, where such license applies only to those patent claims, already
acquired or hereafter acquired, licensable by such copyright holder or
contributor that are necessarily infringed by:

(a) their Contribution(s) (the licensed copyrights of copyright holders and
    non-copyrightable additions of contributors, in source or binary form)
    alone; or

(b) combination of their Contribution(s) with the work of authorship to which
    such Contribution(s) was added by such copyright holder or contributor, if,
    at the time the Contribution is added, such addition causes such combination
    to be necessarily infringed. The patent license shall not apply to any other
    combinations which include the Contribution.

Except as expressly stated above, no rights or licenses from any copyright
holder or contributor is granted under this license, whether expressly, by
implication, estoppel or otherwise.

DISCLAIMER

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
