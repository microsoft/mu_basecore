## @file
# GNU/Linux makefile for 'GenSec' module build.
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
MAKEROOT ?= ..

APPNAME = GenSec

OBJECTS = GenSec.o

include $(MAKEROOT)/Makefiles/app.makefile

LIBS = -lCommon
ifeq ($(CYGWIN), CYGWIN)
  LIBS += -L/lib/e2fsprogs -luuid
endif

ifeq ($(LINUX), Linux)
# MU_CHANGE STARTs: Support cross compiling
ifndef CROSS_LIB_UUID
  LIBS += -luuid
else
  LIBS += -L$(CROSS_LIB_UUID)
endif
# MU_CHANGE ENDs
endif

