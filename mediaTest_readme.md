# mediaTest Demo

After installing the [SigSRF SDK eval](https://github.com/signalogic/SigSRF_SDK), this page gives example command lines and basic documentation for mediaTest, including:

 - packet streaming, both real-time and unlimited rate buffering, with packet re-ordering and packet RFCs

 - test and measurement, including codec audio quality and performance, media RFC verification, and transcoding

 - an example application, including source code, showing how to use Pktlib and Voplib APIs (see architecture diagram on the SigSRF page)

Input and output options include network I/O, pcap file, and audio file format files (raw audio, .au, and .wav); the example command lines below use pcap, wav, and cod (compressed bitstream format) files included with the demo.  The demo is limited to two (2) concurrent transcoding streams, and two (2) concurrent instances (one instance = console window), for a total of four (4) streams.  The commercial software has no limitations for concurrency or multiuser, for either bare metal or VM operation.

# Other Demos

[iaTest Demo (Image Analytics)](https://github.com/signalogic/SigSRF_SDK/blob/master/iaTest_readme.md)

[paTest Demo (Predictive Analytics)](https://github.com/signalogic/SigSRF_SDK/blob/master/paTest_readme.md)

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
[Variable Ptimes](#VariablePtimes)<br/>
[DTMF Handling](#DTMFHandling)<br/>
[Media Processing Insertion Point](#MediaProcessing)<br/>
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
./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv32c_13200_32kHz_mime.wav -Csession_config/codec_test_32kHz_13200bps_config
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

&nbsp;

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/evs_codec_2d_spectrograph.png?raw=true "Frequency domain EVS NB comparison between coCPU fixed-point and 3GPP reference fixed-point")

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

Packet mode tests perform encode, decode, or transcoding based on specifications in a "session configuration file" given in the command line (see notes below).  Packet mode tests read/write IP/UDP/RTP packet streams from/to network interfaces or pcap files.  Both IPv4 and IPv6 format streams are supported.  Pktlib APIs in mediaTest source code examples include session creation, packet Rx and parsing, packet formatting and Tx, jitter buffer, ptime handling (transrating), and more.  The main objectives are to measure transcoding performance with full packet flow, including real-world media framework elements. The following examples use the EVS codec.

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

The demo will work on any EVS pcap, including full header, compact header, and multiframe formats.  Combined with the .cod file input described above, this makes the mediaTest demo an "EVS player" that can read pcaps or .cod files (which use MIME "full header" format per the 3GPP spec).

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

The packet stats log file produced by the above command (evs_16khz_13200bps_CH_RFC8108_IPv6_g711.txt) shows how the SigSRF jitter buffer correctly collates and treats each stream separately, while still resolving out-of-order packets.  For a log file excerpt, see "Packet Stats and Logging" below.

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
term1.dtmf_type = "NONE"  # "RTP" = handle incoming DTMF event packets for term1 -> term2 direction, forward DTMF events for term2 -> term1 direction
term1.dtmf_payload_type = "NONE"  # A value should be given depending on the dtmf_type field
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
term2.dtmf_type = "NONE"  # "RTP" = handle incoming DTMF event packets for term2 -> term1 direction, forward DTMF events for term1 -> term2 direction
term2.dtmf_payload_type = "NONE"
term2.sample_rate = 16000   # in Hz
term2.header_format = 1  # Header format, applies to some codecs (EVS, AMR), 0 = CH (Compact Header), 1 = FH (Full Header)
## term2.dtx_handling = -1  # -1 disables DTX handling

[end_of_session_data]
```

Note that each session typically has one or two "terminations", or endpoints (term1 and term2).  A session with only term1 can accept and send streaming data with one endpoint, and perform processing on the data required by the endpoint, by the server running mediaTest, or both.  A session with term1 and term2 can exchange streaming data between endpoints, and perform intermediate processing, such as transcoding, speech recognition, overlaying or adding data to the streams, etc.

When using pcap files, "remote" IP addr and UDP port values refer to pcap source, and "local" values refer to pcap destination.  When used with mediaTest, local IP addrs are the mediaTest application, and remote IP addrs are the endpoints. Rx traffic (i.e. incoming, with respect to mediaTest) should have destination IP addrs matching local IP addrs and source IP addrs matching remote IP addrs. Tx traffic (i.e. outgoing, w.r.t. mediaTest) will use local IP addrs for source IP addrs and remote IP addrs for destination IP addrs.  Below is a visual explanation:

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/session_config_pcap_terminology.png?raw=true "session config file and pcap terminology -- remote vs. local, src vs. dest")

<a name="DTXHandling"></a>
## DTX Handling

DTX (Discontinuous Transmission) handling can be enabled/disabled on per session basis, and is enabled by default (see the above session config file example).  When enabled, each DTX occurrence is expanded to the required duration as follows:

  - the Pktlib DSGetOrderedPackets() API reacts to SID packets emerging from the jitter buffer and inserts SID CNG (comfort noise generation) packets with adjusted timestamps and sequence numbers in the buffer output packet stream
  
  - the media decoder specified for the session generates comfort noise from the SID CNG packets
  
From this point, the media encoder specified for the session can be used normally, and outgoing packets can be formatted for transmission either with the DSFormatPacket() API or a user-defined method.

A log file example showing incoming SID packets and buffer output DTX expansion is included in "Packet Stats Logging", below.

If DTX handling is enabled with the SigSRF background process, then the user program does not need to call APIs or make any other intervention.

<a name="VariablePtimes"></a>
## Variable Ptimes

Variable ptimes refers to endpoints that have unequal payload times (ptimes); for example one endpoint might be sending/receiving media every 20 msec and another endpoint every 40 msec.  The mediaTest demo includes examples that match, or "transrate" timing between endpoints with unequal ptimes.

Here are demo command lines that convert incoming pcaps with 20 msec ptime to outgoing pcaps with 40 msec ptime:

```C
./mediaTest -cx86 -M0 -Csession_config/g711_20ptime_g711_40ptime_test_config -ipcaps/pcmutest.pcap -opcmutest_40ptime.pcap -opcmutest_40ptime.wav -L
```

```C
./mediaTest -cx86 -M0 -C session_config/evs_20ptime_g711_40ptime_test_config -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -ovptime_test1.pcap -L
```

For the above command lines, note in the mediaTest displayed statistics counters, the number of transcoded frames is half of the number of buffered / pulled frames, because of the 20 to 40 msec ptime conversion.

Here is a demo command line that converts an incoming pcap with 240 msec ptime to 20 msec:

```C
./mediaTest -cx86 -M0 -Csession_config/evs_240ptime_g711_20ptime_test_config -ipcaps/evs_16khz_16400bps_ptime240_FH_IPv4.pcap -ovptime_test2.pcap -ovptime_test2.wav -L
```

Note however that 240 msec is a very large ptime more suited to unidirectional media streams.  For a bidirectional real-time media stream, for example a 2-way voice conversation, large ptimes would cause excessive delay and difficulty for the endpoints to understand each other.

<a name ="DTMFHandling"></a>
## DTMF Handling

DTMF event handling can be enabled/disabled on per session basis, and is enabled by default (see comments in the above session config file example).  When enabled, DTMF events are interpreted by the Pktlib DSGetOrderedPackets() API according to the format specified in RFC 4733.  Applications can look at the "packet info" returned for each packet and determine if a DTMF event packet is available, and if so call the DSGetDTMFInfo() API to learn the event ID, duration, and volume.  Complete DTMF handling examples are shown in C/C++ source codes included with the demo.

Here is a demo command line that processes a pcap (included in the demo) containing DTMF event packets:

```C
./mediaTest -M0 -cx86 -ipcaps/DtmfRtpEvent.pcap -oout_dtmf.pcap -Csession_config/g711_dtmfevent_config -L
```

A log file example showing incoming DTMF event packets and how they are translated to buffer output packets is included in "Packet Stats Logging", below.

If DTMF handling is enabled with the SigSRF background process, then DTMF events are fully automated and the user program does not need to call APIs or make any other intervention.

<a name="MediaProcessing"></a>
## Media Processing Insertion Point

The mediaTest source codes included with the demo show where to insert signal processing and other algorithms to process media data, after extraction from ordered payloads and/or decoding.  The example source code files perform sampling rate conversion and encoding (depending on session configuration), but other algorithms can also be applied.

Examples of media processing include speech and sound recognition, image analytics, and augmented reality (overlaying information on video data).  Data buffers filled by SigSRF can be handed off to another process, for instance to a Spark process for parsing / formatting of unstructured data and subsequent processing by machine learning libraries.

In the mediaTest source code examples, look for the APIs DSSaveStreamData(), which is used to save ordered / extracted / decoded payload data, and DSGetStreamData(), which is used to retrieve it.  These APIs allow user-defined algorithms to control buffer timing between endpoints, depending on the objective -- minimize latency (real-time applications), maximize bandwidth, match or transrate endpoint timing, or otherwise as needed.

<a name="PacketStatsLogging"></a>
## Packet Stats Logging

mediaTest includes packet statistics logging for:

  * incoming packets (network input, pcap file)
  * jitter buffer output
  * outgoing packets (network output, pcap file)

In the above command lines, the -L entry activates packet logging, with the first output filename found taken as the log filename but replaced with a ".txt" extension.  If -Lxxx is given then xxx becomes the log filename.

Statistics logged include packets dropped, out-of-order (ooo), missing, and duplicated.  Statistics are calculated separately for each SSRC (see Multiple RTP Streams section above), with individual packet entries showing sequence number, timestamp, and type (bitstream payload, DTX, SID, SID CNG, DTMF Event, etc).  Here is a packet stats log file excerpt:

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

As mentioned in "DTMF Handling" above, here is a log file example showing incoming DTMF event packets.  Note that per RFC 4733, one or more packets within the event may have duplicated sequence numbers and timestamps.  The packet logging APIs included with SigSRF can optionally mark these as duplicated if needed.

```CoffeeScript
:
:
Seq num 269              timestamp = 47600, pkt len = 160
Seq num 270              timestamp = 47760, pkt len = 4 (DTMF Event)
Seq num 271              timestamp = 47760, pkt len = 4 (DTMF Event)
Seq num 272              timestamp = 47760, pkt len = 4 (DTMF Event)
Seq num 273              timestamp = 47760, pkt len = 4 (DTMF Event)
Seq num 274              timestamp = 47760, pkt len = 4 (DTMF Event)
Seq num 274              timestamp = 47760, pkt len = 4 (DTMF Event)
Seq num 274              timestamp = 47760, pkt len = 4 (DTMF Event)
Seq num 275              timestamp = 48400, pkt len = 160
:
:
```

Packet stats logging is part of the Diaglib module, which includes several flags (see the diaglib.h header file included with the demo).  Some of the more notable flags include:

  - DS_PKTSTATS_LOG_COLLATE_STREAMS, collate and sort packet logs by RTP stream (i.e. using SSRC values)
  - DS_PKTSTATS_LOG_LIST_ALL_INPUT_PKTS, list all current buffer input entries separately from Diaglib analysis sections
  - DS_PKTSTATS_LOG_LIST_ALL_OUTPUT_PKTS, list all current buffer output entries separately from Diaglib analysis sections


<a name="PktlibandJitterBufferNotes"></a>
## Pktlib and Jitter Buffer Notes

As part of the SigSRF software, with its emphasis on high performance streaming, the Pktlib jitter buffer has several advanced features, including:

* Handles out-of-order packets, including packet swaps
* Accepts incoming packets in real-time or at unlimited rate (i.e. as fast as possible), or a combination
* Maximum buffer depth (or back pressure limit) can be specified on per-session basis
* Dynamic channel creation to support multiple RTP streams per session (see Multiple RTP Streams / RFC 8108 section above)
* Dynamic delay depth adjustment option
* Statistics API, logging, and several options such as overrun control, probation control, flush, and bypass modes

The DS_GETORD_PKT_FTRT flag (in pktlib.h) can be used to pull buffered packets in "faster than real-time" (FTRT) mode.  The packet mode command lines on this page can be used in FTRT mode by adding "-rN", where N is the packet add interval in msec.  For example adding -r0 to the basic packet mode command line above:

```C
./mediaTest -M0 -cx86 -ipcaps/pcmutest.pcap -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -Csession_config/pcap_file_test_config -L -r0
```

specifies a packet add interval of zero, or as fast as possible.  -r10 would specify an add interval of 10 msec, -r5 5 msec, etc.  If no -rN entry is given (the default), then the "ptime" value in the session config definition is used as the add interval (see "Session Configuration File Format" above).

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
7) Transcoding in frame mode tests is not supported yet.


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
