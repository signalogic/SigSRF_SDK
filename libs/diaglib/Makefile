# $Header: /root/Signalogic_YYYYvNN/DirectCore/lib/diaglib/Makefile
#
# Copyright (C) Signalogic Inc. 2017-2020
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
INCLUDE = -I../include -I../../include -I../../lib/hwlib -I../../.. -I../../../shared_include -I../pktlib -I../../lib/common

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
DEFINES += -D_USE_CM_ -D_MAVMOD_UAG_ -D_SIGMOD_RTAFv5_ -D_SIGRT

# compiler flags
CC_FLAGS = $(INCLUDE)
CC_FLAGS += $(DBGFLAGS)
CC_FLAGS += -Wall -c -fPIC -flto -pthread -falign-functions=16 -falign-jumps=16 -falign-loops=16 -D_LINUX_ -D__LIBRARYMODE__ -D__UNIFIEDMODE__ -D_DIRECTCORE_ -D_DIRECTCOREV5_ $(DEFINES)

CC_FLAGS += -Wextra -Wno-missing-field-initializers

# link flags
LIB_FLAGS = -pthread -shared -Wl,-soname,libdiaglib.so.1.5.0 -flto

all: $(cpp_objects) $(c_objects) link

$(cpp_objects): %.o: %.cpp
	$(CC) $(CC_FLAGS) $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CC_FLAGS) $< -o $@ 

link:
	ar -rcsu libdiaglib.a $(cpp_objects) $(c_objects)
	$(CC) $(LIB_FLAGS) -o libdiaglib.so $(cpp_objects) $(c_objects) -lrt

ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
	# PPC link
	mv libdiaglib.a /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib
else
	# x86 link
	cp -p libdiaglib.so /usr/lib
	ldconfig
endif

.PHONY:	clean
clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf *.so
	rm -rf *.map
	rm -rf *.scc
ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
	# PowerPC P2020 clean
	rm -rf /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib/libdiaglib*
else
	# x86 clean
	rm -rf /usr/lib/libdiaglib*
endif
