# mediaTest Getting Started

Here are some command lines to use with the mediaTest demo.

## EVS Codec Tests

The following command line will encode a 3GPP reference file (WB sampling rate, 13.2 kbps) to a compressed bitstream file:

  ./mediaTest -cx86 -itest_files/stv16c.INP -otest_files/stv16c_13200_16kHz_mime.COD -Csession_config/codec_test_16kHz_13200bps_config

To compare with the relevant 3GPP reference file:

  cmp reference_files/stv16c_13200_16kHz_mime_o3.COD test_files/stv16c_13200_16kHz_mime.COD

The following command line will encode and then decode a 3GPP reference file (WB sampling rate, 13.2 kbps), producing a .wav file:

  ./mediaTest -cx86 -itest_files/stv16c.INP -otest_files/stv16c_13200_16kHz_mime.wav -Csession_config/codec_test_16kHz_13200bps_config

The following command line will encode a 3GPP reference file (SWB sampling rate, 13.2 kbps) to a compressed bitstream file:

  ./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv32c_13200_32kHz_mime.COD -Csession_config/codec_test_32kHz_13200bps_config

To compare with 3GPP reference file:

  cmp reference_files/stv32c_13200_32kHz_mime_o3.COD test_files/stv32c_13200_32kHz_mime.COD

The following command line will encode and then decode a 3GPP reference file (SWB sampling rate, 13.2 kbps), producing a .wav file:

  ./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv16c_13200_32kHz_mime.wav -Csession_config/codec_test_32kHz_13200bps_config

Notes

1) NB = Narrowband (8 kHz), WB = Wideband (16 kHz), SWB = Super Wideband (32 kHz)
2) If the cmp command gives no output, then results are bit-exact
3) .wav files can be played with Win Media or other player.  Note that some .wav files may be stored in G711 format (uLaw compressed format).
4) sesion config files (specified by the -C cmd line option), contain codec, sampling rate, bitrate, DTX, ptime, and other options.  They may be edited

## EVS Frame Mode Tests

./mediaTest -cx86 -M4 -Csession_config/frame_test_config

[Can we also add here a cmd line where the config file gives a .wav file as an output ?]

## Packet Mode Tests

./mediaTest -M0 -cx86 -Csession_config/packet_test_config_loopback


