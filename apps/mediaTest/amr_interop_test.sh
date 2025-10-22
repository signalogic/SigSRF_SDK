#!/bin/bash

# amr_interop_test.sh
#
# Copyright (c) Signalogic, Dallas, 2023-2025
#
# Objectives
#
#   -encode and decode all possible AMR NB and WB bitrates and octet-aligned and bandwidth-efficient formats
#   -decode AMR NB and WB RTP streams in generated pcaps
#   -interoperate with several "in the wild" pcaps containing AMR NB and WB RTP streams (some may include a mix of other codecs also)
#
# Notes
#
#   -mediaTest generates wav and pcap files
#   -mediaMin verifies pcap files
#   -mediaMin cmd lines use -r0.N to accelerate processing time. For detailed information on acceleration and bulk pcap handling, see https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#bulk-pcap-handling
#
# Revision History
#   Created Nov 2023
#   Revised Jun 2025, remove DISABLE_JITTER_BUFFER_OUTPUT_PCAPS flag from -dN entry. mediaMin no longer generates these by default (see cmd_line_options_flags.h) 
#   Revised Sep 2025, update cmd line spec group_pcap_nocopy to group_pcap_path_nocopy per changes in mediaMin documentation
#   Revised Sep 2025, update path var handling
#
# Usage
#
#   cd /signalogic_software_installpath/sigsrf_sdk_demo/apps/mediaTest
#   source ./amr_interop_test.sh

# mediaTest and mediaMin output and event log files
#   default commands below store mediaTest and mediaMin pcap and wav outputs and event logs to RAM disk or SSD ("/tmp/shared" in the following example, system-dependent replace as needed)
#   to store to working subfolder, assign to empty string
#   another output option is the test_files subfolder, created in SigSRF SDK Rar packages and Docker containers (set MEDIATEST_OUTPUTS to "test_files/")
MEDIATEST_OUTPUTS="/tmp/shared/"
MEDIAMIN_WAV_OUTPUTS="-g /tmp/shared"
MEDIAMIN_PCAP_OUTPUTS="--group_pcap_path_nocopy /tmp/shared"
EVENT_LOG_PATH="--event_log_path /tmp/shared"

# generate wav and/or pcap files with all possible AMR NB and WB bitrates

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_23850_16kHz_mime.pcap -Csession_config/amrwb_codec_test_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_23850_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_23850bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_23850_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_23850bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_23050_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_23050bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_23050_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_23050bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_19850_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_19850bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_19850_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_19850bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_18250_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_18250bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_18250_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_18250bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_15850_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_15850bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_15850_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_15850bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_14250_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_14250bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_14250_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_14250bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_12650_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_12650bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_12650_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_12650bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_8850_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_8850bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_8850_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_8850bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_6600_bw_16kHz_mime.pcap -Csession_config/amr_16kHz_6600bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_amr_6600_oa_16kHz_mime.pcap -Csession_config/amr_16kHz_6600bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_12200_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_12200bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_12200_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_12200bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_10200_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_10200bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_10200_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_10200bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_7950_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_7950bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_7950_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_7950bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_7400_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_7400bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_7400_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_7400bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_6700_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_6700bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_6700_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_6700bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_5900_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_5900bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_5900_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_5900bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_5150_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_5150bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_5150_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_5150bps_octet_align_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_4750_bw_8kHz_mime.pcap -Csession_config/amr_8kHz_4750bps_bandwidth_efficient_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv8c.INP -o${MEDIATEST_OUTPUTS}stv8c_amr_4750_oa_8kHz_mime.pcap -Csession_config/amr_8kHz_4750bps_octet_align_config ${EVENT_LOG_PATH}

# run mediaMin on pcap files generated by mediaTest

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_23850_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_23850_bw_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS}  ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_23850_oa_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_23050_bw_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_23050_oa_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_19850_bw_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_19850_oa_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_18250_bw_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_18250_oa_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_15850_bw_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_15850_oa_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_14250_bw_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_14250_oa_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_12650_bw_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_12650_oa_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_8850_bw_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_8850_oa_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_6600_bw_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_amr_6600_oa_16kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_12200_bw_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_12200_oa_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_10200_bw_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_10200_oa_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_7950_bw_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_7950_oa_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_7400_bw_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_7400_oa_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_6700_bw_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_6700_oa_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_5900_bw_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_5900_oa_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_5150_bw_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_5150_oa_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_4750_bw_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv8c_amr_4750_oa_8kHz_mime.pcap -L -d0x00000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# run mediaMin on various test cases

# AMR-NB 12200 bandwidth-efficient md5 sum ending in 757969
mediaMin -c x86 -i pcaps/announcementplayout_metronometones1sec_2xAMR.pcapng -L -d 0x580000000040811 -r0.5 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 12200 octet-algined md5sum ending in ba8624
mediaMin -cx86 -i pcaps/AMR_MusixFile.pcap -L -d0x00040c11 -r0.5 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -C session_config/merge_testing_config_amrwb -i pcaps/AMRWB.pcap -i pcaps/pcmutest.pcap -L -d0x00040800 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR 23850 octet-aligned md5 sum ending in d689c8
mediaMin -cx86 -i pcaps/AMRWB.pcap -i pcaps/pcmutest.pcap -L -d0x00040c10 -r0.9 -C session_config/merge_testing_config_amrwb --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 23850 octet-aligned md5 sum ending in cd0e3d
mediaMin -cx86 -i pcaps/AMRWB.pcap -L -d0x00040c11 -r0.5 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -C session_config/amrwb_packet_test_config_AMRWB-23.85kbps-20ms_bw -i pcaps/AMRWB-23.85kbps-20ms_bw.pcap -r0 -L -d0x00040800 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 23850 bps bandwidth efficient with ALLOW_OUTOFSPEC_RTP_PADDING flag set (pcap has extra 4 bytes at end of each RTP payload but its RTP headers do not specify RTP padding (maybe a leftover FCS, or frame check sequence ?)
mediaMin -cx86 -i pcaps/AMRWB-23.85kbps-20ms_bw.pcap -r0 -L -d0x20000000040801 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i pcaps/AMRWB-23.85kbps-20ms_bw.pcap -r0.9 -L -d0x20000000040c01 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 23850 bandwidth-efficient md5 sum ending in 5365b0
mediaMin -cx86 -i pcaps/AMRWB-23.85kbps-20ms_bw.pcap -r0.7 -L -d0x5a0000000040811 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
 
mediaMin -cx86 -C session_config/merge_testing_config_amr -i pcaps/AMR_MusixFile.pcap -i pcaps/PCMU.pcap -L -r0.5 -d0x00000c11 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 23850 bandwidth-efficient
mediaMin -cx86 -C session_config/amrwb_packet_test_config_AMRWB_SID -i pcaps/AMRWB_SID.pcap -r0.5 -L -d0x00040800 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 12650 md5 sum ending in b92a6a
mediaMin -cx86 -i pcaps/mediaplayout_music_1malespeaker_5xAMRWB_notimestamps.pcapng -L -d0x000c0c01 -r0.9 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-NB 12200 bandwidth-efficient (21 streams) md5 sum ending in 846537
mediaMin -cx86 -i test_files/crash1.pcap -L -d0x580000000040811 -r0.5 -l2 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-NB 12200 bandwidth-efficient md5 sum ending in 6080a9
mediaMin -cx86 -i test_files/tmpwpP7am.pcap -L -d0x580000000040811 -r0.5 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 23850 octet-aligned only 1 packet
mediaMin -cx86 -i test_files/amr-bw-efficient.pcap -L -d0x20010000c11 -r20 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 23850 octet-aligned md5 sum ending in ebc64b
mediaMin -c x86 -i test_files/codecs3-amr-wb.pcap -L -d 0x20010040c11 -r0.5 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i test_files/codecs-amr-12.pcap -L -d0x20010000c11 -r0.5 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-NB 4750 bandwidth-efficient md5 sum ending in caa0d4
mediaMin -cx86 -i test_files/81786.4289256.478164.pcap -L -d0x580000000040811 -r0.5 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-NB 5900 bandwidth-efficient md5 sum ending in 550837
mediaMin -cx86 -i test_files/85236.4284266.158664.pcap -L -d0x580000000040811 -r0.5 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 6600 bandwidth-efficient md5 sum ending in bb91a0
mediaMin -cx86 -i test_files/65446.4425483.49980.pcap -L -d0x580000000040811 -r0.5 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# AMR-WB 6600 bandwidth-efficient md5 sum ending in 4d636a
mediaMin -cx86 -i test_files/6936.3576684.1144122.pcap -L -d0x580000000040811 -r0.5 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
