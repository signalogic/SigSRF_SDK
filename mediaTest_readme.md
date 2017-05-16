# mediaTest Getting Started

Here are some command lines to use with the mediaTest demo.

## EVS Codec Tests

The following command line will encode a 3GPP reference file (WB sampling rate, 13.2 kbps) to a compressed bitstream file:
```C
  ./mediaTest -cx86 -itest_files/stv16c.INP -otest_files/stv16c_13200_16kHz_mime.COD -Csession_config/codec_test_16kHz_13200bps_config
```
To compare with the relevant 3GPP reference file:
```C
  cmp reference_files/stv16c_13200_16kHz_mime_o3.COD test_files/stv16c_13200_16kHz_mime.COD
```
The following command line will encode and then decode a 3GPP reference file (WB sampling rate, 13.2 kbps), producing a .wav file:
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
The following command line will encode and then decode a 3GPP reference file (SWB sampling rate, 13.2 kbps), producing a .wav file you can listen to and experience EVS audio quality:
```C
./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv16c_13200_32kHz_mime.wav -Csession_config/codec_test_32kHz_13200bps_config
```
## coCPU EVS Codec Tests

The following command lines specify coCPU cores.  The first one does the same EVS WB test as above, and the seond one does an EVS NB test.  Both produce .wav files that you can listen to and experience EVS audio quality:

```C
./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv16c.INP -otest_files/c6x16c_j.wav 

./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv8c.INP -otest_files/c6x8c_j.wav -Csession_config/codec_test_8kHz_13200bps_config
```

## EVS Frame Mode Tests
```C
./mediaTest -cx86 -M4 -Csession_config/frame_test_config
```
[Can we also add here a cmd line where the config file gives a .wav file as an output ?]

## Packet Mode Tests
```C
./mediaTest -M0 -cx86 -C session_config/pcap_file_test_config -i pcaps/pcmutest.pcap -i pcaps/EVS_13.2_16000.pcap

./mediaTest -M0 -cx86 -C session_config/pcap_file_test_config -i pcaps/pcmutest.pcap -i pcaps/EVS_13.2_16000.pcap -o stream1_xcoded.pcap -o stream2_xcoded.pcap
```

## Notes

1) NB = Narrowband (8 kHz), WB = Wideband (16 kHz), SWB = Super Wideband (32 kHz)
2) Comparison results are bit-exact if the cmp command gives no messages
3) .wav files can be played with Win Media or other player.  Note that some .wav files may be stored in G711 format (uLaw compressed format).
4) session config files (specified by the -C cmd line option), contain codec, sampling rate, bitrate, DTX, ptime, and other options. They may be edited

