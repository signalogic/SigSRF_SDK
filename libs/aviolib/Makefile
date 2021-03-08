# $Header: /root/Signalogic_YYYYvNN/DirectCore/lib/aviolib/Makefile
#
# Copyright (C) Signalogic Inc. 2018-2021
#
# Description:  Makefile for aviolib library
#
# Purpose:      API interface to audio and video I/O, including USB audio and cameras, GbE cameras, etc
#
# Target:       generic x86
#
# Linux:        Ubuntu, CentOS, Redhat, Gentoo
#
# Kernel Ver:   3.2 and higher
#
# Revision History
#
#  Created March 2018 JHB
#  Modified Mar 2021 JHB, remove unnecessary includes and defines, simplify flags
#  Modified Mar 2021 JHB, change ALSA detection path to avoid problem building aviolib when no sound devices happen to be connected. See comments

# ALSAPATH=/proc/asound  # may not exist if no sound devices / =ards are connected / installed after a reboot
ALSAPATH=/usr/lib/x86_64-linux-gnu/libasound*
ifneq ($(wildcard $(ALSAPATH)),)
  ALSA_INSTALLED = y
else
  ALSA_INSTALLED = n
endif

CC = /usr/bin/gcc  # x86 build

# include paths
#INCLUDE = -I../include -I../../include -I../../lib/hwlib -I../../../shared_include -I../../../SIG_LIBS -I../voplib -I../diaglib -I. -I../ -I../../lib/common
INCLUDES = -I../include -I../../include -I../ -I../../.. -I../../../shared_include

cpp_objects = 
c_objects = aviolib.o

#comment/uncomment the following line to turn debug on/off
DEBUG=y

ifeq ($(DEBUG),y)
	DBGFLAGS= -O2 -g3 -DDEBUGON
else
	DBGFLAGS= -O2
endif

#defines
DEFINES = -D_X86
ifeq ($(ALSA_INSTALLED),y)
DEFINES += -D_ALSA_INSTALLED_
endif

#**************** compiler flags ******************
CC_FLAGS = $(INCLUDES)
CC_FLAGS += $(DBGFLAGS)
CC_FLAGS += -Wall -c -fPIC -D_LINUX_ -D_SIGRT $(DEFINES)

#*************** shared library flags *****************
SO_FLAGS = $(DBGFLAGS) -fPIC -nostartfiles -pthread -lrt

ifeq ($(ALSA_INSTALLED),y)
LIB_INCLUDES = -lasound -lm
endif

all: $(cpp_objects) $(c_objects) lib

$(cpp_objects): %.o: %.cpp
	$(CC) $(CC_FLAGS) $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CC_FLAGS) $< -o $@ 

lib:
	ar -rcsu libaviolib.a $(cpp_objects) $(c_objects)
	gcc -shared $(SO_FLAGS) -o libaviolib.so $(cpp_objects) $(c_objects) $(LIB_INCLUDES)

# x86 link
#	mv libaviolib.a /usr/lib
	cp -p libaviolib.so /usr/lib
	ldconfig

.PHONY:	clean
clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf *.so
	rm -rf *.map
	rm -rf *.scc

# x86 clean
	rm -rf /usr/lib/libaviolib*
