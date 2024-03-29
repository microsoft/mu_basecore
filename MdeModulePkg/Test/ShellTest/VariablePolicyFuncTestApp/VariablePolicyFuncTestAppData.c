/** @file -- VariablePolicyFuncTestAppData.h
Payload to be used to create an Authenticated Variable for testing.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

UINT8  mTestAuthVarPayload[] = {
  // EFI_VARIABLE_AUTHENTICATION_2
  //  Timestamp
  0xE4, 0x07, 0x08, 0x15, 0x0D, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //  AuthInfo (WIN_CERTIFICATE_UEFI_GUID)
  //    Hdr (WIN_CERTIFICATE)
  //      dwLength
  0x45, 0x05, 0x00, 0x00,
  //      wRevision
  0x00, 0x02,
  //      wCertificateType
  //      (WIN_CERT_TYPE_EFI_GUID)
  0xF1, 0x0E,
  //    CertType
  //    (gEfiCertPkcs7Guid)
  0x9D, 0xD2, 0xAF, 0x4A, 0xDF, 0x68, 0xEE, 0x49, 0x8A, 0xA9, 0x34, 0x7D, 0x37, 0x56, 0x65, 0xA7,
  //    CertData    (Packed SignedData Signature)
  //      Digest Buffer Was...
  //        Name (DummyAuthVar)
  //        44 00 75 00 6D 00 6D 00 79 00 41 00 75 00 74 00 68 00 56 00 61 00 72 00
  //        Vendor Guid (mTestAuthNamespaceGuid)
  //        C6 A2 C5 B6 CE 3E 9B 4B 8C C8 96 D8 D9 CA D3 4E
  //        Attributes (NV + BS + RT, TimeAuth)
  //        27 00 00 00
  //        Timestamp
  //        E4 07 08 15 0D 1E 00 00 00 00 00 00 00 00 00 00
  //        Data (0xDEADBEEF)
  //        EF BE AD DE
  0x30, 0x82, 0x05, 0x29, 0x02, 0x01, 0x01, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48,
  0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x30, 0x0B, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86,
  0xF7, 0x0D, 0x01, 0x07, 0x01, 0xA0, 0x82, 0x03, 0x82, 0x30, 0x82, 0x03, 0x7E, 0x30, 0x82, 0x02,
  0x66, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x5A, 0xAE, 0x85, 0xA8, 0x61, 0x6E, 0x80, 0xA3,
  0x4D, 0x11, 0x69, 0x06, 0xC3, 0xFE, 0x2D, 0x89, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86,
  0xF7, 0x0D, 0x01, 0x01, 0x0B, 0x05, 0x00, 0x30, 0x3F, 0x31, 0x3D, 0x30, 0x3B, 0x06, 0x03, 0x55,
  0x04, 0x03, 0x1E, 0x34, 0x00, 0x50, 0x00, 0x41, 0x00, 0x4C, 0x00, 0x49, 0x00, 0x4E, 0x00, 0x44,
  0x00, 0x52, 0x00, 0x4F, 0x00, 0x4D, 0x00, 0x45, 0x00, 0x5F, 0x00, 0x53, 0x00, 0x65, 0x00, 0x6C,
  0x00, 0x66, 0x00, 0x68, 0x00, 0x6F, 0x00, 0x73, 0x00, 0x74, 0x00, 0x5F, 0x00, 0x53, 0x00, 0x69,
  0x00, 0x67, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x72, 0x30, 0x20, 0x17, 0x0D, 0x30, 0x30, 0x30, 0x31,
  0x30, 0x31, 0x30, 0x37, 0x30, 0x30, 0x30, 0x30, 0x5A, 0x18, 0x0F, 0x32, 0x39, 0x39, 0x39, 0x31,
  0x32, 0x33, 0x31, 0x30, 0x37, 0x30, 0x30, 0x30, 0x30, 0x5A, 0x30, 0x3F, 0x31, 0x3D, 0x30, 0x3B,
  0x06, 0x03, 0x55, 0x04, 0x03, 0x1E, 0x34, 0x00, 0x50, 0x00, 0x41, 0x00, 0x4C, 0x00, 0x49, 0x00,
  0x4E, 0x00, 0x44, 0x00, 0x52, 0x00, 0x4F, 0x00, 0x4D, 0x00, 0x45, 0x00, 0x5F, 0x00, 0x53, 0x00,
  0x65, 0x00, 0x6C, 0x00, 0x66, 0x00, 0x68, 0x00, 0x6F, 0x00, 0x73, 0x00, 0x74, 0x00, 0x5F, 0x00,
  0x53, 0x00, 0x69, 0x00, 0x67, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x72, 0x30, 0x82, 0x01, 0x22, 0x30,
  0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82,
  0x01, 0x0F, 0x00, 0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01, 0x00, 0xC9, 0xA2, 0x80, 0xE7,
  0x3A, 0x0B, 0x3E, 0xCF, 0xEE, 0x0E, 0x22, 0x65, 0xF5, 0x03, 0xD2, 0x6A, 0x99, 0xBF, 0x5F, 0x48,
  0xF4, 0xC0, 0xD3, 0x19, 0xE7, 0x6B, 0x09, 0xFC, 0x0C, 0xB0, 0x3B, 0x69, 0x3A, 0x07, 0x6F, 0x36,
  0x57, 0xF6, 0x63, 0xAF, 0x6B, 0x7B, 0x30, 0x55, 0xD5, 0xE9, 0xF4, 0xDE, 0x89, 0xE3, 0x5F, 0xA1,
  0x71, 0x13, 0x3E, 0x84, 0x5D, 0x46, 0x9F, 0x78, 0xA9, 0x5B, 0xA5, 0x46, 0x3B, 0x38, 0x4F, 0x00,
  0x06, 0x63, 0x0E, 0x7A, 0x0A, 0x93, 0xE7, 0x36, 0x87, 0xCC, 0x47, 0xBD, 0xFB, 0x0A, 0x5D, 0x45,
  0x9C, 0xC4, 0x1B, 0xE6, 0x9E, 0xCB, 0xAB, 0xF9, 0x20, 0x11, 0xEF, 0x03, 0xCA, 0x9F, 0xE9, 0x29,
  0x1A, 0x05, 0xF8, 0xB3, 0x46, 0xB0, 0x3D, 0xFD, 0x88, 0x7C, 0x82, 0x0E, 0x3C, 0x6F, 0xEA, 0x5B,
  0xFF, 0xA8, 0xA4, 0xE0, 0x40, 0x2B, 0x25, 0xE8, 0x59, 0x46, 0xEE, 0xDB, 0x4B, 0x5F, 0x02, 0xB3,
  0x21, 0x33, 0x47, 0x2E, 0xD5, 0x66, 0x79, 0xF3, 0x79, 0x93, 0x18, 0x75, 0x94, 0x4A, 0x01, 0xCF,
  0x59, 0x86, 0xF4, 0x8B, 0x35, 0xBD, 0xA4, 0x58, 0xA4, 0x76, 0x89, 0x77, 0x55, 0x55, 0xB1, 0xE4,
  0x00, 0x09, 0x78, 0xF3, 0x29, 0x5B, 0xC0, 0xED, 0xD6, 0x68, 0x7E, 0xDB, 0xAA, 0x9F, 0x4E, 0xFE,
  0x67, 0x41, 0x4E, 0x6C, 0xC8, 0xDD, 0x52, 0xD6, 0xA5, 0x8A, 0x8A, 0x56, 0x50, 0x51, 0x27, 0x29,
  0x2B, 0xD3, 0x1B, 0x4D, 0xCE, 0x93, 0x76, 0x8E, 0x55, 0x53, 0x55, 0x30, 0x10, 0xF5, 0xF9, 0x6C,
  0xAE, 0xDA, 0xBA, 0xAC, 0x36, 0x79, 0x11, 0x02, 0xD0, 0x24, 0x07, 0xA6, 0xD1, 0x56, 0xCB, 0xEC,
  0x81, 0x29, 0xA8, 0xC1, 0x2E, 0x9D, 0x9B, 0xF9, 0xE9, 0xF4, 0x55, 0x74, 0xA0, 0x52, 0x87, 0x49,
  0x4F, 0xAC, 0x71, 0xFF, 0x30, 0x12, 0x24, 0xDD, 0x6D, 0x50, 0x5C, 0x7D, 0x02, 0x03, 0x01, 0x00,
  0x01, 0xA3, 0x74, 0x30, 0x72, 0x30, 0x70, 0x06, 0x03, 0x55, 0x1D, 0x01, 0x04, 0x69, 0x30, 0x67,
  0x80, 0x10, 0x0E, 0xB2, 0xFB, 0xDC, 0xD5, 0xAB, 0xCC, 0xB4, 0x3B, 0x46, 0x1B, 0x60, 0x18, 0xFD,
  0xDE, 0x74, 0xA1, 0x41, 0x30, 0x3F, 0x31, 0x3D, 0x30, 0x3B, 0x06, 0x03, 0x55, 0x04, 0x03, 0x1E,
  0x34, 0x00, 0x50, 0x00, 0x41, 0x00, 0x4C, 0x00, 0x49, 0x00, 0x4E, 0x00, 0x44, 0x00, 0x52, 0x00,
  0x4F, 0x00, 0x4D, 0x00, 0x45, 0x00, 0x5F, 0x00, 0x53, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x66, 0x00,
  0x68, 0x00, 0x6F, 0x00, 0x73, 0x00, 0x74, 0x00, 0x5F, 0x00, 0x53, 0x00, 0x69, 0x00, 0x67, 0x00,
  0x6E, 0x00, 0x65, 0x00, 0x72, 0x82, 0x10, 0x5A, 0xAE, 0x85, 0xA8, 0x61, 0x6E, 0x80, 0xA3, 0x4D,
  0x11, 0x69, 0x06, 0xC3, 0xFE, 0x2D, 0x89, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7,
  0x0D, 0x01, 0x01, 0x0B, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0xB5, 0xA2, 0xD0, 0x1B, 0x70,
  0x24, 0xC2, 0xE8, 0x64, 0xCD, 0xF1, 0xE9, 0x97, 0x9E, 0xA7, 0xC1, 0x86, 0x92, 0x06, 0x2F, 0x8F,
  0x33, 0x64, 0x0A, 0xB9, 0x2B, 0x77, 0xE2, 0x70, 0x82, 0xDE, 0x06, 0xD3, 0x69, 0x8E, 0xB4, 0x69,
  0xF1, 0x6B, 0x59, 0x5E, 0x68, 0x5F, 0xB4, 0xFA, 0x30, 0xC3, 0xB6, 0xA1, 0x72, 0x1A, 0xD4, 0x01,
  0xED, 0x69, 0x4A, 0x96, 0x0F, 0x1C, 0xC3, 0x6F, 0x80, 0x0B, 0xE5, 0xD4, 0x46, 0xBE, 0x27, 0x9D,
  0xDE, 0x68, 0xB3, 0xA1, 0x93, 0xC3, 0x1A, 0x47, 0x20, 0x7A, 0x87, 0x80, 0x13, 0x85, 0x1E, 0x46,
  0x01, 0x42, 0x6A, 0x68, 0x46, 0xE2, 0x77, 0x3D, 0x2E, 0x50, 0xA1, 0x96, 0x23, 0x83, 0x03, 0xD1,
  0x57, 0xDD, 0xC6, 0x63, 0x59, 0xB7, 0x1A, 0x49, 0xA2, 0xC9, 0x44, 0x8D, 0xC7, 0x81, 0x18, 0xE8,
  0x52, 0x3A, 0x74, 0x32, 0xD3, 0xE6, 0x6D, 0x54, 0x9F, 0xC9, 0x87, 0x1C, 0xBC, 0x81, 0xEB, 0x6D,
  0x5D, 0x58, 0xF7, 0x91, 0x81, 0x5B, 0xB0, 0x86, 0xB4, 0x06, 0xE7, 0x19, 0x44, 0xE9, 0x24, 0x28,
  0xF5, 0x42, 0x7A, 0x7A, 0x28, 0x94, 0x3E, 0x70, 0x61, 0x1B, 0x68, 0x8D, 0xA9, 0x48, 0x3A, 0xFE,
  0x7D, 0xB5, 0x29, 0x10, 0xCE, 0xD6, 0xC1, 0xFF, 0x16, 0xDF, 0x90, 0x94, 0x16, 0xC8, 0xFA, 0x9E,
  0x52, 0x49, 0xE5, 0xC3, 0xF5, 0x8C, 0x87, 0xC2, 0x93, 0x3D, 0x3D, 0x27, 0x23, 0x37, 0xC3, 0xDA,
  0x55, 0x92, 0x12, 0xE9, 0x1F, 0xEB, 0x32, 0xB5, 0xD8, 0x30, 0xD6, 0xC0, 0x23, 0x45, 0xBB, 0x06,
  0xBC, 0x11, 0xA6, 0xA3, 0x47, 0x82, 0x04, 0xCB, 0xAA, 0x98, 0xCA, 0xF9, 0x00, 0x0E, 0xD3, 0xC3,
  0x09, 0xF6, 0x21, 0x4C, 0x90, 0xE0, 0x78, 0x08, 0xAE, 0x8F, 0xB1, 0x7D, 0x62, 0x3F, 0x6A, 0x1E,
  0xD6, 0xF1, 0x8E, 0xEE, 0xFD, 0x49, 0x04, 0xDE, 0x14, 0x9C, 0x7B, 0x31, 0x82, 0x01, 0x7E, 0x30,
  0x82, 0x01, 0x7A, 0x02, 0x01, 0x01, 0x30, 0x53, 0x30, 0x3F, 0x31, 0x3D, 0x30, 0x3B, 0x06, 0x03,
  0x55, 0x04, 0x03, 0x1E, 0x34, 0x00, 0x50, 0x00, 0x41, 0x00, 0x4C, 0x00, 0x49, 0x00, 0x4E, 0x00,
  0x44, 0x00, 0x52, 0x00, 0x4F, 0x00, 0x4D, 0x00, 0x45, 0x00, 0x5F, 0x00, 0x53, 0x00, 0x65, 0x00,
  0x6C, 0x00, 0x66, 0x00, 0x68, 0x00, 0x6F, 0x00, 0x73, 0x00, 0x74, 0x00, 0x5F, 0x00, 0x53, 0x00,
  0x69, 0x00, 0x67, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x72, 0x02, 0x10, 0x5A, 0xAE, 0x85, 0xA8, 0x61,
  0x6E, 0x80, 0xA3, 0x4D, 0x11, 0x69, 0x06, 0xC3, 0xFE, 0x2D, 0x89, 0x30, 0x0D, 0x06, 0x09, 0x60,
  0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86,
  0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82, 0x01, 0x00, 0xA6, 0x06, 0xE7,
  0x46, 0x7E, 0xFB, 0x4A, 0xA7, 0x25, 0x2F, 0x52, 0x1D, 0xBC, 0x5C, 0x41, 0x3B, 0xD3, 0x13, 0x50,
  0xCE, 0x5F, 0xE2, 0x4B, 0x31, 0xED, 0x28, 0x5E, 0xF5, 0x36, 0xBD, 0x1C, 0x38, 0xA1, 0xB6, 0x45,
  0x7C, 0xFD, 0xAB, 0x7B, 0x0C, 0xBF, 0x06, 0x06, 0xBB, 0x95, 0x5E, 0x47, 0x10, 0x7C, 0xD8, 0x10,
  0x76, 0x74, 0x81, 0x2D, 0x40, 0x3A, 0xD0, 0xF4, 0x15, 0x9D, 0xDF, 0x44, 0x2B, 0xA4, 0xCD, 0xF7,
  0x44, 0x77, 0x9F, 0x35, 0x46, 0xD3, 0x30, 0x67, 0x44, 0x33, 0xF4, 0x7B, 0xB6, 0xC0, 0xE4, 0xA2,
  0xAD, 0xDF, 0xAF, 0x56, 0x41, 0xA3, 0x0D, 0x76, 0x36, 0xB9, 0x7E, 0x29, 0x49, 0x17, 0x43, 0xAF,
  0xB0, 0xA0, 0xC0, 0xF1, 0xE1, 0xE6, 0xCA, 0x62, 0x9F, 0x3E, 0x9D, 0x6C, 0x63, 0x03, 0xF6, 0xDF,
  0x84, 0x32, 0xB1, 0x01, 0x0C, 0x12, 0x83, 0x52, 0x13, 0x2F, 0xAE, 0xBC, 0x79, 0xB7, 0x75, 0xF6,
  0x10, 0x20, 0xFC, 0x7A, 0x13, 0x92, 0xF7, 0x87, 0x50, 0xF5, 0x9C, 0xD9, 0xE4, 0xEA, 0x4C, 0x3D,
  0x31, 0xED, 0x7F, 0xA6, 0x6C, 0x58, 0xAD, 0x6C, 0x31, 0xAF, 0xC4, 0x64, 0xAE, 0x11, 0xBF, 0x72,
  0xF5, 0xAA, 0x69, 0xB4, 0x76, 0xDB, 0x73, 0x8F, 0x8C, 0x3E, 0x23, 0x4A, 0x2D, 0xB7, 0x65, 0x65,
  0x10, 0xA8, 0xC6, 0x52, 0x14, 0xE2, 0xC6, 0x2B, 0x07, 0xCE, 0x45, 0x58, 0x6F, 0x92, 0x78, 0xAA,
  0xB5, 0xE9, 0x76, 0x39, 0x8A, 0x17, 0xE3, 0x0B, 0xA5, 0x12, 0x0F, 0x2A, 0xC1, 0xCE, 0xC5, 0x4F,
  0xD8, 0xA7, 0xD1, 0x7C, 0x3F, 0xE3, 0x23, 0x9B, 0x53, 0x56, 0x18, 0x28, 0x66, 0xC7, 0xB3, 0x04,
  0x38, 0xE3, 0x40, 0xCC, 0xB2, 0x18, 0xA8, 0xC7, 0x11, 0xE1, 0x67, 0xD8, 0xBF, 0xBE, 0x8D, 0x2A,
  0x75, 0x00, 0x96, 0x8F, 0x7F, 0x80, 0xCF, 0xDB, 0xF0, 0x0D, 0xB5, 0x8D, 0x73,
  // Data
  0xEF, 0xBE, 0xAD, 0xDE
};
UINTN  mTestAuthVarPayloadSize = sizeof (mTestAuthVarPayload);
