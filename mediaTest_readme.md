# mediaTest Getting Started

Here are some command lines to use with the mediaTest demo.  The demo is limited to two (2) concurrent transcoding streams, and two (2) concurrent instances (one instance = console window), for a total of four (4) streams.  The commercial software has no concurrency or multiuser limitations, for either bare metal or VM operation. 

## Codec Tests

Codec tests are low-level test that perform encode and/or decode using the specified codec.  No transcoding is performed.  The main objectives are to check for bit-exact results, measure audio quality, and measure performance.  The following examples use the EVS codec.  Codec tests do not use Voplib or Pktlib APIs.  The following command line will encode a 3GPP reference file (WB sampling rate, 13.2 kbps) to a compressed bitstream file:
```C
./mediaTest -cx86 -itest_files/stv16c.INP -otest_files/stv16c_13200_16kHz_mime.COD -Csession_config/codec_test_16kHz_13200bps_config
```
To compare with the relevant 3GPP reference file:
```C
cmp reference_files/stv16c_13200_16kHz_mime_o3.COD test_files/stv16c_13200_16kHz_mime.COD
```
The following command line will encode and then decode a 3GPP reference file (WB sampling rate, 13.2 kbps), producing a .wav file you can listen to and experience EVS audio quality:
```C
./mediaTest -cx86 -itest_files/stv16c.INP -otest_files/stv16c_13200_16kHz_mime.wav 
```
The following command line will encode a 3GPP reference file (SWB sampling rate, 13.2 kbps) to a compressed bitstream file:
```C
./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv32c_13200_32kHz_mime.COD -Csession_config/codec_test_32kHz_13200bps_config
```
To compare with the relevant 3GPP reference file:
```C
cmp reference_files/stv32c_13200_32kHz_mime_o3.COD test_files/stv32c_13200_32kHz_mime.COD
```
The following command line will encode and then decode a 3GPP reference file (SWB sampling rate, 13.2 kbps), producing a .wav file:
```C
./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv16c_13200_32kHz_mime.wav -Csession_config/codec_test_32kHz_13200bps_config
```
## coCPU Codec Tests

The following command lines specify coCPU cores.  The first one does the same EVS WB test as above, and the second one does an EVS NB test.  Both produce .wav files that you can listen to and experience EVS audio quality:

```C
./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv16c.INP -otest_files/c6x16c_j.wav 

./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv8c.INP -otest_files/c6x8c_j.wav -Csession_config/codec_test_8kHz_13200bps_config
```

In the above command lines, eight (8) coCPU cores are specified, although the free demo is limited to one coCPU core per instance.  The coCPU clock rate can be set from 1 to 1.6 GHz (-f1000 to -f1600 in the command line).

Below is a screen capture showing side-by-side comparison of the NB output with the 3GPP reference waveform:

![alt text](https://github.com/signalogic/SigSRF_SDK/blob/master/images/EVS_NB_compare_screen_cap.png "EVS NB comparison betwen co"CPU fixed-point and 3GPP reference fixed-point")

## Frame Mode Tests

Frame mode tests perform encode, decode, or transcoding based on specifications in a "configuration file" given in the command line (see notes below).  Frame mode tests use Voplib APIs but not Pktlib APIs.  The main objectives are to check for bit-exact results, measure audio quality, and measure basic transcoding performance, including sampling rate conversion.  The following examples use the EVS codec. 

```C
./mediaTest -cx86 -M4 -Csession_config/frame_test_config

./mediaTest -cx86 -M4 -Csession_config/frame_test_config_wav_output
```

## Packet Mode Tests

Packet mode tests perform encode, decode, or transcoding based on specifications in a "session configuration file" given in the command line (see notes below).  Packet mode tests use both Voplib and Pktlib APIs.  Pktlib APIs include session creation, packet Rx and parsing, packet formatting and Tx, jitter buffer, ptime handling (transrating), and more.  The main objectives are to measure transcoding performance with full packet flow, including real-world media framework elements. The following examples use the EVS codec.

The first command line below does the following:

* reads IP/UDP/RTP packets from the specified input pcap files
* listens for all UDP ports (on any network interface)
* sends transcoded packets over the network

The second command line is similar, but also does the following:

* writes each output stream to the corresponding output .pcap file given on the command line
* sends over the network any additional streams beyond the number of output files given

```C
./mediaTest -M0 -cx86 -Csession_config/pcap_file_test_config -ipcaps/pcmutest.pcap -ipcaps/EVS_13.2_16000.pcap

./mediaTest -M0 -cx86 -Csession_config/pcap_file_test_config -ipcaps/pcmutest.pcap -ipcaps/EVS_13.2_16000.pcap -ostream1_xcoded.pcap -ostream2_xcoded.pcap
```
Here is a look inside the session configuration file (pcap_file_test_config) used in the above command lines:

```CoffeeScript
# Session 1
[start_ofsession_data]

term1.local_ip=192.16.0.147
term1.local_port=18446
term1.remote_ip=192.16.0.16
term1.remote_port=6170
term1.media_type="voice"
term1.codec_type="G711_ULAW"
term1.bitrate=64000  #in kbps
term1.ptime=20
term1.rtp_payload_type=0
term1.dtmf_type="NONE"
term1.dtmf_payload_type="NONE"

term2.local_ip=192.16.0.16
term2.local_port=6154
term2.remote_ip=192.16.0.130
term2.remote_port=10242
term2.media_type="voice"
term2.codec_type="EVS"
term2.bitrate=13200  # in kbps
term2.ptime=20  # in msec
term2.rtp_payload_type=127
term2.dtmf_type="NONE"
term2.dtmf_payload_type="NONE"
term2.evs_sample_rate=8000   # in Hz
term2.evs_header_full=1  # Full header format used

[end_of_session_data]
```

## Notes

1) NB = Narrowband (8 kHz), WB = Wideband (16 kHz), SWB = Super Wideband (32 kHz)
2) Comparison results are bit-exact if the cmp command gives no messages
3) The demo will store .wav files in either 16-bit linear (PCM) format or 8-bit G711 (uLaw) format, depending on the command line specs.  All generated .wav files can be played with Win Media or other player
4) The demo stores EVS compressed bitstream files in ".cod" format, with a MIME header. This format is compatible with 3GPP reference tools, for example you can take a mediaTest generated .cod file and feed it to the 3GPP decoder, and vice versa you can take a 3GPP decoder generated .cod file and feed it to the mediaTest command line
5) session config files (specified by the -C cmd line option), contain codec, sampling rate, bitrate, DTX, ptime, and other options. They may be edited
6) Transcoding in frame mode tests is not supported yet, will be added soon

