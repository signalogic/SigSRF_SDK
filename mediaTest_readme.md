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
To compare with 3GPP reference file:
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

## Frame Mode Tests

Frame mode tests perform encode, decode, or transcoding based on specifications in a "configuration file" given in the command line (see notes below).  Frame mode tests use Voplib APIs but not Pktlib APIs.  The main objectives are to check for bit-exact results, measure audio quality, and measure basic transcoding performance, including sampling rate conversion.  The following examples use the EVS codec. 

```C
./mediaTest -cx86 -M4 -Csession_config/frame_test_config

./mediaTest -cx86 -M4 -Csession_config/frame_test_config_wav_output
```

## Packet Mode Tests

Packet mode tests perform encode, decode, or transcoding based on specifications in a "session configuration file" given in the command line (see notes below).  Packet mode tests use both Voplib and Pktlib APIs.  Pktlib APIs include session creation, packet Rx and parsing, packet formatting and Tx, jitter buffer, ptime handling (transrating), and more.  The main objectives are to measure transcoding performance with full packet flow, including real-world media framework elements. The following examples use the EVS codec. 

```C
./mediaTest -M0 -cx86 -C session_config/pcap_file_test_config -i pcaps/pcmutest.pcap -i pcaps/EVS_13.2_16000.pcap

./mediaTest -M0 -cx86 -C session_config/pcap_file_test_config -i pcaps/pcmutest.pcap -i pcaps/EVS_13.2_16000.pcap -o stream1_xcoded.pcap -o stream2_xcoded.pcap
```

## Notes

1) NB = Narrowband (8 kHz), WB = Wideband (16 kHz), SWB = Super Wideband (32 kHz)
2) Comparison results are bit-exact if the cmp command gives no messages
3) The demo will store .wav files in either 16-bit linear (PCM) format or 8-bit G711 (uLaw) format, depending on the command line specs.  All generated .wav files can be played with Win Media or other player
4) The demo stores EVS compressed bitstream files in ".cod" format, with a MIME header. This format is compatible with 3GPP reference tools, for example you can take a mediaTest generated .cod file and feed it to the 3GPP decoder
5) session config files (specified by the -C cmd line option), contain codec, sampling rate, bitrate, DTX, ptime, and other options. They may be edited

