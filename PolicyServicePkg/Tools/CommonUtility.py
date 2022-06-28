#!/usr/bin/env python
## @ CommonUtility.py
# Common utility script
#
# Copyright (c) 2016 - 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

##
# Import Modules
#
import os
import sys
import re
import shutil
import subprocess
import struct
import hashlib
import string
from   ctypes import *
from   functools import reduce
from   importlib.machinery import SourceFileLoader

def print_bytes (data, indent=0, offset=0, show_ascii = False):
    bytes_per_line = 16
    printable = ' ' + string.ascii_letters + string.digits + string.punctuation
    str_fmt = '{:s}{:04x}: {:%ds} {:s}' % (bytes_per_line * 3)
    bytes_per_line
    data_array = bytearray(data)
    for idx in range(0, len(data_array), bytes_per_line):
        hex_str = ' '.join('%02X' % val for val in data_array[idx:idx + bytes_per_line])
        asc_str = ''.join('%c' % (val if (chr(val) in printable) else '.')
                          for val in data_array[idx:idx + bytes_per_line])
        print (str_fmt.format(indent * ' ', offset + idx, hex_str, ' ' + asc_str if show_ascii else ''))

def get_bits_from_bytes (bytes, start, length):
    if length == 0:
        return 0
    byte_start = (start)  // 8
    byte_end   = (start + length - 1) // 8
    bit_start  = start & 7
    mask = (1 << length) - 1
    val = bytes_to_value (bytes[byte_start:byte_end + 1])
    val = (val >> bit_start) & mask
    return val

def set_bits_to_bytes (bytes, start, length, bvalue):
    if length == 0:
        return
    byte_start = (start)  // 8
    byte_end   = (start + length - 1) // 8
    bit_start  = start & 7
    mask = (1 << length) - 1
    val  = bytes_to_value (bytes[byte_start:byte_end + 1])
    val &= ~(mask << bit_start)
    val |= ((bvalue & mask) << bit_start)
    bytes[byte_start:byte_end+1] = value_to_bytearray (val, byte_end + 1 - byte_start)

def value_to_bytes (value, length):
    return value.to_bytes(length, 'little')

def bytes_to_value (bytes):
    return int.from_bytes (bytes, 'little')

def value_to_bytearray (value, length):
    return bytearray(value_to_bytes(value, length))

def value_to_bytearray (value, length):
    return bytearray(value_to_bytes(value, length))

def get_aligned_value (value, alignment = 4):
    if alignment != (1 << (alignment.bit_length() - 1)):
        raise Exception ('Alignment (0x%x) should to be power of 2 !' % alignment)
    value = (value + (alignment - 1)) & ~(alignment - 1)
    return value

def get_padding_length (data_len, alignment = 4):
    new_data_len = get_aligned_value (data_len, alignment)
    return new_data_len - data_len

def get_file_data (file, mode = 'rb'):
    return open(file, mode).read()

def gen_file_from_object (file, object):
    open (file, 'wb').write(object)

def gen_file_with_size (file, size):
    open (file, 'wb').write(b'\xFF' * size);

def check_files_exist (base_name_list, dir = '', ext = ''):
    for each in base_name_list:
        if not os.path.exists (os.path.join (dir, each + ext)):
            return False
    return True

def load_source (name, filepath):
    mod = SourceFileLoader (name, filepath).load_module()
    return  mod

def get_openssl_path ():
    if os.name == 'nt':
        if 'OPENSSL_PATH' not in os.environ:
            openssl_dir = "C:\\Openssl\\bin\\"
            if os.path.exists (openssl_dir):
                os.environ['OPENSSL_PATH'] = openssl_dir
            else:
                os.environ['OPENSSL_PATH'] = "C:\\Openssl\\"
                if 'OPENSSL_CONF' not in os.environ:
                    openssl_cfg = "C:\\Openssl\\openssl.cfg"
                    if os.path.exists(openssl_cfg):
                        os.environ['OPENSSL_CONF'] = openssl_cfg
        openssl = os.path.join(os.environ.get ('OPENSSL_PATH', ''), 'openssl.exe')
    else:
        # Get openssl path for Linux cases
        openssl = shutil.which('openssl')

    return openssl

def run_process (arg_list, print_cmd = False, capture_out = False):
    sys.stdout.flush()
    if os.name == 'nt' and os.path.splitext(arg_list[0])[1] == '' and \
       os.path.exists (arg_list[0] + '.exe'):
        arg_list[0] += '.exe'
    if print_cmd:
        print (' '.join(arg_list))

    exc    = None
    result = 0
    output = ''
    try:
        if capture_out:
            output = subprocess.check_output(arg_list).decode()
        else:
            result = subprocess.call (arg_list)
    except Exception as ex:
        result = 1
        exc    = ex

    if result:
        if not print_cmd:
            print ('Error in running process:\n  %s' % ' '.join(arg_list))
        if exc is None:
            sys.exit(1)
        else:
            raise exc

    return output
