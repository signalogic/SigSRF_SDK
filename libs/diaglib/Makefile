# $Header: /root/Signalogic_YYYYvNN/DirectCore/lib/diaglib/Makefile
#
# Copyright (C) Signalogic Inc. 2017-2024
#
# Description:  Makefile for diagnostics library
#
# Purpose:      API interface for packet and memory diagnostics
#
# Target:       generic x86 or MPC7447A/7448, PowerPC P2020
#
# Linux:        CentOS, Redhat, Gentoo, WindRiver Linux 4.3 (based on Fedora)
#
# Kernel Ver:   2.4, 2.6.9, 2.6.24, 2.6.25, 2.6.34.9, 3.2
#
# Revision History
#
#  Created March 2017, JHB
#  Updated Mar 2018 JHB, define _X86 instead of _X86_ (to be consistent with shared_include / coCPU usage)
#  Modified Nov 2018 JHB, removed SIG_LIBS and I. from include path, set optimization to -O3
#  Modified Oct 2019 JHB, add -flto flag
#  Modified Jan 2020 JHB, add -pthread flag
#  Modified May 2020 JHB, add -Wextra compiler flag
#  Modified Jul 2020 JHB, add -Wl,-soname,libdiaglib.so.major.minor.internal to link target. MAJOR is incremented for API changes (existing APIs or global vars are removed or existing API params are changed). MINOR is incremented for new APIs or global vars. INTERNAL is incremented for bug fixes or other internal modifications with no effect on the ABI
#  Modified Mar 2021 JHB, add INSTALLPATH to allow builds on relative folder structure, remove unnecessary defines
#  Modified Feb 2024 JHB, if standalone=1 given on command line define STANDALONE, NO_HWLIB, and NO_PKTLIB, and build dialglib_standalone.so
#  Modified Feb 2024 JHB, apply soname only to default libdiaglib.so to avoid ldconfig conflicts, rename CC_FLAGS to CFLAGS and LIB_FLAGS to LDFLAGS (Linux convention)

# set install path var, from lib/diaglib folder SigSRF software install path is 3 levels up
INSTALLPATH=../../..

WRLPATH=/opt/WindRiver/wrlinux-4

ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
  # PowerPC build
  export PATH := /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/x86-linux2/:/opt/WindRiver/wrlinux-4/layers/wrll-toolchain-4.4a-341/powerpc/toolchain/x86-linux2/bin:$(PATH)
  CC = /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/x86-linux2/powerpc-wrs-linux-gnu-ppc_e500v2-glibc_small-gcc
  DEFINES = -D_WRL_
else
  # x86 build
  CC = /usr/bin/gcc
  DEFINES = -D_X86
endif

# include paths
INCLUDE = -I$(INSTALLPATH) -I$(INSTALLPATH)/DirectCore/include -I$(INSTALLPATH)/shared_include -I$(INSTALLPATH)/DirectCore/lib

cpp_objects =
c_objects = diaglib.o lib_logging.o

#comment/uncomment the following line to turn debug on/off
#DEBUG=y

ifeq ($(DEBUG),y)
	DBGFLAGS= -O2 -g3 -DDEBUGON
else
	DBGFLAGS= -O3 -g
endif

#defines
DEFINES += -D_LINUX_ -D_SIGRT

ifeq ($(standalone),1)
  DEFINES += -DSTANDALONE -DNO_HWLIB -DNO_PKTLIB
  DIAGLIB = libdiaglib_standalone.so
else
  DIAGLIB = libdiaglib.so
endif

# compiler flags
CFLAGS = $(INCLUDE)
CFLAGS += $(DBGFLAGS)
CFLAGS += -Wall -c -fPIC -flto -pthread -falign-functions=16 -falign-jumps=16 -falign-loops=16 $(DEFINES)

CFLAGS += -Wextra -Wno-missing-field-initializers

# link flags. Apply -soname only for primary lib to avoid ldconfig conflicts
LDFLAGS = -pthread -shared -flto
ifeq ($(standalone),)
  LDFLAGS += -Wl,-soname,libdiaglib.so.1.5.0
endif

all: $(cpp_objects) $(c_objects) link

$(cpp_objects): %.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@ 

link:
	ar -rcsu libdiaglib.a $(cpp_objects) $(c_objects)
	$(CC) $(LDFLAGS) -o $(DIAGLIB) $(cpp_objects) $(c_objects) -lrt

ifeq ($(wildcard $(WRLPATH)),$(WRLPATH)) # PPC link
 ifneq ($(standalone),1)
	mv libdiaglib.a /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib
 endif
else # x86 link
 ifneq ($(standalone),1)
	cp -p libdiaglib.so /usr/lib
	ldconfig
 endif
endif

.PHONY:	clean
clean:
	rm -rf *.o
#	rm -rf *.so  # don't remove .so or .a files, we now have different possible configurations now so need to leave them, JHB Feb 2024
#	rm -rf *.a
	rm -rf *.map
	rm -rf *.scc
ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
	# PowerPC P2020 clean
	rm -rf /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib/libdiaglib*
else
	# x86 clean
#	rm -rf /usr/lib/libdiaglib*
endif
