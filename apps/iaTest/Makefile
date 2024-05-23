# $Header: /root/Signalogic/apps/iaTest/Makefile
# 
# Description:  Makefile for "iaTest" Application
#   
# Purpose:  Image analytics demo program for OpenCV test and CPU vs coCPU performance comparison
#
# Targets:
#  x86:   Gentoo Linux 2009 (2.6.25), CentOS 5.3 (2.6.18), Fedora Core 11 (2.6.29), Ubuntu 12.04
#  PPC:   MPC7447A/7448, Gentoo Linux 2.6.9 or 2.6.24
#  coCPU: TI c66x
#
# Copyright (C) Signalogic Inc. 2014-2024
#
# Revision History
#  Created May 2015, AKM    
#  Modified Jun 2017 JHB, added coCPU shared C objects
#  Modified Sep 2019 JHB, add diaglib and stublib to link.  Diaglib is needed for event logging (Log_RT), stublib is needed for diaglib references to pktlib (no need to bring in pktlib)
#  Modified Aug 2020 JHB, edit INCLUDES var to allow includes specified as "shared_include/xxx.h" 
#  Modified Feb 2021 JHB, add CPPC and CPPC_FLAGS to separate building c and cpp objects with gcc and g++, remove link reference to libexplain
#  Modified Jan 2022 JHB, fix INCLUDES to give correct path to find core_c.h (and other header files included by ia.c --> cv.h). On Sig lab systems this problem doesn't appear because OpenCV is installed (which sets up /usr/include/opencv2/core/core_c.h etc) but on user systems that's not assumed and these header files were not being found
#  Modified Jan 2022 JHB, set OPENCV_LIBS var depending on whether OpenCV is found to be installed
#  Modified Jan 2022 JHB, build all targets with g++, add -Wno-deprecated-xx flags to remove warnings in gcc 5.3.x and higher
#  Modified Feb 2023 JHB, in OPENCV_LIBS var add explicit reference to -l:libstdc++.so.6.0.21 for Redhat dual ABI issues in gcc 5.x.x. Note this must be before -lstdc++ in the link order (see link target below)
#  Modified May 2024 JHB, implement INSTALLPATH, use g++ compiler for iaTest.cpp (converted from iaTest.c), bring flag naming up to date
#  Modified May 2024 JHB, add -L and -rpath linker options for lib/diaglib so diaglib_standalone.so can be linked and found at run-time

# determine if OpenCV is installed on the system
OPENCVPATH=/usr/lib
OPENCVFILECHECK=$(OPENCVPATH)/libopencv_core.so
ifeq ($(wildcard $(OPENCVFILECHECK)),$(OPENCVFILECHECK))
   OPENCV_INSTALLED = y
else
   OPENCV_INSTALLED = n
endif

CPP = /usr/bin/g++
CC = /usr/bin/gcc

# set install path var, from iaTest folder SigSRF software install path is 4 levels up (without symlinks)
INSTALLPATH:=$(shell pwd)/../../../..

INCLUDES = -I$(INSTALLPATH)/DirectCore/include -I$(INSTALLPATH) -I$(INSTALLPATH)/shared_include -I$(INSTALLPATH)/DirectCore/apps/common -I$(INSTALLPATH)/mCPU_target/Ia

# OpenCV coCPU includes
INCLUDES += -I.$(INSTALLPATH)/mCPU_target/opencv/include -I$(INSTALLPATH)/mCPU_target/opencv/core  -I$(INSTALLPATH)/mCPU_target/core/include/ -I$(INSTALLPATH)/mCPU_target/opencv/include/opencv -I$(INSTALLPATH)/mCPU_target/opencv/legacy/include -I$(INSTALLPATH)/mCPU_target/opencv/core/include -I$(INSTALLPATH)/mCPU_target/opencv/imgproc/include -I$(INSTALLPATH)/mCPU_target/opencv/video/include -I$(INSTALLPATH)/mCPU_target/opencv/features2d/include -I$(INSTALLPATH)/mCPU_target/opencv/flann/include -I$(INSTALLPATH)/mCPU_target/opencv/calib3d/include -I$(INSTALLPATH)/mCPU_target/opencv/objdetect/include

# INCLUDES += -I../../../../mCPU_target/opencv/dynamicuda/include

CFLAGS = $(INCLUDES) 
CFLAGS += -g3 -O3 -D_FILE_OFFSET_BITS=64 -D_LINUX_ -D_IA_ -D_USE_IA_ -Wno-deprecated-declarations -Wno-deprecated
CPPFLAGS = $(CFLAGS) -std=c++0x

ifdef _use_enmgr_
   LDFLAGS = -lenmgr
else
   LDFLAGS =
endif

# linker search paths
LINKER_INCLUDES = -L$(INSTALLPATH)/lib/opencv -L$(INSTALLPATH)/DirectCore/lib/diaglib

#add -rpath option for lib/diaglib, JHB May 2024
LDFLAGS += -Wl,-rpath=$(INSTALLPATH)/DirectCore/lib/diaglib

# link with installed OpenCV libs if possible, otherwise use ones included in SigSRF SDK / demo
ifeq ($(OPENCV_INSTALLED),y)
  OPENCV_LIBS = -lopencv_core -lopencv_imgproc
else
  OPENCV_LIBS = -lopencv_core_sig -lopencv_imgproc_sig -l:libstdc++.so.6.0.21
  #add -rpath option for lib/opencv, JHB May 2024
  LDFLAGS += -Wl,-rpath=$(INSTALLPATH)/DirectCore/lib/opencv
endif

cpp_common_objects = timer.o getUserInterface.o cmdLineOpt.o
c_common_objects = keybd.o
coCPU_shared_ia_c_objects = ia.o yuv.o
coCPU_shared_stream_c_objects = writestream.o readstream.o
lib_cpp_objects = streamlib.o
cpp_objects = iaTest.o

all: $(cpp_common_objects) $(c_common_objects) $(coCPU_shared_ia_c_objects) $(coCPU_shared_stream_c_objects) $(lib_cpp_objects) $(cpp_objects) link

$(cpp_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(c_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(coCPU_shared_ia_c_objects): %.o: $(INSTALLPATH)/mCPU_target/Ia/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(coCPU_shared_stream_c_objects): %.o: $(INSTALLPATH)/mCPU_target/Stream/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(lib_cpp_objects): %.o: $(INSTALLPATH)/apps/streamTest/%.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(cpp_objects): %.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@ 

link:
	$(CPP) $(LDFLAGS) $(cpp_common_objects) $(c_common_objects) $(coCPU_shared_ia_c_objects) $(coCPU_shared_stream_c_objects) $(lib_cpp_objects) $(cpp_objects) $(LINKER_INCLUDES) -o ./iaTest $(OPENCV_LIBS) -lstdc++ -lpthread -lhwmgr -lhwlib -lcimlib -ldiaglib_standalone -lstublib

.PHONY: clean all
clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf *.so
	rm -rf *.map
	rm -rf *.scc 
	rm -rf iaTest
