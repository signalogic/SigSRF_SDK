# mediaTest Demo

Assuming you have installed the [SigSRF SDK eval](https://github.com/signalogic/SigSRF_SDK), here are some command lines and notes for the mediaTest demo.  Input and output options include network I/O, pcap file, and audio file format files (raw audio, .au, and .wav).  The demo is limited to two (2) concurrent transcoding streams, and two (2) concurrent instances (one instance = console window), for a total of four (4) streams.  The commercial software has no concurrency or multiuser limitations, for either bare metal or VM operation.

mediaTest serves two (2) purposes:

 - an example application, including source code, showing how to use Pktlib and Voplib APIs (see architecture diagram on the SigSRF page)
 
 - perform test and measurement, including codec audio quality and performance, pcap verification and transcoding, and support for waveform and compressed bitstream file formats

# Table of Contents

[Codec Tests](#CodecTests)<br/>
&nbsp;&nbsp;[coCPU Codec Tests](#coCPUCodecTests)<br/>
[Frame Mode Tests](#FrameModeTests)<br/>
[Packet Mode Tests](#PacketModeTests)<br/>
&nbsp;&nbsp;[Multiple RTP Streams](#MultipleRTPStreams)<br/>
&nbsp;&nbsp;[Session Configuration File Format](#SessionConfigFileFormat)<br/>
[Packet Stats Logging](#PacketStatsLogging)<br/>
[Jitter Buffer Notes](#JitterBufferNotes)<br/>
[mediaTest Notes](#mediaTestNotes)<br/>
[3GPP Reference Code Notes](#3GPPNotes)<br/>
&nbsp;&nbsp;[Using the 3GPP Decoder](#Using3GPPDecoder)<br/>
&nbsp;&nbsp;[Verifying an EVS pcap](#VerifyingEVSpcap)<br/>
[Wireshark Notes](#WiresharkNotes)<br/>
&nbsp;&nbsp;[Playing Audio in Wireshark](#PlayingAudioWireshark)<br/>
&nbsp;&nbsp;[Saving Audio to File in Wireshark](#SavingAudioWireshark)<br/>

<a name="CodecTests"></a>
## Codec Tests

Codec tests are low-level tests that perform encode and/or decode using the specified codec.  No transcoding is performed, and Voplib and Pktlib APIs are not used.  The main objectives are to check for bit-exact results, measure audio quality, and measure performance.  The following command line will encode a 3GPP reference audio file (WB sampling rate, 13.2 kbps) to an EVS compressed bitstream file:
```C
./mediaTest -cx86 -itest_files/stv16c.INP -otest_files/stv16c_13200_16kHz_mime.COD -Csession_config/codec_test_16kHz_13200bps_config
```
To compare with the relevant 3GPP reference bitstream file:
```C
cmp reference_files/stv16c_13200_16kHz_mime_o3.COD test_files/stv16c_13200_16kHz_mime.COD
```
The following command line will encode and then decode a 3GPP reference audio file (WB sampling rate, 13.2 kbps), producing a .wav file you can listen to and experience EVS audio quality:
```C
./mediaTest -cx86 -itest_files/stv16c.INP -otest_files/stv16c_13200_16kHz_mime.wav 
```
The following command line will encode a 3GPP reference file audio (SWB sampling rate, 13.2 kbps) to an EVS compressed bitstream file:
```C
./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv32c_13200_32kHz_mime.COD -Csession_config/codec_test_32kHz_13200bps_config
```
To compare with the relevant 3GPP reference bitstream file:
```C
cmp reference_files/stv32c_13200_32kHz_mime_o3.COD test_files/stv32c_13200_32kHz_mime.COD
```
The following command line will encode and then decode a 3GPP reference bitstream file (SWB sampling rate, 13.2 kbps), producing a .wav file:
```C
./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv16c_13200_32kHz_mime.wav -Csession_config/codec_test_32kHz_13200bps_config
```
<a name="coCPUCodecTests"></a>
### coCPU Codec Tests

The following command lines specify coCPU cores.  The first one does the same EVS WB test as above, and the second one does an EVS NB test.  Both produce .wav files that you can listen to and experience EVS audio quality:

```C
./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv16c.INP -otest_files/c6x16c_j.wav 

./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv8c.INP -otest_files/c6x8c_j.wav -Csession_config/codec_test_8kHz_13200bps_config
```

In the above command lines, eight (8) coCPU cores are specified, although the free demo is limited to one coCPU core per instance.  The coCPU clock rate can be set from 1 to 1.6 GHz (-f1000 to -f1600 in the command line).  Depending on which coCPU card you have, up to 64 coCPU cores can be specified.  Multiple instances of mediaTest can make use of more cards.

Below is a screen capture showing overlay comparison of the NB output with the 3GPP reference waveform:

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/EVS_NB_compare_screen_cap.png?raw=true "EVS NB comparison between coCPU fixed-point and 3GPP reference fixed-point")

Note the small differences due to coCPU optimization for high capacity applications.  These differences do not perceptually affect audio quality.

<a name="FrameModeTests"></a>
## Frame Mode Tests

Frame mode tests perform encode, decode, or transcoding based on specifications in a "configuration file" given in the command line (see notes below).  Voplib APIs in mediaTest source code examples include codec instance creation, encode, and decode.  The main objectives are to check for bit-exact results, measure audio quality, and measure basic transcoding performance, including sampling rate conversion.  The following examples use the EVS codec. 

```C
./mediaTest -cx86 -M4 -Csession_config/frame_test_config

./mediaTest -cx86 -M4 -Csession_config/frame_test_config_wav_output
```

Below is a frame mode command line that reads a pcap file and outputs to wav file.  No jitter buffering is done, so any out-of-order packets, DTX packets, or SSRC changes are not handled.  The wav file sampling rate is determined from the session config file.

```C
./mediaTest -M4 -cx86 -Csession_config/pcap_file_test_config -ipcaps/EVS_13.2_16000.pcap -oEVS_13.2_16000.wav
```

<a name="PacketModeTests"></a>
## Packet Mode Tests

Packet mode tests perform encode, decode, or transcoding based on specifications in a "session configuration file" given in the command line (see notes below).  Packet mode tests read/write IP/UDP/RTP packet streams from/to network interfaces or pcap files.  Both IPv4 and IPv6 format streams are supported.  Pktlib APIs in mediaTest source code examples include include session creation, packet Rx and parsing, packet formatting and Tx, jitter buffer, ptime handling (transrating), and more.  The main objectives are to measure transcoding performance with full packet flow, including real-world media framework elements. The following examples use the EVS codec.

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
The screencap below shows mediaTest output after the second command line.

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/mediatest_demo_screencap.png?raw=true "mediaTest pcap I/O command line example")

Below is a packet mode command line similar to the above examples, except output is to wav file instead of pcap.  In this case, unlike the equivalent frame mode test above, jitter buffering is peformed, so out-of-order packets, DTX packets, and SSRC changes are handled.  Depending on the nature of network or pcap input, this can make the difference between intelligble audio or not.

```C
./mediaTest -M0 -cx86 -Csession_config/pcap_file_test_config -ipcaps/EVS_13.2_16000.pcap -oEVS_13.2_16000.wav
```

<a name="MultipleRTPStreams"></a>
### Multiple RTP Streams

RFC8108 is not yet ratified, but lays out compelling scenarios for multiple RTP streams within a single session, based on SSRC value transitions.  The mediaTest demo includes an example showing SSRC transition detections, both for creating new RTP streams on the fly (dynamically) and resuming previous ones.  When a new RTP stream is created, new encoder and decoder instances are also created dynamically.  This is particularly important for newer codecs such as EVS, which depends on prior audio history for RF channel EDAC, noise modeling, and audio classification (e.g. voice vs. music).

Here is the mediaTest command line example included in the demo for multiple RTP streams:

```C
./mediaTest -M0 -cx86 -ipcaps/evs_multiple_ssrc_IPv6.pcap -oevs_multiple_ssrc_IPv6_g711.pcap -oevs_multiple_ssrc_IPv6.wav -Csession_config/evs_multiple_ssrc_IPv6_config
```

The packet stats log file produced by the above command (evs_multiple_ssrc_IPv6_g711.txt) shows each SSRC stream, and shows how the SigSRF jitter buffer correctly collates each stream, while still resolving out-of-order packets.

<a name="SessionConfigFileFormat"></a>
### Session Configuration File Format

Here is a look inside the session configuration file (pcap_file_test_config) used in the above command lines:

```CoffeeScript
# Session 1
[start_ofsession_data]

term1.local_ip = 192.16.0.147
term1.local_port = 18446
term1.remote_ip = 192.16.0.16
term1.remote_port = 6170
term1.media_type = "voice"
term1.codec_type = "G711_ULAW"
term1.bitrate = 64000  # in kbps
term1.ptime = 20  # in msec
term1.rtp_payload_type = 0
term1.dtmf_type = "NONE"
term1.dtmf_payload_type = "NONE"

term2.local_ip = 192.16.0.16
term2.local_port = 6154
term2.remote_ip = 192.16.0.130
term2.remote_port = 10242
term2.media_type = "voice"
term2.codec_type = "EVS"
term2.bitrate = 13200  # in kbps
term2.ptime = 20  # in msec
term2.rtp_payload_type = 127
term2.dtmf_type = "NONE"
term2.dtmf_payload_type="NONE"
term2.sample_rate = 16000   # in Hz
term2.evs_header_full=1  # Full Header format used (0 = Compact Header format)

[end_of_session_data]
```

Note that each session has one or two "terminations", or endpoints (term1 and term2).  A session with only term1 can accept and send streaming data with one endpoint, performing some processing on the data.  A session with term1 and term2 can exchange streaming data between endpoints, and perform some intermediate processing, such as transcoding.

When using pcap files, "remote" IP addr and UDP port values refer to pcap source, and "local" values refer to pcap destination.  When used with mediaTest, local IP addrs are the mediaTest application, and remote IP addrs are the endpoints. Rx traffic (i.e. incoming, with respect to mediaTest) should have destination IP addrs matching local IP addrs and source IP addrs matching remote IP addrs. Tx traffic (i.e. outgoing, w.r.t. mediaTest) will use local IP addrs for source IP addrs and remote IP addrs for destination IP addrs.  Below is a visual explanation:

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/session_config_pcap_terminology.png?raw=true "session config file and pcap terminology -- remote vs. local, src vs. dest")

<a name="PacketStatsLogging"></a>
## PacketStatsLogging

mediaTest includes packet statistics logging for:

  * incoming packets (network input, pcap file)
  * jitter buffer output
  * outgoing packets (network output, pcap file)

Statistics include packets dropped, out-of-order (ooo), missing, and duplicated.  Packets are grouped by SSRC (see Multiple RTP Streams section above), with each entry showing sequence number, timestamp, and type (bitstream payload, DTX, etc).  Here is a packet stats log file excerpt:

```CoffeeScript
Packet info for SSRC = 353707 (cont), first seq num = 685, last seq num = 872 ...

Seq num 685              timestamp = 547104, pkt len = 33
Seq num 686              timestamp = 547424, pkt len = 33
Seq num 687 ooo 688      timestamp = 548064, pkt len = 33
Seq num 688 ooo 687      timestamp = 547744, pkt len = 33
Seq num 689 ooo 690      timestamp = 548704, pkt len = 33
Seq num 690 ooo 689      timestamp = 548384, pkt len = 33
Seq num 691              timestamp = 549024, pkt len = 33
Seq num 692              timestamp = 549344, pkt len = 6 (DTX)
```

<a name="JitterBufferNotes"></a>
## Jitter Buffer Notes

As part of the SigSRF software, with its emphasis on high performance streaming, the Pktlib jitter buffer has several advanced features, including:

* Handles out-of-order packets, including 2 packet swaps
* Dynamic delay depth adjustment option
* Accepts sustained "burst mode" input, up to 10x faster than the media ptime (also referred to as "back pressure" in data analytics applications)
* Statistics API, logging, and several options such as overrun control, probation control, flush, and bypass modes

<a name="mediaTestNotes"></a>
## mediaTest Notes

1) NB = Narrowband (8 kHz sampling rate), WB = Wideband (16 kHz), SWB = Super Wideband (32 kHz)
2) Comparison results are bit-exact if the cmp command gives no messages
3) The demo will store .wav files in either 16-bit linear (PCM) format or 8-bit G711 (uLaw) format, depending on the command line specs.  All generated .wav files can be played with Win Media or other player
4) The demo stores EVS compressed bitstream files in ".cod" format, with a MIME header. This format is compatible with 3GPP reference tools, for example you can take a mediaTest generated .cod file and feed it to the 3GPP decoder, and vice versa you can take a 3GPP encoder generated .cod file and feed it to the mediaTest command line.  See examples in the "Using the 3GPP Decoder" section below.
5) session config files (specified by the -C cmd line option), contain codec, sampling rate, bitrate, DTX, ptime, and other options. They may be edited.  See the "Session Configuration File Format" section above.
6) Transcoding in frame mode tests is not supported yet, will be added soon


<a name="3GPPNotes"></a>
## 3GPP Reference Code Notes

<a name="Using3GPPDecoder"></a>
### Using the 3GPP Decoder

*Note: the examples in this section assume you have downloaded the 3GPP reference code and installed somewhere on your system.*

The 3GPP decoder can be used as the "gold standard" reference for debug and comparison in several situations. Below are a few examples.

<a name="VerifyingEVSpcap"></a>
### Verifying an EVS pcap

In some cases, maybe due to unintelligble audio output, questions about pcap format or capture method, SDP descriptor options used for EVS encoding, etc, you may want to simply take a pcap, extract its EVS RTP payload stream, and copy to a .cod file with MIME header suitable for 3GPP decoder input.  The mediaTest command line can do this, here is an example:

```C
./mediaTest -cx86 -ipcaps/EVS_13.2_compact_format_PT_127.pcap -oEVS_pcap_extracted.cod
```
Next, run the 3GPP decoder:

```C
./EVS_dec -mime -no_delay_cmp 16 EVS_pcap_extracted.cod 3GPP_decoded_audio.raw
```

Note the 3GPP decoder will produce only a raw audio format file, so you will need to use sox or other tool to convert to .wav file for playback.  You can also decode with mediaTest directly to .wav format:

```C
./mediaTest -cx86 -iEVS_pcap_extracted.cod -omediaTest_decoded_audio.wav
```
<a name="WiresharkNotes"></a>
## Wireshark Notes

<a name="PlayingAudioWireshark"></a>
### Playing Audio in Wireshark

As a quick reference, the basic procedure for playing audio from G711 encoded caps from within Wireshark is given here.  These instructions are for pcaps containing one RTP stream (not multiple streams).

1. First, when you run mediaTest, make sure your session config file has the correct payload type set for either G711 uLaw or ALaw.

 - Session config payload type values should be 0 (zero) for uLaw and 8 (eight) for ALaw
 - this is the "termN.rtp_payload_type" field in the session config file (see above), where N is 1 or 2, depending on which endpoint is G711

2. Second, Wireshark must "see" the stream in RTP format:

 - Right click a packet in the stream and select "decode as"
 - Under 'Current' select "RTP"

After doing this, the protocol field in the main Wireshark window for the relevant packets should display "RTP".

3. Playing the stream:

 - In the menu bar, go to Telephony -> RTP -> RTP Streams
 - Select the relevant RTP stream in the now displayed "RTP Streams" pop-up window
 - Click "Analyze" in the "RTP Streams" pop-up window
 - Click "Play Streams" in the "RTP Stream Analysis" pop-up window
 - Click the play button in the "RTP Player" pop-up window

<a name="SavingAudioWireshark"></a>
### Saving Audio to File in Wireshark

The procedure for saving audio to file from G711 encoded pcaps is similar to playing audio as noted above.  Here are additional instructions to save the audio data to .au file format, and then use the Linux "sox" program to convert to .wav format.  Note that it's also possible to save to .raw file format (no file header), but that is not covered here.

1. First, follow steps 1. and 2. in the "Playing Audio" notes above.

2. Second, save to .au file from Wireshark:

 - In the menu bar, go to Telephony > RTP > RTP Streams
 - Select the relevant RTP stream in the now displayed "RTP Streams" pop-up window
 - Click Analyze in the "RTP Streams" pop-up window
 - Click Save > Forward Audio Stream to save (this will give .au and .raw options)
 - Enter in filename and choose the location to save, then click Save

3. Last, convert the .au file to .wav format:

 - Enter the following Linux command:
```C 
  sox audio_file.au audio_file.wav
 ```

When .au format is given to Wireshark, it performs uLaw or ALaw conversion internally (based on the payload type in the RTP packets) and writes out 16-bit linear (PCM) audio samples.  If for some reason you are using .raw format, then you will have to correctly specify uLaw vs. ALaw to sox, Audacity, or other conversion program.  If that doesn't match the mediaTest session config file payload type value, then the output audio data may still be audible but incorrect (for example it may have a dc offset or incorrect amplitude scale).

*Note: the above instructions apply to Wireshark version 2.2.6.*
