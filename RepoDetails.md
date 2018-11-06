# Project Mu Basecore Repository

??? info "Git Details"
    Repository Url: {{mu_basecore.url}}  
    Branch:         {{mu_basecore.branch}}  
    Commit:         [{{mu_basecore.commit}}]({{mu_basecore.commitlink}})  
    Commit Date:    {{mu_basecore.date}}

This repository is considered foundational and fundamental to Project Mu. The guiding philosophy is that this any code within this repository should be one or more of the following

* Part of the build system
* Common to any silicon architecture
* Part of the "API layer" that contains protocol and library definitions including
  * Industry Standards
  * UEFI Specifications
  * ACPI Specifications
* Part of the "PI" layer that contains driver dispatch logic, event/signaling logic, or memory management logic
  * This can also include central technologies like variable services

## More Info

Please see the Project Mu docs (https://github.com/Microsoft/mu) for more information.  

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).

For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Issues

Please open any issues in the Project Mu GitHub tracker. [More Details](https://microsoft.github.io/mu/How/contributing/)

## Contributing Code or Docs

Please follow the general Project Mu Pull Request process.  [More Details](https://microsoft.github.io/mu/How/contributing/)

* [Code Requirements](/DeveloperDocs/code_requirements)
* [Doc Requirements](/DeveloperDocs/doc_requirements)

## Builds

run MuBuild.py -c corebuild.mu.json

## Copyright & License

Copyright (c) 2016-2018, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### Upstream License (TianoCore)

Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.
Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.
Copyright (c) 2011 - 2015, ARM Limited. All rights reserved.
Copyright (c) 2014 - 2015, Linaro Limited. All rights reserved.
Copyright (c) 2013 - 2015, Red Hat, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
