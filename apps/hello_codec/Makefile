# $Header: /install_path/apps/Signalogic/mediaTest/hello_codec/Makefile
# 
# Copyright (C) Signalogic Inc. 2022
#
# Description: Makefile for "hello_codec" minimal codec usage example and integration reference
#
# Purposes
#
# 1) Demonstrate minimum usage for SigSRF codecs
# 2) Provide a simplified reference / starting point for customer integration
# 3) Highlight where advanced functionality may be added (or has been implemented in mediaTest), for example sampling rate conversion, user-defined media processing, etc
#
# Notes
#
#  -see "build libs" below for detailed notes on lib usage and minimum libs needed
#  -this example does not include audio file or USB I/O, intermediate coded output file I/O, sampling rate conversion, multichannel audio, etc. For a complete implementation, see x86_mediaTest() in x86_mediaTest.c
#  -notwithstanding the above note, a simple "write result to wav file" is included at tail end of hello_codec.c, to allow convenient verification of codec output
#
# Revision History
#
#  Created Aug 2022 JHB
#  Modified Sep 2022 JHB, modify USE_4p6_CODEC_LIBS var for Debian
#  Modified Oct 2022 JHB, add codec_libs_version=N.nn cmd line option to force build with specific codec libs. For EVS_INSTALLED var, change logic to use * in wildcard() to look for different lib versions. Add logic to check for available codec libs and handle case where a selected version is not available

ifeq ($(no_codecs),1)
  no_evs = 1
  no_melp = 1
  no_g729 = 1
  no_g726 = 1
  no_amrnb = 1
  no_amrwb = 1
  no_amrwbplus = 1
endif

# set install path var, from hello_codec folder SigSRF software install path is 5 levels up
INSTALLPATH=../../../../..

# auto-discover which codecs are installed. no_xxx indicates an override to exclude xxx from the build

ifeq ($(no_evs),1)
  EVS_INSTALLED = n
else
  EVSPATH=/usr/lib/libevscom_sig.x86*.so  # use wildcard to allow for different lib versions based on Linux distro and/or OS version
  ifneq ($(wildcard $(EVSPATH)),"")
    EVS_INSTALLED = y
  else
    EVS_INSTALLED = n
  endif
endif

ifeq ($(no_melp),1)
  MELPE_INSTALLED = n
else
  MELPEPATH=/usr/lib/libmelpe_sig.x86.so
  ifeq ($(wildcard $(MELPEPATH)),$(MELPEPATH))
    MELPE_INSTALLED = y
  else
    MELPE_INSTALLED = n
  endif
endif

ifeq ($(no_g729),1)
  G729AB_INSTALLED = n
else
  G729ABPATH=/usr/lib/libg729ab_sig.x86.so
  ifeq ($(wildcard $(G729ABPATH)),$(G729ABPATH))
    G729AB_INSTALLED = y
  else
    G729AB_INSTALLED = n
  endif
endif

ifeq ($(no_g726),1)
  G726_INSTALLED = n
else
  G726PATH=/usr/lib/libg726_sig.x86.so
  ifeq ($(wildcard $(G726PATH)),$(G726PATH))
    G726_INSTALLED = y
  else
    G726_INSTALLED = n
  endif
endif

ifeq ($(no_amrnb),1)
  AMR_INSTALLED = n
else
  AMRPATH=/usr/lib/libamrdec_sig.x86.so
  ifeq ($(wildcard $(AMRPATH)),$(AMRPATH))
    AMR_INSTALLED = y
  else
    AMR_INSTALLED = n
  endif
endif

ifeq ($(no_amrwb),1)
  AMRWB_INSTALLED = n
else
  AMRWBPATH=/usr/lib/libamrwbdec_sig.x86.so
  ifeq ($(wildcard $(AMRWBPATH)),$(AMRWBPATH))
    AMRWB_INSTALLED = y
  else
    AMRWB_INSTALLED = n
  endif
endif

ifeq ($(no_amrwbplus),1)
  AMRWBPLUS_INSTALLED = n
else
  AMRWBPLUSPATH=/usr/lib/libamrwbplusdec_sig.x86.so
  ifeq ($(wildcard $(AMRWBPLUSPATH)),$(AMRWBPLUSPATH))
    AMRWBPLUS_INSTALLED = y
  else
    AMRWBPLUS_INSTALLED = n
  endif
endif

# compiler path, g++ is required because some source codes are .cpp
CC = /usr/bin/g++
ifneq ($(wildcard $(CC)),$(CC))
  echo Make error message:  g++ compiler path $(CC) not found
endif

# includes
INCLUDES = -I../ -I$(INSTALLPATH)/DirectCore/include -I$(INSTALLPATH) -I$(INSTALLPATH)/shared_include -I$(INSTALLPATH)/DirectCore/apps/common

# compile flags
CC_FLAGS = $(INCLUDES) 

# compile / build flags
CC_FLAGS += -Wall -g3 -O3 -flto -pthread -std=c++0x

# add necessary defines
#   _LINUX_     if not set, then a Win32 system is assumed
#   _USE_PCIE_  applies to coCPU option (ignored if no coCPU accelerator card). If not set than ATCA or other hardware is assumed
#   _SIGRT_     use real-time options where applicable
#   _X86        host platform is x86
CC_FLAGS += -D_FILE_OFFSET_BITS=64 -D_LINUX_ -D_USE_PCIE_ -D_SIGRT -D_X86

ifeq ($(MELPE_INSTALLED),y)
  CC_FLAGS += -D_MELPE_INSTALLED_
endif

ifeq ($(G729AB_INSTALLED),y)
  CC_FLAGS += -D_G729AB_INSTALLED_
endif

ifeq ($(G726_INSTALLED),y)
  CC_FLAGS += -D_G726_INSTALLED_
endif

ifeq ($(AMR_INSTALLED),y)
  CC_FLAGS += -D_AMR_INSTALLED_
endif

ifeq ($(AMRWB_INSTALLED),y)
  CC_FLAGS += -D_AMRWB_INSTALLED_
endif

ifeq ($(AMRWBPLUS_INSTALLED),y)
  CC_FLAGS += -D_AMRWBPLUS_INSTALLED_
endif

LINKER_INCLUDES = -L$(INSTALLPATH)/DirectCore/lib -L/usr/lib

# determine linking for codec libs built with --ffast-math
#
# -older systems require older codec libs
# -depends on Linux distro, system configuration, and version of installed SigSRF found

include /etc/os-release  # source contents of /etc/os-release file; this will set several Makefile variables such as NAME, VERSION_ID, ID, etc
VERSION_ID := $(patsubst "%",%,$(VERSION_ID))  # remove quotes

ifneq (,$(findstring CentOS,$(NAME)))
  VER_MAJOR := $(shell echo $(VERSION_ID) | cut -f1 -d.)
  VER_MINOR := $(shell echo $(VERSION_ID) | cut -f2 -d.)
  # CentOS version <= 7.07
  USE_4p6_CODEC_LIBS := $(shell [ $(VER_MAJOR) -le 7 -o \( $(VER_MAJOR) -eq 7 -a $(VER_MINOR) -ge 7 \) ] && echo true)
else ifneq (,$(findstring Ubuntu,$(NAME)))
  VER_MAJOR := $(shell echo $(VERSION_ID) | cut -f1 -d.)
  VER_MINOR := $(shell echo $(VERSION_ID) | cut -f2 -d.)
  # Ubunto version <= 12.04
  USE_4p6_CODEC_LIBS := $(shell [ $(VER_MAJOR) -le 12 -o \( $(VER_MAJOR) -eq 12 -a $(VER_MINOR) -ge 4 \) ] && echo true)
else  # for os-release files without "VERSION_ID" field (like Debian) we assume newer libs. May need to change this on further OS testing
  USE_4p6_CODEC_LIBS :=
endif

ifneq ($(USE_4p6_CODEC_LIBS),true)  # gcc 11.3 libs is the default
  USE_11p3_CODEC_LIBS := true
endif

# check cmd line overrides for which codec libs to link
ifeq ($(codec_libs_version),4.6)
  USE_4p6_CODEC_LIBS := true
  USE_11p3_CODEC_LIBS :=
endif

ifeq ($(codec_libs_version),11.3)
  USE_11p3_CODEC_LIBS := true
  USE_4p6_CODEC_LIBS :=
endif

ifeq ($(EVS_INSTALLED),y)

# determine versions of lib files available (user might delete one or the other from the base install, yes this has happened)

  EVSPATH:=/usr/lib/libevscom_sig.x86_gcc4.6.so
  ifeq ($(wildcard $(EVSPATH)),$(EVSPATH))
    4p6_EVS_LIBS_FOUND := true
  endif
  EVSPATH:=/usr/lib/libevscom_sig.x86_gcc11.3.so
  ifeq ($(wildcard $(EVSPATH)),$(EVSPATH))
    11p3_EVS_LIBS_FOUND := true
  endif

# decide which of available versions should be linked in the build

  ifeq ($(USE_4p6_CODEC_LIBS),true)
    ifneq ($(4p6_EVS_LIBS_FOUND),true)
      ifeq (,$(findstring clean,$(MAKECMDGOALS)))
        $(info gcc 4.6 EVS codec libs not found)
      endif
      USE_4p6_CODEC_LIBS :=
    endif
  endif

  ifneq ($(USE_4p6_CODEC_LIBS),true)
    ifeq ($(11p3_EVS_LIBS_FOUND),true)
      USE_11p3_CODEC_LIBS := true
    else
      ifeq (,$(findstring clean,$(MAKECMDGOALS)))
        $(info gcc 11.3 EVS codec libs not found)
      endif
      USE_11p3_CODEC_LIBS :=
    endif
  endif

  ifeq ($(USE_4p6_CODEC_LIBS),true)
    ifeq ($(codec_libs_version),4.6)
      $(info codec_libs_version=4.6 on cmd line, building with gcc 4.6 EVS codec libs)  # print build info if cmd line option was given
    endif
    CODEC_LIBS += -levsenc_sig.x86_gcc4.6
    CODEC_LIBS += -levsdec_sig.x86_gcc4.6
    CODEC_LIBS += -levscom_sig.x86_gcc4.6
  else ifeq ($(USE_11p3_CODEC_LIBS),true)
    ifeq ($(codec_libs_version),11.3)
      $(info codec_libs_version=11.3 on cmd line, building with gcc 11.3 EVS codec libs)  # print build info if cmd line option was given
    endif
    CODEC_LIBS += -levsenc_sig.x86_gcc11.3
    CODEC_LIBS += -levsdec_sig.x86_gcc11.3
    CODEC_LIBS += -levscom_sig.x86_gcc11.3
  else
    ifeq (,$(findstring clean,$(MAKECMDGOALS)))
      $(info building with generic EVS codec libs)  # print build info if generic fallback
    endif
    CODEC_LIBS += -levsenc_sig.x86
    CODEC_LIBS += -levsdec_sig.x86
    CODEC_LIBS += -levscom_sig.x86
  endif
endif

ifeq ($(MELPE_INSTALLED),y)
  CODEC_LIBS += -lmelpe_sig.x86
endif

ifeq ($(G729AB_INSTALLED),y)
  CODEC_LIBS += -lg729ab_sig.x86
endif

ifeq ($(G726_INSTALLED),y)
  CODEC_LIBS += -lg726_sig.x86
endif

ifeq ($(AMR_INSTALLED),y)
  CODEC_LIBS += -lamrdec_sig.x86
  CODEC_LIBS += -lamrenc_sig.x86
endif

ifeq ($(AMRWB_INSTALLED),y)
  CODEC_LIBS += -lamrwbdec_sig.x86
  CODEC_LIBS += -lamrwbenc_sig.x86
endif

ifeq ($(AMRWBPLUS_INSTALLED),y)
  CODEC_LIBS += -lamrwbplusenc_sig.x86
  CODEC_LIBS += -lamrwbplusdec_sig.x86
endif

# build libs
#
#  lib      Description                                     R/O (Required/Optional)
#  ---      -----------                                     -----------------------
#  CODEC_LIBS  encoder, decoder (and in some cases          R (at least one codec)
#                common) libs for various codecs
#  voplib      codec interface lib                          R
#  diaglib     display and file logging of event, status,   O
#                and info messages
#  hwlib       DirectCore platform management               O (optional but needed for demo / non-licensed use)
#  cimlib      command line handling                        O
#  stublib     defines null APIs for codecs not installed   O

SIG_LIBS = -lvoplib $(CODEC_LIBS) -ldiaglib -lhwlib -lcimlib -lstublib

AR_FLAGS += -flto -Xlinker -Map=hello_codec.map

cpp_common_objects = timer.o getUserInterface.o cmdLineOpt.o
c_common_objects = keybd.o
c_mediatest_objects = transcoder_control.o cmd_line_interface.o 
c_objects = hello_codec.o

# build targets
all: $(cpp_common_objects) $(c_common_objects) $(c_mediatest_objects) $(c_objects) link

$(cpp_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.cpp
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_mediatest_objects): %.o: $(INSTALLPATH)/apps/mediaTest/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

# link object files, essential libs, and cimlib which provides cmd line handling
link:
	$(CC) $(AR_FLAGS) $(cpp_common_objects) $(c_common_objects) $(c_mediatest_objects) $(c_objects) $(LINKER_INCLUDES) -o ./hello_codec -lstdc++ $(SIG_LIBS) -ldl -lpthread -lm

.PHONY: clean all 
clean:
	rm -rf *.o
	rm -rf hello_codec
