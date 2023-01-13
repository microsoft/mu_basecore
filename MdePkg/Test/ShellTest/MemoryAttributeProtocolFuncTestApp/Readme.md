# Memory Attribute Protocol UEFI shell functional Test

## &#x1F539; Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

## Attribution

This test is a modified version of <https://github.com/jyao1/edk2/commit/5828d4c755658eba06d838edee9a7b4d72a9f8b3>.

## About This Test

Tests does basic verification of the Memory Attribute Protocol

* Make sure protocol exists
* Basic "good path" usage of Get/Clear/Set functions
* Get Attributes of a newly allocated EfiLoaderCode buffer
* Verify Attributes of running code (this test code)
