# $Header: /root/Signalogic_YYYYvNN/DirectCore/lib/alglib/Makefile
#
# Copyright (C) Signalogic Inc. 2017-2024
#
# Description:  Makefile for alg library
#
# Purpose:      API interface for generic algorithms
#
# Target:       generic x86 or MPC7447A/7448, PowerPC P2020
#
# Linux:        Ubuntu, CentOS, Redhat, Gentoo, WindRiver Linux 4.3 (based on Fedora)
#
# Kernel Ver:   2.4, 2.6.9, 2.6.24, 2.6.25, 2.6.34.9, 3.2
#
# Revision History
#
#  Created Jun 2018 CJ
#  Modified Nov 2018 JHB, enable -O3, remove -pthread and -fPIC from lib target (these are compiler options)
#  Modified Aug 2019 JHB, add -flto option
#  Modified Jul 2020 JHB, add -Wl,-soname,libalglib.so.major.minor.internal to link target. MAJOR is incremented for API changes (existing APIs or global vars are removed or existing API params are changed). MINOR is incremented for new APIs or global vars. INTERNAL is incremented for bug fixes or other internal modifications with no effect on the ABI
#  Modified Feb 2022 JHB, modify INCLUDES to allow "shared_include/xxx.h" header file includes
#  Modified Feb 2024 JHB, rename CC_FLAGS to CFLAGS and LIB_FLAGS to LDFLAGS

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
INCLUDES = -I../../include -I../../lib/common -I../../../shared_include -I../../..

cpp_objects = 
c_objects = alglib.o fs_conv.o agc.o

#comment/uncomment the following line to turn debug on/off
# DEBUG=y

ifeq ($(DEBUG),y)
	DBGFLAGS= -O2 -g3 -DDEBUGON
else
	DBGFLAGS= -O3 -flto
endif

#defines
DEFINES = -D_X86 -D_USE_CM_ -D_MAVMOD_UAG_ -D_SIGMOD_RTAFv5_ -D_SIGRT

CFLAGS = $(INCLUDES)
CFLAGS += $(DBGFLAGS)
CFLAGS += -Wall -c -fPIC -pthread -nostartfiles -D_LINUX_ -D__LIBRARYMODE__ -D__UNIFIEDMODE__ -D_DIRECTCORE_ -D_DIRECTCOREV5_ $(DEFINES)

# link flags
LDFLAGS = $(DBGFLAGS) -shared -Wl,-soname,libalglib.so.1.2.1 -flto

all: $(cpp_objects) $(c_objects) link

$(cpp_objects): %.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@ 

link:
	ar -rcsu libalglib.a $(cpp_objects) $(c_objects)
	$(CC) $(LDFLAGS) -o libalglib.so $(cpp_objects) $(c_objects) -lrt

ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
	# PPC link
	mv libalglib.a /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib
else
	# x86 link
#	mv libalglib.a /usr/lib
	cp -p libalglib.so /usr/lib
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
	rm -rf /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib/libalglib*
else
	# x86 clean
	rm -rf /usr/lib/libalglib*
endif
