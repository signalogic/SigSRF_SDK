# $Header: /install_path/apps/Signalogic/mediaTest/Makefile
# 
# Description: Makefile for "mediaTest" test and measurement program
#
# Purpose: (i) media transcoding and packet streaming / processing test program, (ii) example user application showing use of Pktlib and Voplib APIs, (iii) supports optional coCPU card(s) for very high server capacity
# 
# Makefile for mediaTest streaming, transcoding, and audio test and measurement program
#
# Copyright (C) Signalogic Inc. 2015-2021
#
# Revision History
#
#  Created Sep 2015 CJ
#  Modified Jan 2017 CJ, added x86 support
#  Modified Aug2017 JHB, edited SIG_LIBS var to include essential libs
#  Modified Mar 2018 JHB, added aviolib to link (see SIG_LIBS var)
#  Modified Mar 2018 JHB, define _X86 instead of _X86_ (to be consistent with shared_include / coCPU usage)
#  Modified Mar 2018 JHB, add check for ALSA installation, if not installed then don't set _ALSA_INSTALLED_ global define and don't link aviolib
#  Modified Jun 2018 CJ, add checks for non-installed codec libs, omit them in the link if needed
#  Modified Oct 2018 JHB, add install checks for AMR NB, AMR WB, and AMR WB+ codecs
#  Modified Oct 2018 JHB, add -map linker option (which requires -Xlinker option to be recognized)
#  Modified Nov 2018 JHB, add streamlib
#  Modified Dec 2018 JHB, remove unused AMR-WB+ common lib
#  Modified Mar 2019 JHB, add cmd line options to configure ALSA and codec libs linked in customer demo executables.  This is for cases where the customer does not have gcc tools (or does but doesn't want to build) and they will run executables built by Signalogic
#  Modified Jul 2019 JHB, add -Wall to gcc flags
#  Modified Jul 2019 JHB, add -flto linker option
#  Modified Mar 2020 JHB, handle name change of mediaThread_test_app.c to mediaMin.c
#  Modified Apr 2020 JHB, edit INCLUDES var to allow includes specified as "shared_include/xxx.h"
#  Modified Jan 2021 JHB, change ALSA detection to look for /usr/include/alsa folder instead of /proc/asound. ALSA might be installed but not the dev version of ALSA, which has the header files
#  Modified Jan 2021 JHB, add INSTALLPATH, add rpath linker options for version of Kaldi libs used by SigSRF and for ATLAS linear algebra / matrix lib (only if makefile detects Kaldi is installed)
#  Modified Jan 2021 JHB, change c_mediamin_objects target to cpp_mediamin_objects, add cpp_sdp_objects target

# check make cmd line for "no_codecs" option

ifeq ($(no_codecs),1)
   no_melp = 1
   no_g729 = 1
   no_g726 = 1
   no_amrnb = 1
   no_amrwb = 1
   no_amrwbplus = 1
endif

# set install path var, from mediaTest folder SigSRF software install path is 4 levels up
INSTALLPATH=../../../..

# auto-discover whether ALSA, codecs, Kaldi, etc are installed. no_xxx indicates an override to exclude xxx from the build

ifeq ($(no_alsa),1)
  ALSA_INSTALLED = n
else
#  ALSAPATH=/proc/asound
  ALSAPATH=/usr/include/alsa
  ifeq ($(wildcard $(ALSAPATH)),$(ALSAPATH))
    ALSA_INSTALLED = y
  else
    ALSA_INSTALLED = n
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

# currently Kaldi and inference lib are not used by mediaTest. See mediaMin makefile 
no_kaldiasr = 1

ifeq ($(no_kaldiasr),1)
  KALDIASR_INSTALLED = n
else
  # check for any of several libkaldi-xx.so files
  # try SigSRF install first
  KALDIASRPATH=$(INSTALLPATH)/kaldi/lib
  KALDIASRFILECHECK=$(KALDIASRPATH)/libkaldi-base.so

  ifeq ($(wildcard $(KALDIASRFILECHECK)),$(KALDIASRFILECHECK))
    KALDIASR_INSTALLED = y
  else
    # try system path
    KALDIASRPATH=/usr/lib
    KALDIASRFILECHECK=$(KALDIASRPATH)/libkaldi-base.so
    ifeq ($(wildcard $(KALDIASRFILECHECK)),$(KALDIASRFILECHECK))
      KALDIASR_INSTALLED = y
    else
      KALDIASR_INSTALLED = n
    endif
  endif
endif

# compiler path, g++ is required because some source codes are .cpp
CC = /usr/bin/g++
ifneq ($(wildcard $(CC)),$(CC))
  echo Make error message:  g++ compiler path $(CC) not found
endif

# includes
INCLUDES = -I./ -I$(INSTALLPATH)/DirectCore/include -I$(INSTALLPATH) -I$(INSTALLPATH)/shared_include -I$(INSTALLPATH)/DirectCore/apps/common

# compile flags
CC_FLAGS = $(INCLUDES) 

# compile / build flags
CC_FLAGS += -Wall -g3 -O3 -flto -pthread -std=c++0x

# add necessary defines
#   _LINUX_     if not set, then a Win32 system is assumed
#   _USE_PCIE_  applies to coCPU option.  If not set than ATCA or other hardware is assumed
#   _SIGRT_     indicates to use real-time options where applicable
#   _X86        indicates host platform is x86
CC_FLAGS += -D_FILE_OFFSET_BITS=64 -D_LINUX_ -D_USE_PCIE_ -D_SIGRT -D_X86 -DMEDIATEST

ifeq ($(ALSA_INSTALLED),y)
  CC_FLAGS += -D_ALSA_INSTALLED_
endif

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

ifeq ($(KALDIASR_INSTALLED),y)
  CC_FLAGS += -D_KALDIASR_INSTALLED_
endif

LINKER_INCLUDES = -L$(INSTALLPATH)/DirectCore/lib -L/usr/lib

# codec libs, depending on install + configuration
CODEC_LIBS = -levsenc_sig.x86 -levsdec_sig.x86 -levscom_sig.x86

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

# essential libs for user apps (pktlib = packet handling, voplib = voice-over-packet (including codecs), diaglib = diagnostics, hwlib = DirectCore, streamlib = stream groups and merging
SIG_LIBS = -lvoplib $(CODEC_LIBS) -lpktlib -ldiaglib -lhwlib -lalglib -lstreamlib -lderlib

# add aviolib lib if ALSA present on the system (USB audio I/O)
ifeq ($(ALSA_INSTALLED),y)
  SIG_LIBS += -laviolib
endif

ifdef _use_enmgr_
  AR_FLAGS = -lenmgr
else
  AR_FLAGS =
endif

AR_FLAGS += -flto -Xlinker -Map=mediaTest.map

# if Kaldi ASR present on the system, link inferlib, Kaldi libs, and ATLAS and BLAS linear algebra libs. Notes:
# -if not found stublib will fill in inferlib API stubs (which won't be called)
# -inferlib calls kaldi libs which call ATLAS and BLAS libs

ifeq ($(KALDIASR_INSTALLED),y)

  SIG_LIBS += -linferlib
  AR_FLAGS += -Wl,-rpath=$(KALDIASRPATH)

  # try SigSRF install first
  ATLASPATH=$(INSTALLPATH)/blas_atlas

  ifeq ($(wildcard $(ATLASPATH)),$(ATLASPATH))
    AR_FLAGS += -Wl,-rpath=$(ATLASPATH)
  else

    # try system folders (i.e. full download/install of ATLAS and BLAS)
    ATLASPATH=/usr/lib/atlas-base
    ifeq ($(wildcard $(ATLASPATH)),$(ATLASPATH))
      AR_FLAGS += -Wl,-rpath=$(ATLASPATH)
    endif
  endif
endif

cpp_common_objects = timer.o getUserInterface.o cmdLineOpt.o
c_common_objects = keybd.o
cpp_sdp_objects = types.o sdp.o utils.o reader.o writer.o 
cpp_mediaMin_objects = mediaMin.o sdp_app.o session_app.o user_io.o
c_objects = sigMRF_init.o control_thread_task.o codec_thread_task.o transcoder_control.o codec_test_control.o host_c66x_xfer_control.o dummy_packet.o mediaTest.o cmd_line_interface.o packet_flow_media_proc.o x86_mediaTest.o

# build targets
all: $(cpp_common_objects) $(cpp_sdp_objects) $(c_common_objects) $(cpp_mediaMin_objects) $(c_objects) link

$(cpp_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.cpp
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(cpp_sdp_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/sdp/%.cpp
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(cpp_mediaMin_objects): %.o: mediaMin/%.cpp
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

# link object files, essential libs, and cimlib which provides cmd line handling
link:
	$(CC) $(AR_FLAGS) $(cpp_common_objects) $(cpp_sdp_objects) $(c_common_objects) $(c_objects) $(cpp_mediaMin_objects) $(LINKER_INCLUDES) -o ./mediaTest -lstdc++ $(SIG_LIBS) -ldl -lpthread -lcimlib -lstublib

.PHONY: clean all 
clean:
	rm -rf *.o
	rm -rf mediaTest 
