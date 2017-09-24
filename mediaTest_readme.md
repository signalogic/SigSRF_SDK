# mediaTest Demo

After installing the [SigSRF SDK eval](https://github.com/signalogic/SigSRF_SDK), here are some command lines and notes for the mediaTest demo.  Input and output options include network I/O, pcap file, and audio file format files (raw audio, .au, and .wav).  The demo is limited to two (2) concurrent transcoding streams, and two (2) concurrent instances (one instance = console window), for a total of four (4) streams.  The commercial software has no limitations for concurrency or multiuser, for either bare metal or VM operation.

mediaTest serves two (2) purposes:

 - an example application, including source code, showing how to use Pktlib and Voplib APIs (see architecture diagram on the SigSRF page)
 
 - perform test and measurement, including codec audio quality and performance, pcap verification and transcoding, and support for waveform and compressed bitstream file formats

# Table of Contents

[Codec Tests](#CodecTests)<br/>
&nbsp;&nbsp;[coCPU Codec Tests](#coCPUCodecTests)<br/>
[Frame Mode Tests](#FrameModeTests)<br/>
[Packet Mode Tests](#PacketModeTests)<br/>
&nbsp;&nbsp;[Convert Pcap to Wav](#ConvertPcap2Wav)<br/>
&nbsp;&nbsp;[Multiple RTP Streams (RFC8108)](#MultipleRTPStreams)<br/>
&nbsp;&nbsp;[Duplicated RTP Streams (RFC7198)](#DuplicatedRTPStreams)<br/>
&nbsp;&nbsp;[Session Configuration File Format](#SessionConfigFileFormat)<br/>
[DTX Handling](#DTXHandling)<br/>
[DTMF Handling](#DTMFHandling)<br/>
[Packet Stats Logging](#PacketStatsLogging)<br/>
[Pktlib and Jitter Buffer Notes](#PktlibandJitterBufferNotes)<br/>
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
./mediaTest -cx86 -M4 -Csession_config/frame_test_config -L

./mediaTest -cx86 -M4 -Csession_config/frame_test_config_wav_output -L
```

Below is a frame mode command line that reads a pcap file and outputs to wav file.  No jitter buffering is done, so any out-of-order packets, DTX packets, or SSRC changes are not handled.  The wav file sampling rate is determined from the session config file.

```C
./mediaTest -M4 -cx86 -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -oevs_16khz_13200bps_FH_IPv4.wav -Csession_config/pcap_file_test_config -L
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
./mediaTest -M0 -cx86 -ipcaps/pcmutest.pcap -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -Csession_config/pcap_file_test_config -L

./mediaTest -M0 -cx86 -ipcaps/pcmutest.pcap -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -ostream1_xcoded.pcap -ostream2_xcoded.pcap -Csession_config/pcap_file_test_config -L
```
The screencap below shows mediaTest output after the second command line.

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/mediatest_demo_screencap.png?raw=true "mediaTest pcap I/O command line example")

<a name="ConvertPcap2Wav"></a>
### Convert Pcap to Wav

Here are two simple mediaTest demo command lines that convert an EVS pcap to a wav file:

```C
./mediaTest -M0 -cx86 -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -oevs_16khz_13200bps_FH_IPv4.wav -Csession_config/pcap_file_test_config -L

./mediaTest -M0 -cx86 -ipcaps/evs_16khz_13200bps_CH_PT127_IPv4.pcap -oevs_16khz_13200bps_CH_PT127_IPv4.wav -Csession_config/pcap_file_test_config -L
```

In this case, unlike the similar frame mode test example above, jitter buffering is peformed, so out-of-order packets, DTX packets, and SSRC changes are handled.  Depending on the nature of network or pcap input, this can make the difference between intelligble audio or not.

An output pcap filename could also be added to the above command lines, i.e. decode audio to wav file, and also encode the audio to G711 or other codec.

Depending on the number of sessions defined in the session config file, multiple inputs and outputs can be entered (session config files are given by the -C cmd line option, see the Session Configuration File Format section below).

<a name="MultipleRTPStreams"></a>
### Multiple RTP Streams (RFC 8108)

RFC8108 is not yet ratified, but lays out compelling scenarios for multiple RTP streams per session, based on SSRC value transitions.  The mediaTest demo includes an example showing SSRC transition detections, both for creating new RTP streams on the fly (dynamically) and resuming previous ones.  When a new RTP stream is created, new encoder and decoder instances are also created dynamically, in order to maintain separate and contiguous content for each stream.  This is particularly important for advanced codecs such as EVS, which depend heavily on prior audio history for RF channel EDAC, noise modeling, and audio classification (e.g. voice vs. music).

Here is the mediaTest command line example included in the demo for multiple RTP streams:

```C
./mediaTest -M0 -cx86 -ipcaps/evs_16khz_13200bps_CH_RFC8108_IPv6.pcap -oevs_16khz_13200bps_CH_RFC8108_IPv6_g711.pcap -oevs_16khz_13200bps_CH_RFC8108_IPv6.wav -Csession_config/evs_16khz_13200bps_CH_RFC8108_IPv6_config -L
```

Here is a screen capture showing output for the above command line, with RTP stream transitions highlighted:

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/mediaTest_multiple_ssrc_screencap.png?raw=true "mediaTest multiple RTP stream command line example")

The packet stats log file produced by the above command (evs_multiple_ssrc_IPv6_g711.txt) shows how the SigSRF jitter buffer correctly collates and treats each stream separately, while still resolving out-of-order packets.  For a log file excerpt, see the Packet Stats and Logging section below.

<a name="DuplicatedRTPStreams"></a>
### Duplicated RTP Streams (RFC 7198)

RFC7198 is a method to address packet loss that does not incur unbounded delay, by duplicating packets and sending as separate redundant RTP streams.  Here is the mediaTest command line example included in the demo for RFC7198:

```C
./mediaTest -M0 -cx86 -ipcaps/evs_16khz_13200bps_CH_RFC7198_IPv6.pcap -oevs_16khz_13200bps_CH_RFC7198_IPv6_g711.pcap -oevs_16khz_13200bps_CH_RFC7198_IPv6.wav -Csession_config/evs_16khz_13200bps_CH_RFC7198_IPv6_config -L
```

<a name="SessionConfigFileFormat"></a>
### Session Configuration File Format

Here is a look inside a typical session configuration file, similar to those used in the above command lines:

```CoffeeScript
# Session 1
[start_of_session_data]

term1.local_ip = fd01:5d2::11:123:5222  # IPv6 format
term1.local_port = 18446
term1.remote_ip = d01:5d2::11:123:5201
term1.remote_port = 6170
term1.media_type = "voice"
term1.codec_type = "G711_ULAW"
term1.bitrate = 64000  # in bps
term1.ptime = 20  # in msec
term1.rtp_payload_type = 0
term1.dtmf_type = "NONE"  # "RTP" = handle DTMF event packets
term1.dtmf_payload_type = "NONE"
term1.sample_rate = 8000   # in Hz (note for fixed rate codecs this field is descriptive only)
## term1.dtx_handling = -1  # -1 disables DTX handling

term2.local_ip = 192.16.0.16  # IPv4 format
term2.local_port = 6154
term2.remote_ip = 192.16.0.130
term2.remote_port = 10242
term2.media_type = "voice"
term2.codec_type = "EVS"
term2.bitrate = 13200  # in bps
term2.ptime = 20  # in msec
term2.rtp_payload_type = 127
term2.dtmf_type = "NONE"
term2.dtmf_payload_type = "NONE"
term2.sample_rate = 16000   # in Hz
term2.header_format = 1  # Header format, applies to some codecs (EVS, AMR), 0 = CH (Compact Header), 1 = FH (Full Header)
## term2.dtx_handling = -1  # -1 disables DTX handling

[end_of_session_data]
```

Note that each session has one or two "terminations", or endpoints (term1 and term2).  A session with only term1 can accept and send streaming data with one endpoint, performing some processing on the data.  A session with term1 and term2 can exchange streaming data between endpoints, and perform some intermediate processing, such as transcoding.

When using pcap files, "remote" IP addr and UDP port values refer to pcap source, and "local" values refer to pcap destination.  When used with mediaTest, local IP addrs are the mediaTest application, and remote IP addrs are the endpoints. Rx traffic (i.e. incoming, with respect to mediaTest) should have destination IP addrs matching local IP addrs and source IP addrs matching remote IP addrs. Tx traffic (i.e. outgoing, w.r.t. mediaTest) will use local IP addrs for source IP addrs and remote IP addrs for destination IP addrs.  Below is a visual explanation:

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/session_config_pcap_terminology.png?raw=true "session config file and pcap terminology -- remote vs. local, src vs. dest")

<a name="DTXHandling"></a>
## DTX Handling

DTX (Discontinuous Transmission) handling can be enabled/disabled on per session basis, and is enabled by default (see the above session config file example).  When enabled the following functionality expands the output packet stream to meet the required duration for each DTX occurrence:

  - the Pktlib DSGetOrderedPackets() API reacts to incoming SID packets and inserts SID CNG (comfort noise generation) packets with adjusted timestamps and sequence numbers in the outgoing packet stream
  
  - the media decoder specified for the session generates comfort noise from the SIG CNG packets
  
A log file example showing incoming SID packets and outgoing DTX expansion is shown below in "Packet Stats Logging".

<a name ="DTMFHandling"></a>
## DTMF Handling

<a name="PacketStatsLogging"></a>
## Packet Stats Logging

mediaTest includes packet statistics logging for:

  * incoming packets (network input, pcap file)
  * jitter buffer output
  * outgoing packets (network output, pcap file)

In the above command lines, the -L entry activates packet logging, with the first output filename found taken as the log filename but replaced with a ".txt" extension.  If -Lxxx is given then xxx becomes the log filename.

Statistics logged include packets dropped, out-of-order (ooo), missing, and duplicated.  Packets are grouped by SSRC (see Multiple RTP Streams section above), with each entry showing sequence number, timestamp, and type (bitstream payload, DTX, SID, SID Reuse, DTMF Event, etc).  Here is a packet stats log file excerpt:

```CoffeeScript
Packet info for SSRC = 353707 (cont), first seq num = 685, last seq num = 872 ...

Seq num 685              timestamp = 547104, pkt len = 33
Seq num 686              timestamp = 547424, pkt len = 33
Seq num 688 ooo 687      timestamp = 548064, pkt len = 33
Seq num 687 ooo 688      timestamp = 547744, pkt len = 33
Seq num 690 ooo 689      timestamp = 548704, pkt len = 33
Seq num 689 ooo 690      timestamp = 548384, pkt len = 33
Seq num 691              timestamp = 549024, pkt len = 33
Seq num 692              timestamp = 549344, pkt len = 6 (DTX)
:
:
```

Here is an excerpt from a stream summary:

```CoffeeScript
:
:
Seq num 10001            timestamp = 451024, pkt len = 33
Seq num 10002            timestamp = 451344, pkt len = 6 (DTX)
Seq num 10003            timestamp = 453904, pkt len = 6 (DTX)
Seq num 10004            timestamp = 456464, pkt len = 6 (DTX)

Out-of-order seq numbers = 0, missing seq numbers = 22, number of DTX packets = 78

Total packets dropped = 0
Total packets duplicated = 0
```

As mentioned in "DTX Handling" above, here is a log file example, first showing incoming SID packets...

```CoffeeScript
:
:
Seq num 22              timestamp = 107024, pkt len = 33
Seq num 23              timestamp = 107344, pkt len = 33
Seq num 24              timestamp = 107664, pkt len = 6 (SID)
Seq num 25              timestamp = 110224, pkt len = 6 (SID)
Seq num 26              timestamp = 112144, pkt len = 33
Seq num 27              timestamp = 112464, pkt len = 33
:
:
```

... and corresponding outgoing SID and SID comfort noise packets:


```CoffeeScript
:
:
Seq num 22              timestamp = 107024, pkt len = 33
Seq num 23              timestamp = 107344, pkt len = 33
Seq num 24              timestamp = 107664, pkt len = 6 (SID)
Seq num 25              timestamp = 107984, pkt len = 6 (SID CNG-R)
Seq num 26              timestamp = 108304, pkt len = 6 (SID CNG-R)
Seq num 27              timestamp = 108624, pkt len = 6 (SID CNG-R)
Seq num 28              timestamp = 108944, pkt len = 6 (SID CNG-R)
Seq num 29              timestamp = 109264, pkt len = 6 (SID CNG-R)
Seq num 30              timestamp = 109584, pkt len = 6 (SID CNG-R)
Seq num 31              timestamp = 109904, pkt len = 6 (SID CNG-R)
Seq num 32              timestamp = 110224, pkt len = 6 (SID)
Seq num 33              timestamp = 110544, pkt len = 6 (SID CNG-R)
Seq num 34              timestamp = 110864, pkt len = 6 (SID CNG-R)
Seq num 35              timestamp = 111184, pkt len = 6 (SID CNG-R)
Seq num 36              timestamp = 111504, pkt len = 6 (SID CNG-R)
Seq num 37              timestamp = 111824, pkt len = 6 (SID CNG-R)
Seq num 38              timestamp = 112144, pkt len = 33
Seq num 39              timestamp = 112464, pkt len = 33
:
:
```

<a name="PktlibandJitterBufferNotes"></a>
## Pktlib and Jitter Buffer Notes

As part of the SigSRF software, with its emphasis on high performance streaming, the Pktlib jitter buffer has several advanced features, including:

* Handles out-of-order packets, including packet swaps
* Dynamic delay depth adjustment option
* Accepts sustained "burst mode" input, up to 10x faster than real-time (also referred to as "back pressure" in data analytics applications)
* Dynamic channel creation to support multiple RTP streams per session (see Multiple RTP Streams section above)
* Statistics API, logging, and several options such as overrun control, probation control, flush, and bypass modes

Some of the RFCs supported by Pktlib include:

* RFC 3550 (real-time transport protocol)
* RFC 7198 (packet duplication)
* RFC 8108 (multiple RTP streams)
* RFC 2833 and 4733 (DTMF)
* RFC 4867 (RTP payload and file storage for AMR-NB and AMR-WB codecs)
* RFC 3551, 3558, 4788, 5188, 5391, 5993, 6716

<a name="mediaTestNotes"></a>
## mediaTest Notes

1) In the command line filenames above, CH = Compact Header, FH = Full Header, PTnnn = Payload Type nn.  Some filenames contain values indicating sampling rate and bitrate (denoted by NNkhz and NNbps).  Some pcap filenames contain packets organized according to a specific RFC (denoted by RFCnnnn).
2) NB = Narrowband (8 kHz sampling rate), WB = Wideband (16 kHz), SWB = Super Wideband (32 kHz)
3) Comparison results are bit-exact if the cmp command gives no messages
4) The demo will store .wav files in either 16-bit linear (PCM) format or 8-bit G711 (uLaw) format, depending on the command line specs.  All generated .wav files can be played with Win Media, VLC, or other player
5) The demo stores EVS compressed bitstream files in ".cod" format, with a MIME header and with FH formatted frames (i.e. every frame includes a ToC byte). This format is compatible with 3GPP reference tools, for example you can take a mediaTest generated .cod file and feed it to the 3GPP decoder, and vice versa you can take a 3GPP encoder generated .cod file and feed it to the mediaTest command line.  See examples in the "Using the 3GPP Decoder" section below.
6) session config files (specified by the -C cmd line option), contain codec, sampling rate, bitrate, DTX, ptime, and other options. They may be edited.  See the "Session Configuration File Format" section above.
7) Transcoding in frame mode tests is not supported yet, will be added soon


<a name="3GPPNotes"></a>
## 3GPP Reference Code Notes

<a name="Using3GPPDecoder"></a>
### Using the 3GPP Decoder

*Note: the examples in this section assume you have downloaded the 3GPP reference code and installed somewhere on your system.*

The 3GPP decoder can be used as the "gold standard" reference for debug and comparison in several situations. Below are a few examples.

<a name="VerifyingEVSpcap"></a>
### Verifying an EVS pcap

In some cases, maybe due to unintelligble audio output, questions about pcap format or capture method, SDP descriptor options used for EVS encoding, etc, you may want to simply take a pcap, extract its EVS RTP payload stream, and copy to a .cod file with MIME header suitable for 3GPP decoder input.  The mediaTest command line can do this, here are two examples:

```C
./mediaTest -cx86 -ipcaps/evs_16khz_13200bps_CH_PT127_IPv4.pcap -oEVS_pcap_extracted1.cod

./mediaTest -cx86 -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -oEVS_pcap_extracted2.cod
```
Next, run the 3GPP decoder:

```C
./EVS_dec -mime -no_delay_cmp 16 EVS_pcap_extracted1.cod 3GPP_decoded_audio1.raw

./EVS_dec -mime -no_delay_cmp 16 EVS_pcap_extracted2.cod 3GPP_decoded_audio2.raw
```

Note the 3GPP decoder will produce only a raw audio format file, so you will need to use sox or other tool to convert to .wav file for playback.  You can also decode with mediaTest directly to .wav format:

```C
./mediaTest -cx86 -iEVS_pcap_extracted1.cod -omediaTest_decoded_audio1.wav

./mediaTest -cx86 -iEVS_pcap_extracted2.cod -omediaTest_decoded_audio2.wav
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
