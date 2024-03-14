# CParser4

Files CLexer.py, CListener.py and CParser.py are auto generated through antlr4. The files are tied directly to the
verison of antlr4-python3-runtime that was installed while generating the files. Inside of the file, there is a check
that verifies the version of `antlr4-python3-runtime` installed in the system matches the version used to generate
the files.

## Setup required to generate a new version of CParser4 files

Through the python enviornment, install antlr4-tools.

```ini
    python -m pip install antlr4-tools==0.2.1
```

Verify the python version of antlr4-python3-runtime version is the correct version being targeted.

## Generating new files CParser4 files

In Command prompt, navigate to the CParser4 diretory and run the following command

```ini
    antlr4  -Dlanguage=Python3 C.g4
```

The only generated files that are required are `CLexer.py`, `CListener.py` and `CParser.py`.

## Copyright

Copyright (C) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent
