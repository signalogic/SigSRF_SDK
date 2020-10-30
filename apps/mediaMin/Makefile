# $Header: /install_path/apps/Signalogic/mediaTest/mediaMin/Makefile
# 
#  Description:  Makefile for minimum user app pktlib interface
#
#  Purpose:  1) media transcoding and packet streaming / processing test program, 2) example user application showing use of pktlib DSPushPackets() and DSPullPackets() APIs
# 
#  Copyright (C) Signalogic Inc. 2018-2020
#
#  Revision History
#
#    Created Aug 2018 JHB
#    Modified Sep 2018 JHB, add -Wall to gcc flags
#    Modified Oct 2018 JHB, add install checks for AMR-NB, AMR-WB, and AMR-WB+ codecs
#    Modified Nov 2018 JHB, add streamlib
#    Modified Dec 2018 JHB, remove unused AMR-WB+ common lib
#    Modified Mar 2019 JHB, add cmd line options to configure ALSA and codec libs linked into customer demo executables.  This is for cases where the customer does not have gcc tools (or does but doesn't want to build) and they will run executables built by Signalogic
#    Modified Aug 2019 JHB, add -flto (link-time optimization)
#    Modified Nov 2019 JHB, add check for Kaldi ASR installed
#    Modified Feb 2020 JHB, show how to add sources to take precedence over what's included in shared objects
#    Modified Mar 2020 JHB, handle name change of mediaThread_test_app.c to mediaMin.c

ifeq ($(no_codecs),1)
   no_melp = 1
   no_g729 = 1
   no_g726 = 1
   no_amrnb = 1
   no_amrwb = 1
   no_amrwbplus = 1
endif

# no_xxx indicates to forcibly exclude xxx from the build.  Otherwise we auto-discover whether xxx has been installed

ifeq ($(no_alsa),1)
  ALSAPATH=xx
else
  ALSAPATH=/proc/asound
endif
ifeq ($(wildcard $(ALSAPATH)),$(ALSAPATH))
  ALSA_INSTALLED = y
else
  ALSA_INSTALLED = n
endif

ifeq ($(no_melp),1)
  MELPEPATH=xx
else
  MELPEPATH=/usr/lib/libmelpe_sig.x86.so
endif
ifeq ($(wildcard $(MELPEPATH)),$(MELPEPATH))
  MELPE_INSTALLED = y
else
  MELPE_INSTALLED = n
endif

ifeq ($(no_g729),1)
  G729ABPATH=xx
else
  G729ABPATH=/usr/lib/libg729ab_sig.x86.so
endif
ifeq ($(wildcard $(G729ABPATH)),$(G729ABPATH))
  G729AB_INSTALLED = y
else
  G729AB_INSTALLED = n
endif

ifeq ($(no_g726),1)
  G726PATH=xx
else
  G726PATH=/usr/lib/libg726_sig.x86.so
endif
ifeq ($(wildcard $(G726PATH)),$(G726PATH))
  G726_INSTALLED = y
else
  G726_INSTALLED = n
endif

ifeq ($(no_amrnb),1)
  AMRPATH=xx
else
  AMRPATH=/usr/lib/libamrdec_sig.x86.so
endif
ifeq ($(wildcard $(AMRPATH)),$(AMRPATH))
  AMR_INSTALLED = y
else
  AMR_INSTALLED = n
endif

ifeq ($(no_amrwb),1)
  AMRWBPATH=xx
else
  AMRWBPATH=/usr/lib/libamrwbdec_sig.x86.so
endif
ifeq ($(wildcard $(AMRWBPATH)),$(AMRWBPATH))
  AMRWB_INSTALLED = y
else
  AMRWB_INSTALLED = n
endif

ifeq ($(no_amrwbplus),1)
  AMRWBPLUSPATH=xx
else
  AMRWBPLUSPATH=/usr/lib/libamrwbplusdec_sig.x86.so
endif
ifeq ($(wildcard $(AMRWBPLUSPATH)),$(AMRWBPLUSPATH))
  AMRWBPLUS_INSTALLED = y
else
  AMRWBPLUS_INSTALLED = n
endif

ifeq ($(no_kaldiasr),1)
  KALDIASRPATH=xx
else
  #check for any of several libkaldi-xx.so files
  KALDIASRPATH=/usr/lib/libkaldi-base.so
endif
ifeq ($(wildcard $(KALDIASRPATH)),$(KALDIASRPATH))
  KALDIASR_INSTALLED = y
else
  KALDIASR_INSTALLED = n
endif

# compiler path, g++ is required because some source codes are .cpp
CC = /usr/bin/g++
ifneq ($(wildcard $(CC)),$(CC))
  echo Make error message:  g++ compiler path $(CC) not found
endif

# includes
INCLUDES =  -I../ -I../../../../include -I../../../../.. -I../../../../../shared_include -I../../../common

# compile flags
CC_FLAGS = $(INCLUDES)
CC_FLAGS += -Wall -g3 -O3 -flto -pthread -std=c++0x

# add necessary defines
#   _LINUX_     if not set, then a Win32 system is assumed
#   _USE_PCIE_  applies to coCPU option.  If not set than ATCA or other hardware is assumed
#   _SIGRT_     indicates to use real-time options where applicable
#   _X86        indicates host platform is x86
#   MEDIAMIN    indicates the build is a minimum API interface using packet flow and media processing thread provided by pktlib
CC_FLAGS += -D_FILE_OFFSET_BITS=64 -D_LINUX_ -D_USE_PCIE_ -D_SIGRT -D_X86 -DMEDIAMIN

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

LINKER_INCLUDES = -L../../../../lib -L/usr/lib

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

# essential libs for user apps (pktlib = packet handling, voplib = voice-over-packet (including codec interface), CODEC_LIBS (variable, depending on which codecs are installed), diaglib = diagnostics, hwlib = DirectCore, streamlib = stream groups and merging
SIG_LIBS = -lvoplib $(CODEC_LIBS) -lpktlib -ldiaglib -lhwlib -lalglib -lstreamlib

# add aviolib lib if ALSA present on the system (USB audio I/O support)
ifeq ($(ALSA_INSTALLED),y)
  SIG_LIBS += -laviolib
endif

# add inferlib lib if Kaldi ASR present on the system
ifeq ($(KALDIASR_INSTALLED),y)
  SIG_LIBS += -linferlib
endif

cpp_common_objects = timer.o getUserInterface.o cmdLineOpt.o
c_common_objects = keybd.o
c_mediaTest_objects = transcoder_control.o cmd_line_interface.o
c_objects = mediaMin.o
# add sources as needed for user defined processing. For example, adding audio_domain_processing.c will take precedence over the default version included in streamlib.so
# c_objects += audio_domain_processing.o
# c_objects += packet_media_flow_proc.o

# build targets
all: $(cpp_common_objects) $(c_common_objects) $(c_mediaTest_objects) $(c_objects) linker	

$(cpp_common_objects): %.o: ../../../common/%.cpp
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_common_objects): %.o: ../../../common/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_mediaTest_objects): %.o: ../%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

# link object files, essential libs, and cimlib which provides cmd line handling
linker:
	$(CC) $(CC_FLAGS) $(cpp_common_objects) $(c_common_objects) $(c_mediaTest_objects) $(c_objects) $(LINKER_INCLUDES) -o ./mediaMin -lstdc++ $(SIG_LIBS) -ldl -lcimlib -lstublib

.PHONY: clean all
clean:
	rm -rf *.o
	rm -rf mediaMin
