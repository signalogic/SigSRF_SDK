# $Header: /install_path/apps/Signalogic/mediaTest/mediaMin/Makefile
# 
# Description: Makefile for mediaMin reference application
#
# mediaMin purpose
#  -reference program for SBC, transcoding, lawful intercept, and packet streaming
#  -source code showing use of DSCreateSession(), DSPushPackets(), DSPullPackets(), and other top level APIs
#  -options for functional test, high capacity and stress test
# 
# Copyright (C) Signalogic Inc. 2018-2021
#
# Revision History
#
#  Created Aug 2018 JHB
#  Modified Sep 2018 JHB, add -Wall to gcc flags
#  Modified Oct 2018 JHB, add install checks for AMR-NB, AMR-WB, and AMR-WB+ codecs
#  Modified Nov 2018 JHB, add streamlib
#  Modified Dec 2018 JHB, remove unused AMR-WB+ common lib
#  Modified Mar 2019 JHB, add cmd line options to configure ALSA and codec libs linked into customer demo executables.  This is for cases where the customer does not have gcc tools (or does but doesn't want to build) and they will run executables built by Signalogic
#  Modified Aug 2019 JHB, add -flto (link-time optimization)
#  Modified Nov 2019 JHB, add check for Kaldi ASR installed
#  Modified Feb 2020 JHB, show how to add sources to take precedence over what's included in shared objects
#  Modified Mar 2020 JHB, handle name change of mediaThread_test_app.c to mediaMin.c
#  Modified Jan 2021 JHB, change ALSA detection to look for /usr/include/alsa folder instead of /proc/asound. ALSA might be installed but not the dev version of ALSA, which has the header files
#  Modified Jan 2021 JHB, add INSTALLPATH, add rpath linker options for version of Kaldi libs used by SigSRF and for ATLAS linear algebra / matrix lib (only if makefile detects Kaldi is installed)
#  Modified Jan 2021 JHB, break out link flags from CC_FLAGS into AR_FLAGS, add -rpath linker options
#  Modified Jan 2021 JHB, add cpp_objects target
#  Modified Mar 2021 JHB, add derlib (DER decoding lib) to SIG_LIBS var
#  Modified Apr 2021 JHB, add sdp_app.o session_app.o and user_io.o to cpp_objects target

# check make cmd line for "no_codecs" option

ifeq ($(no_codecs),1)
   no_melp = 1
   no_g729 = 1
   no_g726 = 1
   no_amrnb = 1
   no_amrwb = 1
   no_amrwbplus = 1
endif

# set install path var, from mediaMin folder SigSRF software install path is 5 levels up
INSTALLPATH=../../../../..

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
INCLUDES =  -I../ -I$(INSTALLPATH)/DirectCore/include -I$(INSTALLPATH) -I$(INSTALLPATH)/shared_include -I$(INSTALLPATH)/DirectCore/apps/common

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

# essential libs for user apps (pktlib = packet handling, voplib = voice-over-packet (including codec interface), CODEC_LIBS (variable, depending on which codecs are installed), diaglib = diagnostics, hwlib = DirectCore, streamlib = stream groups and merging
SIG_LIBS = -lvoplib $(CODEC_LIBS) -lpktlib -ldiaglib -lhwlib -lalglib -lstreamlib -lderlib

# add aviolib lib if ALSA present on the system (USB audio I/O support)
ifeq ($(ALSA_INSTALLED),y)
  SIG_LIBS += -laviolib
endif

# link flags
AR_FLAGS = -flto -Xlinker -Map=mediaMin.map -Wl,--disable-new-dtags  # get the RUNPATH crap out of there

# if Kaldi ASR present on the system, link inferlib, Kaldi libs, and ATLAS and BLAS linear algebra libs. Notes:
# -if not found stublib will fill in inferlib API stubs (which won't be called)
# -inferlib calls kaldi libs which call ATLAS and BLAS libs

ifeq ($(KALDIASR_INSTALLED),y)

  SIG_LIBS += -linferlib
  AR_FLAGS += -Wl,-rpath=$(KALDIASRPATH)

  # ATLAS and BLAS: try SigSRF install first
  ATLASPATH=$(INSTALLPATH)/blas_atlas/lib

  ifeq ($(wildcard $(ATLASPATH)),$(ATLASPATH))
    AR_FLAGS += -Wl,-rpath=$(ATLASPATH)
  else

    # try system folders (i.e. full download/install of ATLAS and BLAS)
    ATLASPATH=/usr/lib/atlas-base
    ifeq ($(wildcard $(ATLASPATH)),$(ATLASPATH))
      AR_FLAGS += -Wl,-rpath=$(ATLASPATH)
    endif
  endif

  # OpenFST: try SigSRF install first
  OPENFSTPATH=$(INSTALLPATH)/openfst/lib

  ifeq ($(wildcard $(OPENFSTPATH)),$(OPENFSTPATH))
    AR_FLAGS += -Wl,-rpath=$(OPENFSTPATH)
  else

    # try system folders (i.e. full download/install of OpenFST)
    OPENFSTPATH=/usr/lib/openfst
    ifeq ($(wildcard $(OPENFSTPATH)),$(OPENFSTPATH))
      AR_FLAGS += -Wl,-rpath=$(OPENFSTPATH)
    endif
  endif

  # gfortran: try SigSRF install first
  GFORTRANPATH=$(INSTALLPATH)/gfortran/lib

  ifeq ($(wildcard $(GFORTRANPATH)),$(GFORTRANPATH))
    AR_FLAGS += -Wl,-rpath=$(GFORTRANPATH)
  else

    # try system folders (i.e. full download/install of gfortran)
    GFORTRANPATH=/usr/lib/gfortran
    ifeq ($(wildcard $(GFORTRANPATH)),$(GFORTRANPATH))
      AR_FLAGS += -Wl,-rpath=$(GFORTRANPATH)
    endif
  endif

endif

cpp_common_objects = timer.o getUserInterface.o cmdLineOpt.o 
c_common_objects = keybd.o
cpp_sdp_objects = types.o sdp.o utils.o reader.o writer.o 
c_mediaTest_objects = transcoder_control.o cmd_line_interface.o
cpp_objects = sdp_app.o session_app.o user_io.o mediaMin.o
# add sources as needed for user defined processing. For example, adding audio_domain_processing.c will take precedence over the default version included in streamlib.so
# c_objects += audio_domain_processing.o
# c_objects += packet_media_flow_proc.o

# build targets
all: $(cpp_common_objects) $(cpp_sdp_objects) $(c_common_objects) $(c_mediaTest_objects) $(cpp_objects) $(c_objects) linker	

$(cpp_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.cpp
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(cpp_sdp_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/sdp/%.cpp
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_mediaTest_objects): %.o: ../%.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(cpp_objects): %.o: %.cpp
	$(CC) $(CC_FLAGS) -c $< -o $@ 

$(c_objects): %.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@ 

# link object files, essential libs, and cimlib which provides cmd line handling. stublib is last in link order and fills in top-level APIs for any modules not available (their XX_INSTALLED flags not set) due to version, demo, license etc (examples would be some less often used codecs)
linker:
	$(CC) $(AR_FLAGS) $(cpp_common_objects) $(cpp_sdp_objects) $(c_common_objects) $(c_mediaTest_objects) $(cpp_objects) $(c_objects) $(LINKER_INCLUDES) -o ./mediaMin -lstdc++ $(SIG_LIBS) -ldl -lcimlib -lstublib

.PHONY: clean all
clean:
	rm -rf *.o
	rm -rf mediaMin
