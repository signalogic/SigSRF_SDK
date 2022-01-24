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
# Copyright (C) Signalogic Inc. 2014-2022
#
# Revision History
#  Created May 2015, AKM    
#  Modified Jun 2017 JHB, added coCPU shared C objects
#  Modified Sep 2019 JHB, add diaglib and stublib to link.  Diaglib is needed for event logging (Log_RT), stublib is needed for diaglib references to pktlib (no need to bring in pktlib)
#  Modified Aug 2020 JHB, edit INCLUDES var to allow includes specified as "shared_include/xxx.h" 
#  Modified Feb 2021 JHB, add CPPC and CPPC_FLAGS to separate building c and cpp objects with gcc and g++, remove link reference to libexplain
#  Modified Jan 2022 JHB, fix INCLUDES to give correct path to find core_c.h (and other header files included by ia.c --> cv.h). On Sig lab systems this problem doesn't appear because OpenCV is installed (which sets up /usr/include/opencv2/core/core_c.h etc) but on user systems that's not assumed and these header files were not being found

# determine if OpenCV is installed on the system
OPENCVPATH=/usr/lib
OPENCVFILECHECK=$(OPENCVPATH)/libopencv_core.so
ifeq ($(wildcard $(OPENCVFILECHECK)),$(OPENCVFILECHECK))
   OPENCV_INSTALLED = y
else
   OPENCV_INSTALLED = n
endif

CPPC = /usr/bin/g++
CC = /usr/bin/gcc

INCLUDES = -I./ -I../../../include -I../../../.. -I../../../../shared_include -I../../common  -I../../../../mCPU_target/Ia
# OpenCV coCPU includes
INCLUDES += -I../../../../mCPU_target/opencv/include -I../../../../mCPU_target/opencv/core  -I../../../../mCPU_target/core/include/ -I../../../../mCPU_target/opencv/include/opencv -I../../../../mCPU_target/opencv/legacy/include -I../../../../mCPU_target/opencv/core/include -I../../../../mCPU_target/opencv/imgproc/include -I../../../../mCPU_target/opencv/video/include -I../../../../mCPU_target/opencv/features2d/include -I../../../../mCPU_target/opencv/flann/include -I../../../../mCPU_target/opencv/calib3d/include -I../../../../mCPU_target/opencv/objdetect/include

# INCLUDES += -I../../../../mCPU_target/opencv/dynamicuda/include

CC_FLAGS = $(INCLUDES) 
CC_FLAGS += -g3 -O3 -D_FILE_OFFSET_BITS=64 -D_LINUX_ -D_IA_ -D_USE_IA_
CPPC_FLAGS = $(CC_FLAGS) -std=c++0x

ifdef _use_enmgr_
   AR_FLAGS = -lenmgr
else
   AR_FLAGS =
endif
AR_FLAGS += -L../../../lib/opencv

# link with installed OpenCV libs if possible, otherwise use ones included in SigSRF SDK / demo
ifeq ($(OPENCV_INSTALLED),y)
  OPENCV_LIBS = -lopencv_core -lopencv_imgproc
else 
  OPENCV_LIBS = -lopencv_core_sig -lopencv_imgproc_sig
endif

cpp_objects = timer.o getUserInterface.o cmdLineOpt.o
lib_cpp_objects = streamlib.o
c_objects = keybd.o
coCPU_shared_ia_c_objects = ia.o yuv.o
coCPU_shared_stream_c_objects = writestream.o readstream.o

all: iaTest.o $(cpp_objects) $(c_objects) $(coCPU_shared_ia_c_objects) $(coCPU_shared_stream_c_objects) $(lib_cpp_objects) link

iaTest.o:
	$(CPPC) $(CPPC_FLAGS) -c iaTest.c 

$(cpp_objects): %.o: ../../common/%.cpp
	$(CPPC) $(CPPC_FLAGS) -c $< -o $@ 

$(c_objects): %.o: ../../common/%.c
	$(CPPC) $(CPPC_FLAGS) -c $< -o $@ 

$(coCPU_shared_ia_c_objects): %.o: ../../../../mCPU_target/Ia/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(coCPU_shared_stream_c_objects): %.o: ../../../../mCPU_target/Stream/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(lib_cpp_objects): %.o: ../streamTest/%.cpp
	$(CPPC) $(CPPC_FLAGS) -c $< -o $@ 

link:
	$(CPPC) $(cpp_objects) $(c_objects) $(coCPU_shared_ia_c_objects) $(coCPU_shared_stream_c_objects) $(lib_cpp_objects) iaTest.o $(AR_FLAGS) -o ./iaTest -lstdc++ -lpthread -lhwmgr -lhwlib -lcimlib -ldiaglib $(OPENCV_LIBS) -lstublib

.PHONY: clean all
clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf *.so
	rm -rf *.map
	rm -rf *.scc 
	rm -rf iaTest
