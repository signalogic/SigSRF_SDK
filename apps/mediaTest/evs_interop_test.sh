#!/bin/bash

# evs_interop_test.sh
#
# Copyright (c) Signalogic, Dallas, 2023-2024
#
# Notes
#
#   -test various EVS bitrates, header-full and compact header, DTX and RF enable/disable
#   -also test other AMR-WB IO mode and VBR mode files
#   -generates both .wav and .pcap files
#   -mediaMin cmd lines use -r0.9 to accelerate processing time. For detailed information on acceleration and bulk pcap handling, see https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#bulk-pcap-handling
#
# Usage
#
#   cd /signalogic_software_installpath/sigsrf_sdk_demo/apps/mediaTest
#   source ./evs_interop_test.sh

#mediaTest outputs: replace with output path as needed. Default is mediaTest/test_files subfolder (installed with the SigSRF SDK from Rar packages or pre-installed in Docker containers)
MT_OUTPUTS="test_files"

# mediaMin outputs: replace with system-specific locations. In the following examples "/tmp/shared" is a RAM disk location. Assign to empty string to store output files on mediaMin subfolder
MM_WAV_OUTPUTS="-g /tmp/shared"
MM_PCAP_OUTPUTS="--group_pcap_nocopy /tmp/shared"

#
# AMR-WB IO mode tests
#
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_15850_full_header.wav -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_15850_full_header_dtx_disabled.wav -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_dtx_disabled_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_15850_full_header_dtx_disabled.pcap -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_dtx_disabled_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_15850_full_header.pcap -Csession_config/evs_16kHz_15850bps_amrwb_io_full_header_config
cd mediaMin
./mediaMin -cx86 -i../test_files/stv16c_16kHz_15850_full_header.pcap -L -d0x08000c11 -r0.9 "$MM_WAV_OUTPUTS" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
cd mediaMin
./mediaMin -cx86 -i../test_files/stv16c_16kHz_15850_full_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
cd ..
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_12650_full_header.wav -Csession_config/evs_16kHz_12650bps_amrwb_io_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_12650_full_header.pcap -Csession_config/evs_16kHz_12650bps_amrwb_io_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_12650_compact_header.wav -Csession_config/evs_16kHz_12650bps_amrwb_io_compact_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_12650_compact_header.pcap -Csession_config/evs_16kHz_12650bps_amrwb_io_compact_header_config
cd mediaMin
./mediaMin -cx86 -i../test_files/stv16c_16kHz_12650_full_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
./mediaMin -cx86 -i../test_files/stv16c_16kHz_12650_compact_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
cd ..
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_23850_full_header.wav -Csession_config/evs_16kHz_23850bps_amrwb_io_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_23850_full_header.pcap -Csession_config/evs_16kHz_23850bps_amrwb_io_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_23850_compact_header.wav -Csession_config/evs_16kHz_23850bps_amrwb_io_compact_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_23850_compact_header.pcap -Csession_config/evs_16kHz_23850bps_amrwb_io_compact_header_config
cd mediaMin
./mediaMin -cx86 -i../test_files/stv16c_16kHz_23850_full_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
./mediaMin -cx86 -i../test_files/stv16c_16kHz_23850_compact_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
cd ..
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_8850_full_header.wav -Csession_config/evs_16kHz_8850bps_amrwb_io_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_8850_full_header.pcap -Csession_config/evs_16kHz_8850bps_amrwb_io_full_header_config
cd mediaMin
./mediaMin -cx86 -i../test_files/stv16c_16kHz_8850_full_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
cd ..
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_6600_full_header.wav -Csession_config/evs_16kHz_6600bps_amrwb_io_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_6600_full_header.pcap -Csession_config/evs_16kHz_6600bps_amrwb_io_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_6600_compact_header.wav -Csession_config/evs_16kHz_6600bps_amrwb_io_compact_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_6600_compact_header.pcap -Csession_config/evs_16kHz_6600bps_amrwb_io_compact_header_config
cd mediaMin
./mediaMin -cx86 -i../test_files/stv16c_16kHz_6600_full_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
./mediaMin -cx86 -i../test_files/stv16c_16kHz_6600_compact_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
#
# mixed modes and mixed rates, including AMR-WB IO mode with bit shifted payloads
#
./mediaMin -cx86 -i../pcaps/evs_mixed_mode_mixed_rate.pcap -L -d0x0c0c0c01 -r0.9 -C../session_config/EVS_AMR-WB_IO_mode_payload_shift "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
cd ..
#
# VBR 5900 bps mode tests
#
./mediaTest -cx86 -itest_files/stv16c.wav -o"${MT_OUTPUTS}"/stv16c_16kHz_5900_full_header.wav -Csession_config/evs_16kHz_5900bps_full_header_config  
./mediaTest -cx86 -itest_files/stv16c.wav -o"${MT_OUTPUTS}"/stv16c_16kHz_5900_compact_header.wav -Csession_config/evs_16kHz_5900bps_compact_header_config  
./mediaTest -cx86 -itest_files/stv16c.wav -o"${MT_OUTPUTS}"/stv16c_16kHz_5900_full_header_dtx_disabled.wav -Csession_config/evs_16kHz_5900bps_full_header_dtx_disabled_config
./mediaTest -cx86 -itest_files/stv16c.wav -o"${MT_OUTPUTS}"/stv16c_16kHz_5900_compact_header_dtx_disabled.wav -Csession_config/evs_16kHz_5900bps_compact_header_dtx_disabled_config
./mediaTest -cx86 -itest_files/stv16c.wav -o"${MT_OUTPUTS}"/stv16c_16kHz_5900_full_header.pcap -Csession_config/evs_16kHz_5900bps_full_header_config
./mediaTest -cx86 -itest_files/stv16c.wav -o"${MT_OUTPUTS}"/stv16c_16kHz_5900_compact_header.pcap -Csession_config/evs_16kHz_5900bps_compact_header_config
./mediaTest -cx86 -itest_files/stv16c.wav -o"${MT_OUTPUTS}"/stv16c_16kHz_5900_full_header_dtx_disabled.pcap -Csession_config/evs_16kHz_5900bps_full_header_dtx_disabled_config
./mediaTest -cx86 -itest_files/stv16c.wav -o"${MT_OUTPUTS}"/stv16c_16kHz_5900_compact_header_dtx_disabled.pcap -Csession_config/evs_16kHz_5900bps_compact_header_dtx_disabled_config
./mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o"${MT_OUTPUTS}"/AAdefaultBusinessHoursGreeting_16kHz_5900_full_header.wav -Csession_config/evs_16kHz_5900bps_full_header_config
./mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o"${MT_OUTPUTS}"/AAdefaultBusinessHoursGreeting_16kHz_5900_compact_header.wav -Csession_config/evs_16kHz_5900bps_compact_header_config
./mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o"${MT_OUTPUTS}"/AAdefaultBusinessHoursGreeting_8kHz_5900_full_header.wav -Csession_config/evs_8kHz_input_8kHz_5900bps_full_header_config
./mediaTest -cx86 -itest_files/AAdefaultBusinessHoursGreeting.pcm -o"${MT_OUTPUTS}"/AAdefaultBusinessHoursGreeting_8kHz_5900_compact_header.wav -Csession_config/evs_8kHz_input_8kHz_5900bps_compact_header_config

./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stvc16_48kHz_5900.wav -Csession_config/evs_48kHz_input_16kHz_5900bps_full_band_config
./mediaTest -cx86 -itest_files/T_mode.wav -o"${MT_OUTPUTS}"/T_mode_48kHz_5900.wav -Csession_config/evs_48kHz_input_16kHz_5900bps_full_band_config

cd mediaMin
./mediaMin -cx86 -i../test_files/stv16c_16kHz_5900_full_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
./mediaMin -cx86 -i../test_files/stv16c_16kHz_5900_compact_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"

./mediaMin -cx86 -i../pcaps/evs_5900_1_hf0.rtpdump -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
./mediaMin -cx86 -i../pcaps/evs_5900_1_hf1.rtpdump -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
#
# VBR 5900 stereo (2-channel)
#
./mediaMin -cx86 -i../pcaps/evs_5900_2_hf0.rtpdump -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
./mediaMin -cx86 -i../pcaps/evs_5900_2_hf1.rtpdump -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"

cd ..
#
# 24400 bps tests
#
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_24400_full_header.pcap -Csession_config/evs_16kHz_24400bps_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_24400_compact_header.pcap -Csession_config/evs_16kHz_24400bps_compact_header_config
cd mediaMin
./mediaMin -cx86 -i../test_files/stv16c_16kHz_24400_full_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
./mediaMin -cx86 -i../test_files/stv16c_16kHz_24400_compact_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"

cd ..
#
# 13200 bps tests, including RF enable mode tests
#
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_13200_full_header.wav -Csession_config/evs_16kHz_13200bps_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_13200_compact_header.wav -Csession_config/evs_16kHz_13200bps_compact_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_13200_full_header.pcap -Csession_config/evs_16kHz_13200bps_full_header_config
./mediaTest -cx86 -itest_files/stv16c.INP -o"${MT_OUTPUTS}"/stv16c_16kHz_13200_compact_header.pcap -Csession_config/evs_16kHz_13200bps_compact_header_config
cd mediaMin
./mediaMin -cx86 -i../test_files/stv16c_16kHz_13200_full_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
./mediaMin -cx86 -i../test_files/stv16c_16kHz_13200_compact_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"

./mediaMin -cx86 -i../test_files/stv16c_16kHz_13200_full_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"
./mediaMin -cx86 -i../test_files/stv16c_16kHz_13200_compact_header.pcap -L -d0x08000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"

./mediaMin -cx86 -i../pcaps/evs_long_rate_alignment.pcap -L -d0x20018000c11 -r0.9 "${MM_WAV_OUTPUTS}" "${MM_PCAP_OUTPUTS% *}" "${MM_PCAP_OUTPUTS#* }"

# return to mediaTest folder (starting point)
 cd ..
 