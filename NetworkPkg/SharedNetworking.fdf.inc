## @file
# SharedNetwork FDF include file for All Architectures.
#
# This file can be included to a platform FDF in FV.FVDXE by using
# "!include NetworkPkg/SharedNetworking.fdf.inc" to add SharedNetworking
#
#    Copyright (c) Microsoft Corporation
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Currently only works on X64
FILE FV_IMAGE = C96105D2-B7D3-4E21-B377-846027D595A7 {
  SECTION FV_IMAGE = NetworkPkg/SharedNetworking/Mu-SharedNetworking_extdep/$(TARGET)/X64/FVDXE.Fv # Shared_Networking FV
  SECTION UI = "SharedNetworking"
}
