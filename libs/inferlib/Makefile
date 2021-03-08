# $Header: /root/Signalogic_YYYYvNN/DirectCore/lib/inferlib/Makefile
#
# Copyright (C) Signalogic Inc. 2019-2021
#
# Description:  Makefile for packet library
#
# Purpose:      API interface to parsing and processing network packets, including audio, video, and data analytics
#
# Target:       generic x86 or MPC7447A/7448, PowerPC P2020
#
# Linux:        Ubuntu, CentOS, Redhat, Gentoo, WindRiver Linux 4.3 (based on Fedora)
#
# Kernel Ver:   2.4, 2.6.9, 2.6.24, 2.6.25, 2.6.34.9, 3.2
#
# Revision History
#
#  Created Jan 2019 CJ
#  Modified Jan 2021 JHB, create AR_FLAGs (and add -flto), add shared_include to includes, rename top-level source to inferlib.cpp
#  Modified Mar 2021 JHB, add INSTALLPATH to allow builds on relative folder structure, remove unncessary -I include paths

# set install path var, from lib/inferlib folder SigSRF software install path is 3 levels up
INSTALLPATH=../../..

WRLPATH=/opt/WindRiver/wrlinux-4

ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
  export PATH := /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/x86-linux2/:/opt/WindRiver/wrlinux-4/layers/wrll-toolchain-4.4a-341/powerpc/toolchain/x86-linux2/bin:$(PATH)
  CC = /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/x86-linux2/powerpc-wrs-linux-gnu-ppc_e500v2-glibc_small-gcc
  DEFINES = -D_WRL_
else
  CC = /usr/bin/g++  # x86 build
  DEFINES = -D_X86
endif

# include paths
INCLUDES = -I$(INSTALLPATH)/DirectCore/include -I../../.. -I$(INSTALLPATH)/shared_include $(KALDI_INCLUDES)
KALDI_INCLUDES = -I/storage/kaldi/src/ -I/storage/kaldi/tools/openfst/include/ -I/storage/kaldi/tools/portaudio/install/include/

# linked in libs
#KALDI_LIBS = -Wl,-rpath=/storage/kaldi/tools/openfst/lib/libfst.so -lkaldi-online2 -lkaldi-ivector -lkaldi-nnet3 -lkaldi-chain -lkaldi-cudamatrix -lkaldi-decoder -lkaldi-lat -lkaldi-fstext -lkaldi-hmm -lkaldi-feat -lkaldi-transform -lkaldi-gmm -lkaldi-tree -lkaldi-util -lkaldi-matrix -lkaldi-base
KALDI_LIBS = -Wl,-rpath=../../../openfst/lib/libfst.so -Wl,-rpath=../../../kaldi/lib
KALDI_LIBS += -lkaldi-online2 -lkaldi-ivector -lkaldi-nnet3 -lkaldi-chain -lkaldi-cudamatrix -lkaldi-decoder -lkaldi-lat -lkaldi-fstext -lkaldi-hmm -lkaldi-feat -lkaldi-transform -lkaldi-gmm -lkaldi-tree -lkaldi-util -lkaldi-matrix -lkaldi-base

cpp_objects = inferlib.o
c_objects = 

#comment/uncomment the following line to turn debug on/off
#DEBUG=y

ifeq ($(DEBUG),y)
	DBGFLAGS= -O2 -g3 -DDEBUGON
else
	DBGFLAGS= -O3 -flto
endif

#defines
KALDI_DEFINES = -DKALDI_DOUBLEPRECISION=0 -DHAVE_EXECINFO_H=1 -DHAVE_CXXABI_H -DHAVE_ATLAS
DEFINES += -D_LINUX_ -D_SIGRT $(KALDI_DEFINES)

# compile flags
CC_FLAGS = $(INCLUDES)
CC_FLAGS += $(DBGFLAGS)
CC_FLAGS += -std=c++11 -Wall -Wno-sign-compare -Wno-unused-local-typedefs -Wno-deprecated-declarations -Winit-self -c -fPIC -pthread -nostartfiles $(DEFINES)

ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
CC_FLAGS += -D_WRL_
endif

#link flags
AR_FLAGS = $(DBGFLAGS) -shared -Xlinker -Map=libinferlib.map

all: $(cpp_objects) $(c_objects) link

$(cpp_objects): %.o: %.cpp
	$(CC) $(CC_FLAGS) $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CC_FLAGS) $< -o $@ 

link:
	ar -rcsu libinferlib.a $(cpp_objects) $(c_objects)
	g++ $(AR_FLAGS) $(cpp_objects) $(c_objects) $(KALDI_LIBS) -lrt -o libinferlib.so

ifeq ($(wildcard $(WRLPATH)),$(WRLPATH))
	# PPC link
	mv libinferlib.a /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib
else
	# x86 link
#	cp -p libinferlib.a /usr/lib
	cp -p libinferlib.so /usr/lib
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
	rm -rf /opt/WindRiver/wrlinux-4/sysroots/adsp2-glibc_small/sysroot/te500v2/usr/lib/libinferlib*
else
	# x86 clean
	rm -rf /usr/lib/libinferlib.*
endif
