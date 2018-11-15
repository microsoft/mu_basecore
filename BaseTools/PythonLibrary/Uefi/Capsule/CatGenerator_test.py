import os
import logging
import unittest
from Uefi.Capsule.CatGenerator import *

#must run from build env or set PYTHONPATH env variable to point to the PythonLibrary folder

class CatGeneratorTest(unittest.TestCase):

  def test_win10_OS(self):
    o = CatGenerator("x64", "win10")
    self.assertEqual(o.OperatingSystem, "10")

  def test_10_OS(self):
    o = CatGenerator("x64", "10")
    self.assertEqual(o.OperatingSystem, "10")

  def test_win10Server_OS(self):
    o = CatGenerator("x64", "Server10")
    self.assertEqual(o.OperatingSystem, "Server10")

  def test_invalid_OS(self):
    with self.assertRaises(ValueError):
      CatGenerator("x64", "Invalid Junk")

  def test_x64_arch(self):
    o = CatGenerator("x64", "win10")
    self.assertEqual(o.Arch, "X64")

  def test_amd64_arch(self):
    o = CatGenerator("amd64", "win10")
    self.assertEqual(o.Arch, "X64")

  def test_arm_arch(self):
    o = CatGenerator("arm", "win10")
    self.assertEqual(o.Arch, "ARM")

  def test_arm64_arch(self):
    o = CatGenerator("arm64", "win10")
    self.assertEqual(o.Arch, "ARM64")

  def test_aarch64_arch(self):
    o = CatGenerator("aarch64", "win10")
    self.assertEqual(o.Arch, "ARM64")
  
  def test_invalid_arch(self):
    with self.assertRaises(ValueError):
      CatGenerator("Invalid Arch", "win10") 

  def test_invalid_pathtotool(self):
    o = CatGenerator("amd64", "10")
    with self.assertRaises(Exception) as cm:
      o.MakeCat("garbage", os.path.join("c:", "test", "badpath", "inf2cat.exe"))
    self.assertTrue(str(cm.exception).startswith("Can't find Inf2Cat on this machine."))

  


