## @file Tpm2Simulator.py
# This file contains transportation layer classes for interacting with the TPM 2.0 simulator.
#
##
# Copyright (c) 2017, Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

import socket
import struct
import Tpm2Stream as t2s
import Tpm2Defs as t2d

PLAT_COMMANDS = {
  'TPM_SIGNAL_POWER_ON':            1,
  'TPM_SIGNAL_POWER_OFF':           2,
  'TPM_SIGNAL_PHYS_PRES_ON':        3,
  'TPM_SIGNAL_PHYS_PRES_OFF':       4,
  'TPM_SIGNAL_HASH_START':          5,
  'TPM_SIGNAL_HASH_DATA':           6,
    ## {UINT32 BufferSize, BYTE[BufferSize] Buffer}
  'TPM_SIGNAL_HASH_END':            7,
  'TPM_SEND_COMMAND':               8,
    ## {BYTE Locality, UINT32 InBufferSize, BYTE[InBufferSize] InBuffer} ->
    ## {UINT32 OutBufferSize, BYTE[OutBufferSize] OutBuffer}
  'TPM_SIGNAL_CANCEL_ON':           9,
  'TPM_SIGNAL_CANCEL_OFF':          10,
  'TPM_SIGNAL_NV_ON':               11,
  'TPM_SIGNAL_NV_OFF':              12,
  'TPM_SIGNAL_KEY_CACHE_ON':        13,
  'TPM_SIGNAL_KEY_CACHE_OFF':       14,
  'TPM_REMOTE_HANDSHAKE':           15,
  'TPM_SET_ALTERNATIVE_RESULT':     16,
  'TPM_SIGNAL_RESET':               17,
  'TPM_SESSION_END':                20,
  'TPM_STOP':                       21,
  'TPM_GET_COMMAND_RESPONSE_SIZES': 25,
  'TPM_TEST_FAILURE_MODE':          30,
}

class TpmSimulator(object):

  def __init__(self, host='localhost', port=2321):
    super(TpmSimulator, self).__init__()

    # Connect to the control socket.
    self.platSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.platSock.connect((host, port+1))

    # Connect to the simulator socket.
    self.tpmSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.tpmSock.connect((host, port))

    # Power cycle the TPM.
    self.platSock.send(struct.pack(">L", PLAT_COMMANDS['TPM_SIGNAL_POWER_OFF']))
    self.platSock.send(struct.pack(">L", PLAT_COMMANDS['TPM_SIGNAL_POWER_ON']))

    # Enable the NV space.
    self.platSock.send(struct.pack(">L", PLAT_COMMANDS['TPM_SIGNAL_NV_ON']))

  def send_raw_data(self, data):
    print("RAW -->: " + str(data).encode('hex'))
    self.tpmSock.send(data)

  def read_raw_data(self, count):
    data = self.tpmSock.recv(count)
    print("RAW <--: " + str(data).encode('hex'))
    return data

  def send_data(self, data):
    # Send the "I'm about to send data" command.
    self.send_raw_data(struct.pack(">L", PLAT_COMMANDS['TPM_SEND_COMMAND']))
    # Send the locality for the data.
    self.send_raw_data(struct.pack(">b", 0x03))
    # Send the size of the data.
    self.send_raw_data(struct.pack(">L", len(data)))

    # Now, send the data itself.
    self.send_raw_data(data)

    # Poll until a result is available.
    # NOTE: This shouldn't be necessary and denotes a lack of understanding...
    while True:
      result_size = self.read_raw_data(4)
      result_size = struct.unpack(">L", result_size)[0]
      if (result_size > 0):
        break

    return self.read_raw_data(result_size)

  def startup(self, type):
    stream = t2s.Tpm2CommandStream(t2d.TPM_ST_NO_SESSIONS, 0x00, t2d.TPM_CC_Startup)
    stream.add_element(t2s.Tpm2StreamPrimitive(t2d.TPM_SU_Size, type))
    return self.send_data(stream.get_stream())
