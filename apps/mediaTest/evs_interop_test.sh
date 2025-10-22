#!/bin/bash

# evs_interop_test.sh
#
# Copyright (c) Signalogic, Dallas, 2023-2025
#
# Objectives
#
#   -test encoding and decoding of combinations of EVS primary and AMR-WB IO mode bitrates, VBR mode, header-full and compact header, DTX and RF enable/disable
#   -decode EVS RTP streams in generated pcaps
#   -interoperate with several "in the wild" pcaps containing EVS RTP streams (some may include a mix of other codecs also)
#
# Notes
#
#   -mediaTest generates wav and pcap files
#   -mediaMin verifies pcap files
#   -mediaMin cmd lines use -r0.9 to accelerate processing time. For detailed information on acceleration and bulk pcap handling, see https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#bulk-pcap-handling
#
# Revision History
#   Created Nov 2023
#   Revised Jun 2025, remove DISABLE_JITTER_BUFFER_OUTPUT_PCAPS flag from -dN entry. mediaMin no longer generates these by default (see cmd_line_options_flags.h) 
#   Revised Jul 2025, remove ENABLE_DORMANT_SESSIONS flag in -dN entry for evs_mixed_mode_mixed_rate.pcap (after flag definition change in cmd_line_options_flags.h)
#   Revised Sep 2025, update cmd line spec group_pcap_nocopy to group_pcap_path_nocopy per changes in mediaMin documentation
#   Revised Sep 2025, update path var handling
#
# Usage
#
#   cd /signalogic_software_installpath/sigsrf_sdk_demo/apps/mediaTest
#   source ./evs_interop_test.sh

# mediaTest and mediaMin output and event log files
#   default commands below store mediaTest and mediaMin pcap and wav outputs and event logs to RAM disk or SSD ("/tmp/shared" in the following example, system-dependent replace as needed)
#   to store to working subfolder, assign to empty string
#   another output option is the test_files subfolder, created in SigSRF SDK Rar packages and Docker containers (set MEDIATEST_OUTPUTS to "test_files/")
MEDIATEST_OUTPUTS="/tmp/shared/"
MEDIAMIN_WAV_OUTPUTS="-g /tmp/shared"
MEDIAMIN_PCAP_OUTPUTS="--group_pcap_path_nocopy /tmp/shared"
EVENT_LOG_PATH="--event_log_path /tmp/shared"

# AMR-WB IO mode tests
#
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_15850_full_header.wav -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_15850_full_header_dtx_disabled.wav -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_15850_full_header_dtx_disabled.pcap -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_15850_full_header.pcap -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_15850_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_15850_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_12650_full_header.wav -Csession_config/evs_16kHz_12650bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_12650_full_header.pcap -Csession_config/evs_16kHz_12650bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_12650_compact_header.wav -Csession_config/evs_16kHz_12650bps_amrwb_io_compact_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_12650_compact_header.pcap -Csession_config/evs_16kHz_12650bps_amrwb_io_compact_header_config ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_12650_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_12650_compact_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23850_full_header.wav -Csession_config/evs_16kHz_23850bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23850_full_header.pcap -Csession_config/evs_16kHz_23850bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23850_compact_header.wav -Csession_config/evs_16kHz_23850bps_amrwb_io_compact_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23850_compact_header.pcap -Csession_config/evs_16kHz_23850bps_amrwb_io_compact_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23050_full_header.pcap -Csession_config/evs_16kHz_23050bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23050_compact_header.pcap -Csession_config/evs_16kHz_23050bps_amrwb_io_compact_header_config ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23850_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23850_compact_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23050_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_23050_compact_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_8850_full_header.wav -Csession_config/evs_16kHz_8850bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_8850_full_header.pcap -Csession_config/evs_16kHz_8850bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_8850_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_6600_full_header.wav -Csession_config/evs_16kHz_6600bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_6600_full_header.pcap -Csession_config/evs_16kHz_6600bps_amrwb_io_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_6600_compact_header.wav -Csession_config/evs_16kHz_6600bps_amrwb_io_compact_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_6600_compact_header.pcap -Csession_config/evs_16kHz_6600bps_amrwb_io_compact_header_config ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_6600_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_6600_compact_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# mixed modes and mixed rates, including AMR-WB IO mode with bit shifted payloads
#
mediaMin -cx86 -ipcaps/evs_mixed_mode_mixed_rate.pcap -L -d0x000c0c01 -r0.9 -Csession_config/EVS_AMR-WB_IO_mode_payload_shift --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}  # md5 sum ending in a285d7 with -r20

# VBR 5900 bps mode tests
#
mediaTest -cx86 -itest_files/stv16c.wav -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_full_header.wav -Csession_config/evs_16kHz_5900bps_full_header_config ${EVENT_LOG_PATH}  
mediaTest -cx86 -itest_files/stv16c.wav -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_compact_header.wav -Csession_config/evs_16kHz_5900bps_compact_header_config ${EVENT_LOG_PATH}  
mediaTest -cx86 -itest_files/stv16c.wav -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_full_header_dtx_disabled.wav -Csession_config/evs_16kHz_5900bps_full_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.wav -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_compact_header_dtx_disabled.wav -Csession_config/evs_16kHz_5900bps_compact_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.wav -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_full_header.pcap -Csession_config/evs_16kHz_5900bps_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.wav -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_compact_header.pcap -Csession_config/evs_16kHz_5900bps_compact_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.wav -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_full_header_dtx_disabled.pcap -Csession_config/evs_16kHz_5900bps_full_header_dtx_disabled_config
mediaTest -cx86 -itest_files/stv16c.wav -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_compact_header_dtx_disabled.pcap -Csession_config/evs_16kHz_5900bps_compact_header_dtx_disabled_config
mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o${MEDIATEST_OUTPUTS}AAdefaultBusinessHoursGreeting_16kHz_5900_full_header.wav -Csession_config/evs_16kHz_5900bps_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o${MEDIATEST_OUTPUTS}AAdefaultBusinessHoursGreeting_16kHz_5900_compact_header.wav -Csession_config/evs_16kHz_5900bps_compact_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o${MEDIATEST_OUTPUTS}AAdefaultBusinessHoursGreeting_8kHz_5900_full_header.wav -Csession_config/evs_8kHz_input_8kHz_5900bps_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o${MEDIATEST_OUTPUTS}AAdefaultBusinessHoursGreeting_8kHz_5900_compact_header.wav -Csession_config/evs_8kHz_input_8kHz_5900bps_compact_header_config ${EVENT_LOG_PATH}

mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stvc16_48kHz_5900.wav -Csession_config/evs_48kHz_input_16kHz_5900bps_full_band_config
mediaTest -cx86 -itest_files/T_mode.wav -o${MEDIATEST_OUTPUTS}T_mode_48kHz_5900.wav -Csession_config/evs_48kHz_input_16kHz_5900bps_full_band_config

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_5900_compact_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# 24400 and 13200, mix of CMR/no CMR, mix of ptimes per payload, variable ptime, RF enable
#
mediaMin -c x86 -i test_files/evs_float_b24_4m_wb_cbr_hfOnly0_cmr0_ptime20.pcap -L -d 0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -c x86 -i test_files/evs_float_b24_4m_dtx_swb_cbr_hfOnly1_cmr1_ptime20.pcap -L -d 0x00000c11 -r0.9 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}  # md5 sum ending in 8f380c
mediaMin -c x86 -i test_files/evs_float_b24_4m_dtx_swb_cbr_hfOnly0_cmr1_ptime60.pcap -L -d 0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -c x86 -i test_files/evs_float_b13_2m_dtx_swb_cbr_rf3hi_hfOnly0_cmr0_ptime20.pcap -L -d 0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -c x86 -i test_files/evs_float_b13_2m_dtx_swb_cbr_rf3hi_hfOnly0_cmr1_ptime20.pcap -L -d 0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# VBR 5900 single channel, hf0 has collision avoidance padding, hf1 is hf-only format
#
mediaMin -cx86 -ipcaps/evs_5900_1_hf0.rtpdump -L -d0x00000c11 -r0.9 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}  # md5 sum ending in 59392d
mediaMin -cx86 -ipcaps/evs_5900_1_hf1.rtpdump -L -d0x00000c11 -r0.9 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
#
# VBR 5900 single channel, some payloads carry 2x frames, hf0 has collision avoidance padding, hf1 is hf-only format
#
mediaMin -cx86 -ipcaps/evs_5900_2_hf0.rtpdump -L -d0x00000c11 -r0.9 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}  # md5 sum ending in 8febe7
mediaMin -cx86 -ipcaps/evs_5900_2_hf1.rtpdump -L -d0x00000c11 -r0.9 --md5sum ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# 16400 bps tests
#
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_16400_full_header.pcap -Csession_config/evs_16kHz_16400bps_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_16400_compact_header.pcap -Csession_config/evs_16kHz_16400bps_compact_header_config ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_16400_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_16400_compact_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# 24400 bps tests
#
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_24400_full_header.pcap -Csession_config/evs_16kHz_24400bps_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_24400_compact_header.pcap -Csession_config/evs_16kHz_24400bps_compact_header_config ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_24400_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_24400_compact_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# 13200 bps tests, including RF enable mode tests
#
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_13200_full_header.wav -Csession_config/evs_16kHz_13200bps_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_13200_compact_header.wav -Csession_config/evs_16kHz_13200bps_compact_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_13200_full_header.pcap -Csession_config/evs_16kHz_13200bps_full_header_config ${EVENT_LOG_PATH}
mediaTest -cx86 -itest_files/stv16c.INP -o${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_13200_compact_header.pcap -Csession_config/evs_16kHz_13200bps_compact_header_config ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_13200_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_13200_compact_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_13200_full_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
mediaMin -cx86 -i${MEDIATEST_OUTPUTS}stv16c_evs_16kHz_13200_compact_header.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}

# 17 min audio pcap with 2x EVS RTP streams out of alignment (also includes one AMR-WB stream)
#
mediaMin -cx86 -ipcaps/evs_long_rate_alignment.pcap -L -d0x00000c11 -r0.9 ${MEDIAMIN_WAV_OUTPUTS} ${MEDIAMIN_PCAP_OUTPUTS} ${EVENT_LOG_PATH}
