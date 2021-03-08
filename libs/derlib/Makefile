# $Header: /root/Signalogic_YYYYvNN/DirectCore/lib/derlib/Makefile
#
# Description: Makefile for DER decoding library
#
# Purpose: Support ETSI LI HI2 and HI3 DER encoded data per ASN.1 format
#
# Copyright (C) Signalogic Inc. 2021
#
# Revision History
#
#  Created Feb 2021 JHB

# set install path var, from lib/derlib folder SigSRF software install path is 3 levels up
INSTALLPATH=../../..

WRLPATH=/opt/WindRiver/wrlinux-4

ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
  # PowerPC build
  export PATH := /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/x86-linux2/:/opt/WindRiver/wrlinux-4/layers/wrll-toolchain-4.4a-341/powerpc/toolchain/x86-linux2/bin:$(PATH)
  CC = /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/x86-linux2/powerpc-wrs-linux-gnu-ppc_e500v2-glibc_small-gcc
  DEFINES = -D_WRL_
else
  # x86 build
  CC = /usr/bin/g++
  DEFINES = -D_X86
endif

DERLIBPATH=$(INSTALLPATH)/DirectCore/lib/derlib
DERLIBFILECHECK=$(DERLIBPATH)/der.h

ifeq ($(wildcard $(DERLIBFILECHECK)),$(DERLIBFILECHECK))
  DERLIB_INSTALLED = y
else
  DERLIB_INSTALLED = n
endif

# include paths
INCLUDE = -I. -I$(INSTALLPATH) -I$(INSTALLPATH)/DirectCore/include -I$(INSTALLPATH)/shared_include

cpp_objects = derlib.o
# libwandder c code items currently not used
c_objects = # decoder.o encoder.o itemhandler.o etsili.o link_22.o

#comment/uncomment the following line to turn debug on/off
#DEBUG=y

ifeq ($(DEBUG),y)
	DBGFLAGS= -O2 -g3 -DDEBUGON
else
	DBGFLAGS= -O3 -g
endif

#defines
DEFINES += -D_LINUX_ -D_SIGRT

ifeq ($(DERLIB_INSTALLED),y)
  DEFINES += -D_DERLIB_INSTALLED_
endif

# compiler flags
CC_FLAGS = $(INCLUDE)
CC_FLAGS += $(DBGFLAGS)
CC_FLAGS += -Wall -c -fPIC -flto -pthread -falign-functions=16 -falign-jumps=16 -falign-loops=16 $(DEFINES)

CC_FLAGS += -Wextra -Wno-missing-field-initializers

# link flags
LIB_FLAGS = -pthread -shared -Wl,-soname,libderlib.so.1.0.0 -flto

all: $(cpp_objects) $(c_objects) link

$(cpp_objects): %.o: %.cpp
	$(CC) $(CC_FLAGS) $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CC_FLAGS) $< -o $@ 

link:
	ar -rcsu libderlib.a $(cpp_objects) $(c_objects)
	$(CC) $(LIB_FLAGS) -o libderlib.so $(cpp_objects) $(c_objects) -lrt

ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
	# PPC link
	mv libderlib.a /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib
else
	# x86 link
	cp -p libderlib.so /usr/lib
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
	rm -rf /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib/libderlib*
else
	# x86 clean
	rm -rf /usr/lib/libderlib*
endif
