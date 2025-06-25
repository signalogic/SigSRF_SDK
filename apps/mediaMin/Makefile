# $Header: /install_path/apps/Signalogic/mediaTest/mediaMin/Makefile
# 
# Copyright (C) Signalogic Inc. 2018-2025
#
# Description: Makefile for mediaMin reference application
#
# mediaMin purposes
#  -reference program for SBC, transcoding, lawful intercept, and packet streaming
#  -source code showing use of DSCreateSession(), DSPushPackets(), DSPullPackets(), and other top level APIs
#  -options for functional test, high capacity and stress test
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
#  Modified Apr 2022 JHB, link libm (required by -ffast-math compiler flag used in codec builds)
#  Modified Apr 2022 JHB, add system-dependent linking for codecs built with --fast-math
#  Modified Sep 2022 JHB, modify USE_4p6_CODEC_LIBS var for Debian
#  Modified Sep 2022 JHB, include lpthread in 'linker' target ("In the past the linker considered symbols in dependencies ... to be available. But that changed in some later version and now the linker enforces a more strict view of what is available" https://stackoverflow.com/questions/19901934/libpthread-so-0-error-adding-symbols-dso-missing-from-command-line)
#  Modified Oct 2022 JHB, add codec_libs_version=N.nn cmd line option to force build with specific codec libs. For EVS_INSTALLED var, change logic to use * in wildcard() to look for different lib versions. Add logic to check for available codec libs and handle case where a selected version is not available
#  Modified Jan 2023 JHB, change -DMEDIAMIN to -D_MEDIAMIN_, make source code searching for mediaMin and mediaTest build references easier
#  Modified Feb 2023 JHB, modify codec lib build logic to look at gcc and glibc versions and decide between v4.x, v9.4, and v11.3 (the latter using vectorized math functions, giving fastest performance. See additional comments below). Include GLIBC v2.35 libmvec lib files as needed
#  Modified Mar 2023 JHB, fix -flto issue with ldd versions 2.23-2.24. See comments near GLIBC_VERSION
#  Modified Feb 2024 JHB, rename CC_FLAGS to CPPFLAGS and AR_FLAGS to LDFLAGS
#  Modified Apr 2024 JHB, set INSTALLATH to $(shell pwd); this avoids relative paths in -rpath linker options
#  Modified May 2024 JHB, add -rdynamic linker flag to include more symbols in executable output. This helps when backtrace_symbols() is used (which it is now by DSGetBacktrace() in diaglib)
#  Modified Jun 2024 JHB, add crc32 object
#  Modified Sep 2024 JHB, add -g to end of LDFLAGS, fixes issue where v7.4 gdb can't read symbol info
#  Modified Nov 2024 JHB, add -Wextra -Wno-missing-field-initializers -Werror to CPPFLAGS to enable extra warnings *except* for missing struct initializers, which is a hopeless quest for perfection even when a struct is declared with = { 0 } so it does get fully initialized to zero (https://stackoverflow.com/questions/68555447/g11-getting-rid-of-missing-initializer-for-member-aanonymous, https://stackoverflow.com/questions/7724939/struct-initialization-fails-with-gcc-4)
#  Modified Jan 2025 JHB, add -Wno-error=implicit-fallthrough to CPPFLAGS for gcc 7.x and higher
#  Modified Feb 2025 JHB, comments only
#  Modified Apr 2025 JHB, add exception for gcc 9.3.1, the -Wno-error=implicit-fallthrough flag is not supported
#  Modified Jun 2025 JHB, add stats.cpp and port_io.cpp to cpp_objects target

# check make cmd line for "no_codecs" option

ifeq ($(no_codecs),1)
  no_evs = 1
  no_melp = 1
  no_g729 = 1
  no_g726 = 1
  no_amrnb = 1
  no_amrwb = 1
  no_amrwbplus = 1
endif

# set install path var, from mediaMin folder SigSRF software install path is 5 levels up (without symlinks)
INSTALLPATH:=$(shell pwd)/../../../../..

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
CXX = /usr/bin/g++
ifneq ($(wildcard $(CXX)),$(CXX))
  echo Make error message:  g++ compiler path $(CXX) not found
endif

# includes
INCLUDES = -I../ -I$(INSTALLPATH)/DirectCore/include -I$(INSTALLPATH) -I$(INSTALLPATH)/shared_include -I$(INSTALLPATH)/DirectCore/apps/common

# compile flags
CPPFLAGS += $(INCLUDES) -Wall -Wextra -Wno-missing-field-initializers -Werror -g3 -O3 -pthread -std=c++0x
# if using gdb
#CPPFLAGS += $(INCLUDES) -Wall -gdwarf-2 -pthread -std=c++0x

# get gcc major version
GCC_VERSION := $(shell gcc -dumpversion | awk -F. '{print $$1}')
GCC_VERSION_FULL := $(shell gcc -dumpfullversion 2> /dev/null | awk '{print $$1}')
ifeq ($(GCC_VERSION_FULL),)  # dumpfullversion can fail on older compilers
  GCC_VERSION_FULL := $(shell gcc -dumpversion | awk '{print $$1}')
endif
ifeq (,$(findstring clean,$(MAKECMDGOALS)))  # print only when target is not clean:
  $(info GCC_VERSION=$(GCC_VERSION) major version found in Makefile)
  $(info GCC_VERSION_FULL=$(GCC_VERSION_FULL) full version found in Makefile)
endif

# for gcc 7 and higher degrade switch-statement fall-through errors to warnings
ifeq ($(shell expr $(GCC_VERSION) \> 6), 1)
  # for whatever reason, gcc 9.3.1 is an exception and can't handle this flag, JHB Apr 2025
  ifneq ($(GCC_VERSION_FULL), 9.3.1)
    CPPFLAGS += -Wno-error=implicit-fallthrough
  endif
endif

# get ldd version (should match GLIBC version)
GLIBC_VERSION := $(shell ldd --version | head -1 | awk '{print $$NF}')

# apply flto exception if needed. LTO is not included for ldd versions 2.23 and 2.24 due to a link error similar to "hidden symbol `pm_run' in /tmp/xxx.ltrans4.ltrans.o is referenced by DSO". ldd versions lower and higher do not have this issue, JHB Mar 2023

ifeq ($(shell awk -v a="$(GLIBC_VERSION)" -v b="2.23" 'BEGIN{print(a==b)}'), 1)
else ifeq ($(shell awk -v a="$(GLIBC_VERSION)" -v b="2.24" 'BEGIN{print(a==b)}'), 1)
else ifneq ($(no_flto),1) # check for command line option; e.g. make all no_flto=1
  # add -flto to compile flags
  CPPFLAGS += -flto
endif

# add necessary defines
#   _LINUX_     if not set, then a Win32 system is assumed
#   _USE_PCIE_  applies to coCPU option.  If not set than ATCA or other hardware is assumed
#   _SIGRT      indicates SigSRF builds with real-time options
#   _X86        indicates host platform is x86
#   _MEDIAMIN_  indicates the build is a minimum API interface using packet flow and media processing thread provided by pktlib
CPPFLAGS += -D_FILE_OFFSET_BITS=64 -D_LINUX_ -D_USE_PCIE_ -D_SIGRT -D_X86 -D_MEDIAMIN_

ifeq ($(ALSA_INSTALLED),y)
  CPPFLAGS += -D_ALSA_INSTALLED_
endif

ifeq ($(MELPE_INSTALLED),y)
  CPPFLAGS += -D_MELPE_INSTALLED_
endif

ifeq ($(G729AB_INSTALLED),y)
  CPPFLAGS += -D_G729AB_INSTALLED_
endif

ifeq ($(G726_INSTALLED),y)
  CPPFLAGS += -D_G726_INSTALLED_
endif

ifeq ($(AMR_INSTALLED),y)
  CPPFLAGS += -D_AMR_INSTALLED_
endif

ifeq ($(AMRWB_INSTALLED),y)
  CPPFLAGS += -D_AMRWB_INSTALLED_
endif

ifeq ($(AMRWBPLUS_INSTALLED),y)
  CPPFLAGS += -D_AMRWBPLUS_INSTALLED_
endif

ifeq ($(KALDIASR_INSTALLED),y)
  CPPFLAGS += -D_KALDIASR_INSTALLED_
endif

# linker search paths
LINKER_INCLUDES = -L$(INSTALLPATH)/SIG_LIBS/glibc -L/usr/lib

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
else ifeq ($(codec_libs_version),9.4)
  USE_4p6_CODEC_LIBS :=
  USE_9p4_CODEC_LIBS := true
  USE_11p3_CODEC_LIBS :=
else ifeq ($(codec_libs_version),11.3)
  USE_4p6_CODEC_LIBS :=
  USE_11p3_CODEC_LIBS := true
endif

ifeq ($(EVS_INSTALLED),y)

# determine versions of lib files available (user might delete one or the other from the base install, yes this has happened)

  EVSPATH:=/usr/lib/libevscom_sig.x86_gcc4.6.so
  ifeq ($(wildcard $(EVSPATH)),$(EVSPATH))
    4p6_EVS_LIBS_FOUND := true
  endif
  EVSPATH:=/usr/lib/libevscom_sig.x86_gcc9.4.so
  ifeq ($(wildcard $(EVSPATH)),$(EVSPATH))
    9p4_EVS_LIBS_FOUND := true
  endif
  EVSPATH:=/usr/lib/libevscom_sig.x86_gcc11.3.so
  ifeq ($(wildcard $(EVSPATH)),$(EVSPATH))
    11p3_EVS_LIBS_FOUND := true
  endif

# decide which of available versions should be linked in the build

  ifeq ($(USE_4p6_CODEC_LIBS),true)
    ifneq ($(4p6_EVS_LIBS_FOUND),true)
      ifeq (,$(findstring clean,$(MAKECMDGOALS)))  # print only when target is not clean:
        $(info gcc 4.6 EVS codec libs not found)
      endif
      USE_4p6_CODEC_LIBS :=
    endif
  else ifeq ($(USE_9p4_CODEC_LIBS),true)
    ifneq ($(9p4_EVS_LIBS_FOUND),true)
      ifeq (,$(findstring clean,$(MAKECMDGOALS)))
        $(info gcc 9.4 EVS codec libs not found)
      endif
      USE_9p4_CODEC_LIBS :=
    endif
  else
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
    # add if linking gcc 11.3 libs with AVX on older system
    # GLIB_2-35_LIBMVEC = -l:libmvec.so.1

  else ifeq ($(USE_11p3_CODEC_LIBS),true)

# note - currently gcc v5 through 10 are using v9.4 libs and v11 and higher are using v11.3 libs. This is approximate; more gcc specific lib versions may be needed. While all codec libs are typically built with --ffast-math and -O3, vll.3 uses vectorized math functions and produces substantially faster code. If you hear glitches or pops in decoded output there may be a link mismatch between 11.3 libs and your application gcc version. In that case you can (i) modify the Makefile so CODEC_LIBS includes xxx_sig.x86_gcc4.6 (old and slow but never fails to link) (ii) force an available lib version to be used (add codec_libs_version=N.n after Make all, where N.n can be 4.6, 9.4, or 11.3) or (iii) contact Signalogic for a specific lib version

    ifeq ($(codec_libs_version),11.3)  # command line forces v11.3
      $(info codec_libs_version=11.3 on cmd line, building with gcc 11.3 EVS codec libs)  # print build info if cmd line option was given

      CODEC_LIBS += -levsenc_sig.x86_gcc11.3
      CODEC_LIBS += -levsdec_sig.x86_gcc11.3
      CODEC_LIBS += -levscom_sig.x86_gcc11.3
      ifeq ($(shell awk -v a="$(GLIBC_VERSION)" -v b="2.35" 'BEGIN{print(a<b)}'), 1)
        GLIB_2-35_LIBMVEC = -l:libmvec.so.1
      endif

    else ifeq ($(shell expr $(GCC_VERSION) \< 11), 1)  # v9.4 is known to be incompatible with v11.3
      CODEC_LIBS += -levsenc_sig.x86_gcc9.4
      CODEC_LIBS += -levsdec_sig.x86_gcc9.4
      CODEC_LIBS += -levscom_sig.x86_gcc9.4
    else
      CODEC_LIBS += -levsenc_sig.x86_gcc11.3
      CODEC_LIBS += -levsdec_sig.x86_gcc11.3
      CODEC_LIBS += -levscom_sig.x86_gcc11.3
      ifeq ($(shell awk -v a="$(GLIBC_VERSION)" -v b="2.35" 'BEGIN{print(a<b)}'), 1)
        GLIB_2-35_LIBMVEC = -l:libmvec.so.1
      endif
    endif
  else  # currently this won't work as the install .rar and Docker containers do not have generic libs
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

# essential libs for user apps (pktlib = packet handling, voplib = voice-over-packet (including codec interface), CODEC_LIBS (variable, depending on which codecs are installed), diaglib = diagnostics, hwlib = DirectCore, streamlib = stream groups and merging. Kaldi, ATLAS, and BLAS are handled with -Wl,-rpath
SIG_LIBS = -lvoplib $(CODEC_LIBS) -lpktlib -ldiaglib -lhwlib -lalglib -lstreamlib -lderlib

# add aviolib lib if ALSA present on the system (USB audio I/O support)
ifeq ($(ALSA_INSTALLED),y)
  SIG_LIBS += -laviolib
endif

# link flags
LDFLAGS = -flto -Xlinker -Map=mediaMin.map -Wl,--disable-new-dtags -rdynamic

# if Kaldi ASR present on the system, link inferlib, Kaldi libs, and ATLAS and BLAS linear algebra libs. Notes:
# -if not found stublib will fill in inferlib API stubs (which won't be called)
# -inferlib calls kaldi libs which call ATLAS and BLAS libs

ifeq ($(KALDIASR_INSTALLED),y)

  SIG_LIBS += -linferlib
  LDFLAGS += -Wl,-rpath=$(KALDIASRPATH)

  # ATLAS and BLAS: try SigSRF install first
  ATLASPATH=$(INSTALLPATH)/blas_atlas/lib

  ifeq ($(wildcard $(ATLASPATH)),$(ATLASPATH))
    LDFLAGS += -Wl,-rpath=$(ATLASPATH)
  else

    # try system folders (i.e. full download/install of ATLAS and BLAS)
    ATLASPATH=/usr/lib/atlas-base
    ifeq ($(wildcard $(ATLASPATH)),$(ATLASPATH))
      LDFLAGS += -Wl,-rpath=$(ATLASPATH)
    endif
  endif

  # OpenFST: try SigSRF install first
  OPENFSTPATH=$(INSTALLPATH)/openfst/lib

  ifeq ($(wildcard $(OPENFSTPATH)),$(OPENFSTPATH))
    LDFLAGS += -Wl,-rpath=$(OPENFSTPATH)
  else

    # try system folders (i.e. full download/install of OpenFST)
    OPENFSTPATH=/usr/lib/openfst
    ifeq ($(wildcard $(OPENFSTPATH)),$(OPENFSTPATH))
      LDFLAGS += -Wl,-rpath=$(OPENFSTPATH)
    endif
  endif

  # gfortran: try SigSRF install first
  GFORTRANPATH=$(INSTALLPATH)/gfortran/lib

  ifeq ($(wildcard $(GFORTRANPATH)),$(GFORTRANPATH))
    LDFLAGS += -Wl,-rpath=$(GFORTRANPATH)
  else

    # try system folders (i.e. full download/install of gfortran)
    GFORTRANPATH=/usr/lib/gfortran
    ifeq ($(wildcard $(GFORTRANPATH)),$(GFORTRANPATH))
      LDFLAGS += -Wl,-rpath=$(GFORTRANPATH)
    endif
  endif

endif

# add $(INSTALLPATH)/SIG_LIBS/glibc to run-time executable path. Notes:
#  -this is part of high performance codec lib handling
#  -glibc contains glibc 2.35 versions of libmvec.so.1 and libm.so.6, which is specified with -l: in link target if host system GLIBC version is lower than 2.35

LDFLAGS += -Wl,-rpath=$(INSTALLPATH)/SIG_LIBS/glibc

# turn on verbose link if in command line (e.g. Make all verbose_link=1)
ifeq ($(verbose_link),1)
  LDFLAGS += -Wl,--verbose
endif

# without this on systems with glibc 2.31 or higher glibc functions that occur after -ldl may fail to link, JHB Feb 2024
LDFLAGS += -Wl,--no-as-needed

# without -g at tail end of linker flags, some versions of gdb can't read symbol info
LDFLAGS += -g

cpp_common_objects = timer.o getUserInterface.o cmdLineOpt.o
c_common_objects = keybd.o
cpp_sdp_objects = types.o sdp.o utils.o reader.o writer.o
c_crc_objects = crc32.o
c_mediaTest_objects = transcoder_control.o cmd_line_interface.o
cpp_objects = sdp_app.o session_app.o user_io.o stats.o port_io.o mediaMin.o
# add sources as needed for user defined processing. For example, adding audio_domain_processing.c will take precedence over the default version included in streamlib.so
# c_objects += audio_domain_processing.o
# c_objects += packet_media_flow_proc.o

# build targets
all: $(cpp_common_objects) $(cpp_sdp_objects) $(c_crc_objects) $(c_common_objects) $(c_mediaTest_objects) $(cpp_objects) $(c_objects) link	

$(cpp_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@ 

$(cpp_sdp_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/sdp/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@ 

$(c_crc_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/crc/%.c
	$(CXX) $(CPPFLAGS) -c $< -o $@ 

$(c_common_objects): %.o: $(INSTALLPATH)/DirectCore/apps/common/%.c
	$(CXX) $(CPPFLAGS) -c $< -o $@ 

$(c_mediaTest_objects): %.o: ../%.c
	$(CXX) $(CPPFLAGS) -c $< -o $@ 

$(cpp_objects): %.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@ 

$(c_objects): %.o: %.c
	$(CXX) $(CPPFLAGS) -c $< -o $@ 

# link object files, essential libs, and cimlib which provides cmd line handling. stublib is last in link order and fills in top-level APIs for any modules not available (their XX_INSTALLED flags not set) due to version, demo, license etc (examples would be some not-always-installed codecs)
link:
	$(CXX) $(LDFLAGS) $(cpp_common_objects) $(cpp_sdp_objects) $(c_crc_objects) $(c_common_objects) $(c_mediaTest_objects) $(cpp_objects) $(c_objects) $(LINKER_INCLUDES) -o ./mediaMin -lstdc++ $(SIG_LIBS) -lcimlib -ldl -lpthread $(GLIB_2-35_LIBMVEC) -lm -lstublib

.PHONY: clean all
clean:
	rm -rf *.o
	rm -rf mediaMin
