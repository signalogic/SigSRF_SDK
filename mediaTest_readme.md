# mediaMin and mediaTest Reference Apps

After installing the [SigSRF SDK](https://github.com/signalogic/SigSRF_SDK), this page gives example command lines and basic documentation for the mediaMin and mediaTest reference applications, including:

 - packet streaming, both real-time and unlimited rate buffering, with packet re-ordering and packet RFCs

 - test and measurement, including codec audio quality and performance, media RFC verification, and transcoding

 - an example application, including source code, showing how to use Pktlib and Voplib APIs (see architecture diagram on the SigSRF page)

Input and output options include network I/O, pcap file, and audio file format files (raw audio, .au, and .wav); the example command lines below use pcap, wav, and cod (compressed bitstream format) files included with the SigSRF SDK.  SDK capacity is limited to two (2) concurrent transcoding streams, and two (2) concurrent instances (one instance = console window), for a total of four (4) streams. The commercial software has no limitations for concurrency or multiuser, for bare metal, VM, container, or other supported platforms.

# News and Updates

1Q 2021 - encapsulated stream support, tested with OpenLI pcaps containing DER encoded HI3 intercept streams, per ETSI LI and ASN.1 standards

1Q 2021 - real-time ASR option added to mediaMin command line. Kaldi ASR works on stream group outputs, after RTP decoding, stream merging and other signal processing. All codecs supported

1Q 2021 - sdp file option added to mediaMin command line. SDP info can be used to override mediaMin auto-detection or in application-specific cases

4Q 2020 - mediaTest generates encoded pcaps from wav and other audio format files. All codecs supported

2Q 2019: Consolidated <a href="https://bit.ly/2UZXoaW" target="_blank">SigSRF documentation<a> published

Here are some new features added recently:

* USB audio support.  There are some pics below showing the Focusrite 2i2 in action

* Codecs now include EVS, AMR-NB, AMR-WB, AMR-WB+, G729AB, G726, G711, and MELPe (gov/mil standard for 2400, 1200, and 600 bps, also known as STANAG 4591)

* integrated, real-time Kaldi speech recognition (Kaldi guys refer to this as "online decoding")

1Q 2019:  SigSRF software deployed in G7 country equivalent to FBI, providing single server high capacity (500+ concurrent sessions)

3Q 2018:  mediaMin joins mediaTest as a reference / example application, with published soure code.  mediaMin uses a minimal set of SigSRF APIs -- push packet, pull packet, session management -- and dynamic session creation to process pcaps and UDP port data.  Plug in a multistream pcap and decode all streams, handle DTX, merge streams together, generate output pcaps and wav files, and more

1Q 2018:  SigSRF and mediaTest software reached a milestone, now in use or deployed with more than 20 customers.

<a name="DemoLimits"></a>
# Demo Limits

mediaMin and mediaTest demo functionality is limited as follows:

   1) Data limit.  Processing is limited to 3000 frames / payloads of data.  There is no limit on data sources, which include various file types (audio, encoded, pcap), network sockets, and USB audio.

   2) Concurrency limit.  Maximum number of concurrent instances is two and maximum number of channels per instance is 2 (total of 4 concurrent channels).

If you need an evaluation demo with an increased limit for a trial period, [contact us](https://github.com/signalogic/SigSRF_SDK#DocumentationSupport).

# Other Demos

[iaTest Demo (Image Analytics)](https://github.com/signalogic/SigSRF_SDK/blob/master/iaTest_readme.md)

[paTest Demo (Predictive Analytics)](https://github.com/signalogic/SigSRF_SDK/blob/master/paTest_readme.md)

# Table of Contents

[**mediaMin**](#user-content-mediamin)<br/>

&nbsp;&nbsp;&nbsp;[**Real-Time Streaming and Packet Flow**](#user-content-realtimestreaming)</br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Decoding and Transcoding](#user-content-decodingandtranscoding)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Multiple RTP Streams (RFC8108)](#user-content-multiplertpstreamscmdline)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Duplicated RTP Streams (RFC7198)](#user-content-duplicatedrtpstreamscmdline)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Jitter Buffer Control](#user-content-jitterbuffercontrol)<br/>

&nbsp;&nbsp;&nbsp;[**Dynamic Session Creation**](#user-content-dynamicsessioncreation)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[SDP Support](#user-content-sdpsupport)<br/>

&nbsp;&nbsp;&nbsp;[**Stream Groups**](#user-content-streamgroups)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Audio Quality Processing](#user-content-audioqualityprocessing)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Stream Alignment](#user-content-streamalignment)<br/>

&nbsp;&nbsp;&nbsp;[**Encapsulated Streams**](#user-content-encapsulatedstreams)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[OpenLI Support](#user-content-openlisupport)<br/>

&nbsp;&nbsp;&nbsp;[**Static Session Configuration**](#user-content-staticsessionconfig)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Session Endpoint Flow Diagram](#user-content-sessionconfigdiagram)<br/>

[**mediaTest**](#user-content-mediatest)<br/>

&nbsp;&nbsp;&nbsp;[**Codec + Audio Mode**](#user-content-codecaudiomode)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[x86 Codec Testing](#user-content-x86codectesting)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[coCPU Codec Testing](#user-content-cocpucodectesting)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Lab Audio Workstation with USB Audio](#user-content-labaudioworkstation)<br/>
&nbsp;&nbsp;&nbsp;[**Frame Mode**](#user-content-framemode)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Converting Pcaps to Wav and Playing Pcaps](#user-content-convertingpcaps2wav)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[EVS Player](#user-content-evsplayer)</br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[AMR Player](#user-content-amrplayer)</br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Converting Wav to Pcaps](#user-content-convertingwav2pcaps)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[EVS Pcap Generation](#user-content-evspcapgenerator)</br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[AMR Pcap Generation](#user-content-amrpcapgenerator)</br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[DTX Handling](#user-content-dtxhandling)<br/>
&nbsp;&nbsp;&nbsp;[**mediaTest Notes**](#user-content-mediatestnotes)<br/>

[**pktlib**](#user-content-pktlib)<br/>

&nbsp;&nbsp;&nbsp;[**Variable Ptimes**](#user-content-variableptimes)<br/>
&nbsp;&nbsp;&nbsp;[**DTMF Handling**](#user-content-dtmfhandling)<br/>
&nbsp;&nbsp;&nbsp;[**Jitter Buffer**](#user-content-jitterbuffer)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Packet Push Rate Control](#user-content-packetpushratecontrol)<br/>
&nbsp;&nbsp;&nbsp;[**Multiple RTP Streams (RFC8108)**](#user-content-multiplertpstreams)<br/>
&nbsp;&nbsp;&nbsp;[**Duplicated RTP Streams (RFC7198)**](#user-content-duplicatedrtpstreams)<br/>

[**Run-Time Stats**](#user-content-runtimestats)<br/>
[**Event Log**](#user-content-eventlog)<br/>
[**Packet Log**](#user-content-packetlog)<br/>

[**RFCs**](#user-content-supportedrfcs)<br/>
[**User-Defined Signal Processing Insertion Points**](#user-content-userdefinedsignalprocessinginsertionpoints)<br/>
[**3GPP Reference Code Notes**](#user-content-3gppnotes)<br/>
&nbsp;&nbsp;&nbsp;[Using the 3GPP Decoder](#user-content-using3gppdecoder)<br/>
&nbsp;&nbsp;&nbsp;[Verifying an EVS pcap](#user-content-verifyingevspcap)<br/>
[**API Usage**](#user-content-apiusage)<br/>
[**Wireshark Notes**](#user-content-wiresharknotes)<br/>
&nbsp;&nbsp;&nbsp;[Playing Audio in Wireshark](#user-content-playingaudiowireshark)<br/>
&nbsp;&nbsp;&nbsp;[Saving Audio to File in Wireshark](#user-content-savingaudiowireshark)<br/>
[**Audio Quality Notes**](#user-content-audioqualitynotes)<br/>

<a name="mediaMin"></a>
# mediaMin

The mediaMin reference application runs optimized, high-capacity media packet streaming on x86 servers <sup>[1]</sup>, suitable for use on bare-metal, VM, or container platforms. mediaMin reads/writes IP packet streams from/to network interfaces or pcap files (any combination of IPv4 and IPv6), processing multiple concurrent packet streams, with packet handling, DTX, media decoding, stream group processing, and event and packet logging and statistics.

For core functionality, mediaMin utilizes SigSRF libraries, including [pktlib](#user-content-pktlib) (packet handling and high-capacity media/packet thread workers), voplib (voice-over-packet interface), streamlib (streaming media signal processing), and others. Pktlib includes jitter buffer, DTX, SID and media packet re-ordering and repair, packet formatting, and interface to voplib for media decoding and encoding.

To provide a simple, flexible, ready-to-use interface, mediaMin provides a substantial amount of additional functionality. mediaMin command line options instruct pktlib to operate in "analytics mode" (when packet timestamps are missing or problematic), "telecom mode", or a hybrid mode. Typical application examples include SBC transcoding in telecom mode and lawful interception in analytics mode. Signal processing may be applied to stream groups, which can be formed on the basis of input stream grouping or arbitrarily as needed. [Stream Group](#user-content-streamgroups) signal processing includes stream merging, interstream alignment, audio quality enhancement, stream deduplication, speech recognition, and real-time encoded RTP packdet output.

In addition to providing a ready-to-use application, <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaMin/mediaMin.cpp" target="_blank">mediaMin source code</a> demonstrates use of pktlib APIs <sup>[2]</sup> for session creation, packet handling and parsing, packet formatting, jitter buffer, ptime handling (transrating). <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/packet_flow_media_proc.c" target="_blank">Packet/media thread source</a> code used by pktlib is also available to show use of voplib and streamlib APIs <sup>[2]</sup>.  

mediaMin supports [dynamic session creation](#user-content-dynamicsessioncreation), recognizing packet streams with unique combinations of IP/port/payload "on the fly", auto-detecting the codec type, and creating a session to process subsequent packet flow in the stream. [Static session configuration](#user-content-staticsessionconfig) is also supported using parameters in a session config file supplied on the command line.

<sup>1</sup> Capacity figures are spec'ed for Xeon E5 2660 servers running Ubuntu and CentOS, with no add-in hardware. Stress testing includes concurrent session counts up to 50 per x86 core, with sustained test durations over 1400 hrs.</br>
<sup>2</sup> pktlib, voplib, and streamlib are SigSRF library modules, as shown in the <a href="https://github.com/signalogic/SigSRF_SDK#user-content-softwarearchitecturediagram" target="_blank">SigSRF software architecture diagram</a>.

<a name="RealTimeStreaming"></a>
## Real-Time Streaming and Packet Flow Command Line Examples

SigSRF software processes streams from/to network sockets or pcap files, applying required RFCs, media options, and encoding, decoding, or transcoding in real-time (or at a specified rate). Multiple concurrent streams with arbitrary endpoints, RFCs, and media processing requirements are handled and all processing is multithreaded and designed to be scaled up to high capacity, or scaled down to IoT or Edge embedded targets (see [SigSRF Overview](https://github.com/signalogic/SigSRF_SDK#Overview)).

Buffering ("backpressure" in data analytics terminology) is handled using an advanced jitter buffer with several user-controllable options (see [Jitter Buffer](https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#JitterBuffer)).

User-defined media processing can be inserted into packet/media data flow in two (2) places:

> 1) In packet/media thread processing, after decoding, but prior to sampling rate conversion and encoding, inside <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/packet_flow_media_proc.c" target="_blank">packet/media thread source code</a>
> 
> 2) In stream group output processing, inside <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/audio_domain_processing.c" target="_blank">media domain processing source code</a>

See [User-Defined Signal Processing Insertion Points](#user-content-userdefinedsignalprocessinginsertionpoints) below for more information.

<a name="DecodingAndTranscoding"></a>
### Decoding and Transcoding Cmd Line Examples

The mediaMin reference application decodes input packet streams in real-time (or at a specified rate) from network sockets and/or pcap files, and encodes output packet streams to network sockets stream and/or pcap files.  mediaMin relies on the pktlib and streamlib library modules for transcoding and transrating, including mismatched and variable ptimes between endpoints, DTX frames, DTMF events, sampling rate conversion, time-alignment of multiple streams in the same call group, and more. Numerous RFCs are supported (see [RFC List](#user-content-supportedrfcs) on this page), as is intermediate pcap and wav file output from decoded endpoints. A simple command line format includes I/O, operating mode and options, packet and event logging, SDP support, and more.  A static session config file is optional.

Below are some transcoding command line examples. The first command does the following:

* reads IP/UDP/RTP packets from the specified input pcap files
* listens for all UDP ports (on any network interface)
* sends transcoded packets over the network

The second command line is similar, but also does the following:

* writes each output stream to the corresponding output .pcap file given on the command line
* sends over the network any additional streams beyond the number of output files given

```C
./mediaMin -M0 -cx86 -i../pcaps/pcmutest.pcap -i../pcaps/EVS_16khz_13200bps_FH_IPv4.pcap -C../session_config/pcap_file_test_config -L

./mediaMin -M0 -cx86 -i../pcaps/pcmutest.pcap -i../pcaps/EVS_16khz_13200bps_FH_IPv4.pcap -ostream1_xcoded.pcap -ostream2_xcoded.pcap -C../session_config/pcap_file_test_config -L
```

The screencap below shows mediaTest output after the second command line.

![mediaMin pcap I/O command line example](https://github.com/signalogic/SigSRF_SDK/blob/master/images/mediatest_demo_screencap.png?raw=true "mediaMin pcap I/O command line example")

<a name="MultipleRTPStreamsCmdLine"></a>
### Multiple RTP Streams (RFC 8108)

As explained in [Multiple RTP Streams (RFC8108)](#user-content-multiplertpstreams) below, [pktlib](#user-content-pktlib) implements RFC8108, which specifies multiple RTP streams within a session, created and switching based on SSRC transitions. Below are mediaMin command line examples for testing multiple RTP streams:

    ./mediaMin -M0 -cx86 -i../pcaps/mediaplayout_multipleRFC8108withresume_3xEVS_notimestamps.pcapng -L -d0x40c01 -r20
 
    ./mediaMin -M0 -cx86 -i../pcaps/EVS_16khz_13200bps_CH_RFC8108_IPv6.pcap -Csession_config/EVS_16khz_13200bps_CH_RFC8108_IPv6_config -L -d0x40c00

The first command line above uses dynamic session creation, analytics mode, and a 20 msec packet push rate. The second command line uses static session creation, analytics mode, and a "fast as possible" push rate (i.e. no -rN value specified on the command line). Analytics mode is used in both cases because input pcap packet timestamps are incorrect.

Below is a screen capture showing output for the second command line above, with RTP stream transitions highlighted:

![mediaMin multiple RTP streams example](https://github.com/signalogic/SigSRF_SDK/blob/master/images/mediaTest_multiple_ssrc_screencap.png?raw=true "mediaMin multiple RTP streams example")

Packet stats and history log files produced by the above commands (mediaplayout_multipleRFC8108withresume_3xEVS_notimestamps_pkt_log_am.txt and EVS_16khz_13200bps_CH_RFC8108_IPv6_pkt_log_am.txt) show packet history grouped and collated by SSRC, ooo (out-of-order) packets re-ordered in the jitter buffer output section vs. the input section, and SID packet stats (as a result of DTX handling). For a packet log file excerpt, see [Packet Log](#user-content-packetlog) below.

<a name="DuplicatedRTPStreamsCmdLine"></a>
### Duplicated RTP Streams (RFC 7198)

As explained in [Duplicated RTP Streams (RFC7198)](#user-content-duplicatedrtpstreams) below, [pktlib](#user-content-pktlib) implements RFC7198 in order to detect and deal withstreams with packets duplicated for redundancy. Below are mediaMin command line examples included in the SigSRF SDK for RFC7198:

    ./mediaMin -M0 -cx86 -i../pcaps/mediaplayout_RFC7198_EVS.pcapng -L -d0xc11 -r20

    ./mediaMin -M0 -cx86 -i../pcaps/EVS_16khz_13200bps_CH_RFC7198_IPv6.pcap -oEVS_16khz_13200bps_CH_RFC7198_IPv6_g711.pcap -oEVS_16khz_13200bps_CH_RFC7198_IPv6.wav -C../session_config/EVS_16khz_13200bps_CH_RFC7198_IPv6_config -L -d0x40c00

The first command line above uses dynamic session creation, telecom mode, and a 20 msec packet push rate. The second command line uses static session creation, analytics mode, and a "fast as possible" push rate (i.e. no -rN value specified on the command line).

<a name="JitterBufferControl"></a>
### Jitter Buffer Control

Below are mediaMin command line examples showing how to control jitter buffer depth:



See [Jitter Buffer](#user-content-jitterbuffer) below for information on underlying jitter buffer operation and functionality.

<a name="DynamicSessionCreation"></a>
## Dynamic Session Creation

mediaMin supports dynamic session creation, recognizing packet streams with unique combinations of IP/port/payload "on the fly", auto-detecting the codec type, and creating sessions to process subsequent packet flow in each stream it finds. [Static session configuration](#user-content-staticsessionconfig) is also supported using parameters in a session config file supplied on the command line.

In cases where input streams have a definitive end, for instance one or more command line input pcaps, mediaMin will automatically do session cleanup and delete.

<a name="SDPSupport"></a>
### SDP Support

mediaMin supports SDP (<a href="https://en.wikipedia.org/wiki/Session_Description_Protocol" target="_blank">Session Description Portocol</a>) input to moderate dynamic session creation, allowing applications to

> 1) override codec auto-detection
> 2) ignore one or more payload types, in effect ignoring the stream

SDP input can be given as a command line argument with an "-s" option, as shown in the following command line example:

    ./mediaMin -M0 -cx86 -i../pcaps/input.pcapng -L -d0x100c0c01 -r20 -sexample.sdp

or as contents of SIP TCP/IP packets in the incoming packet flow. In the latter case, SDP info should appear before stream(s) start in order to take effect.

```coffeescript
# Example SDP file for use in mediaMin cmd line. Signalogic, Jan2021
#
# mediaMin cmd line syntax:
#
#   -sfilepath.sdp
#
# Notes:
#   1) mediaMin will show messages for payload types found in incoming packet flow when either (i) the payload type is unmatched to a supported codec, or (ii) the payload type is not specified in the SDP file
#      In both cases, one message for each payload type will be shown
#   2) Dynamic payload types must be >= 96 and <= 127, per IETF guidelines (https://tools.ietf.org/id/draft-wu-avtcore-dynamic-pt-usage-02.html). mediaMin will ignore payload type values outside this range
#   3) # can be used as comment delineator. Starting with this symbol, the line, or remainder of the line, is ignored

m=audio 41970 RTP/AVP 96 97 98 99 100
a=rtpmap:96 AMR-WB/16000
a=fmtp:96 mode-set=0,1,2; mode-change-period=2; mode-change-neighbor=1; max-red=0
a=rtpmap:97 AMR/8000
a=fmtp:97 mode-set=0,2,5,7; mode-change-period=2; mode-change-neighbor=1; max-red=0
a=rtpmap:98 AMR/8000
a=fmtp:98 mode-change-period=2; mode-change-neighbor=1; max-red=0
a=rtpmap:99 telephone-event/8000
a=rtpmap:100 telephone-event/16000

# more rtmaps, uncomment to enable

# a=rtpmap:109 EVS/16000
# a=rtpmap:112 AMR-WB/16000
```

Note in the above SDP file example that comments, marked by "#", are supported, although there is no widely accepted method of commenting SDP info mentioned in RFCs or other standards.

<a name="StreamGroups"></a>
## Stream Groups

<a name="AudioQualityProcessing"></a>
### Audio Quality Processing

<a name="StreamAlignment"></a>
### Stream Alignment

<a name="EncapsulatedStreams"></a>
## Encapsulated Streams

mediaMin utilizes the <a href="https://github.com/signalogic/SigSRF_SDK/tree/master/libs/derlib" target="_blank">derlib module</a> to support encapsulated streams, for example HI3 intercept streams with DER encoded contents formatted per ETSI LI and ASN.1 standards. DER encoding stands for Distinguished Encoding Rules, a subset of <a href="https://en.wikipedia.org/wiki/X.690" target="_blank">X.690</a>. DER encoding allows a variety of information to be encapsulated within a TCP/IP stream, including fully formed UDP/IP RTP packets (in HI3 intercept streams these are referred to as CC, or Content of Communication, packets).

<a name="OpenLISupport"></a>
### OpenLI Support

After downloading the SigSRF SDK, below are <a href="https://openli.nz" target="_blank">OpenLI</a> generated encapsulated stream pcap examples you can run:

    ./mediaMin -M0 -cx86 -i../pcaps/openli-voip-example.pcap -L -d0x000c1c01 -r20
 
    ./mediaMin -M0 -cx86 -i../pcaps/openli-voip-example2.pcap -L -d0x000c1c01 -r20

Here are some notes about the above command lines and what to look for after they run:

1) Both examples above contain two (2) G711a streams, but in the second example the first stream generates two (2) child streams (per RFC8108, see [Multiple RTP Streams (RFC8108)](#user-content-multiplertpstreams) above), as highlighted in red in the mediaMin [run-time stats](#user-content-runtimestats) screen capture below. There should be no [stream group](#user-content-streamgroups) output FLC (frame loss concealment), packet logs should be clean, and no warnings or errors in the event log (highlighted in green).

![OpenLI HI3 intercept processing, mediaMin run-time stats](https://github.com/signalogic/SigSRF_SDK/blob/master/images/openli_hi3_intercept_run-time_stats.png?raw=true "OpenLI HI3 intercept processing, mediaMin run-time stats")

2) In the first example, [run-time stats](#user-content-runtimestats) should show a small amount of packet loss (9 packets) in the second stream. The stats should also show these as repaired.

3) In these OpenLI examples, DER encoded packet timestamps do not increment at ptime intervals, so the above mediaMin command lines have "analytics mode" enabled (0xc0000 flags set in the -dN argument). In analytics mode mediaMin uses a queue balancing algorithm and command-specified ptime (the -r20 argument in the above examples) to dynamically determine packet push rates. In both analytics and telecom modes, the pktlib and streamlib modules use RTP timestamps to help with packet repair and interstream alignment.

4) The above mediaMin command lines have DER stream detection enabled (0x1000 flag in the -dN argument).

5) Dynamic session creation is enabled on the mediaMin command line (0x1 flag in the -dN argument); i.e. no static session config file is supplied. When creating sessions dynamically, or "on the fly", mediaMin looks for occurrences of unique IP/port/payload combinations, and auto-detects the codec type. HI3 DER stream detection, dynamic session creation, and codec auto-detection are highlighted in the mediaMin run-time screen capture below.

![OpenLI HI3 intercept stream detection, dynamic session creation, and codec auto-detection](https://github.com/signalogic/SigSRF_SDK/blob/master/images/openli_hi3_intercept_DER_stream_detection_session_creation.png?raw=true "OpenLI HI3 intercept stream detection, dynamic session creation, and codec auto-detection")

6) The above mediaMin command lines have [stream groups](#user-content-streamgroups) enabled (0x400 flag in the -dN argument). Stream group output is formed by combining (or "merging") all input stream contributors, correctly time-aligned, and with audio quality signal proessing applied. ASR (automatic speech recognition) can also be enabled for stream group output.

After loading stream group outputs in Wireshark (openli-voip-example_group0_am.pcap and openli-voip-example2_group0_am.pcap) you should see the following waveform displays:

![OpenLI HI3 intercept processing, stream group output in Wireshark](https://github.com/signalogic/SigSRF_SDK/blob/master/images/openli_hi3_intercept_example_stream_group_output_wireshark.png?raw=true "OpenLI HI3 intercept processing, stream group output in Wireshark")

![OpenLI HI3 intercept processing, stream group output in Wireshark, 2nd example](https://github.com/signalogic/SigSRF_SDK/blob/master/images/openli_hi3_intercept_example2_stream_group_output_wireshark.png?raw=true "OpenLI HI3 intercept processing, stream group output in Wireshark, 2nd example")

In the second example, the child streams contain early media (ring tones), which appear as "rectangular bursts" in the above waveform display. To play stream group output audio click on the :arrow_forward: button.

In the above displays, note the "Max Delta" stat. This is an indicator of both audio quality and real-time performance; any deviation from the specified ptime (in this case 20 msec) is problematic. SigSRF pktlib and streamlib module processing prioritize stability of this metric, as well as accurate time-alignment of individual stream contributors relative to each other.

mediaMin also generates [stream group](#user-content-streamgroups) output .wav files and individual contributor .wav files, which may be needed depending on the application (but should not be used to authenticate audio quality, see [Audio Quality Notes](#user-content-audioqualitynotes) below).

<a name="StaticSessionConfig"></a>
## Static Session Configuration

Static session configuration can be handled programmatically using the DSCreateSession() API, after setting elements of structs defined in shared_include/session.h, or using a session configuration text file to set the struct elements.  The latter method is implemented by mediaTest (see transcoder.c).  For existing sessions, the DSGetSessionInfo() and DSSetSessionInfo() APIs can be used to retrieve and modify session options.

Structs defined in shared_include/session.h include SESSION_DATA, TERMINATION_INFO, voice_attributes, and video_attributes.

Here is a look inside a typical session configuration file, similar to those used in the above command lines:

```CoffeeScript
# Session 1
[start_of_session_data]

term1.remote_ip = d01:5d2::11:123:5201  # IPv6 format
term1.remote_port = 6170
term1.local_ip = fd01:5d2::11:123:5222
term1.local_port = 18446
term1.media_type = "voice"
term1.codec_type = "G711_ULAW"
term1.bitrate = 64000  # in bps
term1.ptime = 20  # in msec
term1.rtp_payload_type = 0
term1.dtmf_type = "NONE"  # "RTP" = handle incoming DTMF event packets for term1 -> term2 direction, forward DTMF events for term2 -> term1 direction
term1.dtmf_payload_type = "NONE"  # A value should be given depending on the dtmf_type field
term1.sample_rate = 8000   # in Hz (note for fixed rate codecs this field is descriptive only)
## term1.dtx_handling = -1  # -1 disables DTX handling

term2.remote_ip = 192.16.0.130  # IPv4 format
term2.remote_port = 10242
term2.local_ip = 192.16.0.16
term2.local_port = 6154
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

# Session 2
[start_of_session_data]

...more session definitions ...

```

Note that each session typically has one or two "terminations", or endpoints (term1 and term2).  A session with only term1 can accept and send streaming data with one endpoint, and perform processing on the data required by the endpoint, by the server running mediaTest, or both.  A session with term1 and term2 can exchange streaming data between endpoints, and perform intermediate processing, such as transcoding, speech recognition, overlaying or adding data to the streams, etc.  The number of sessions defined is limited only by the performance of the platform.

<a name="SessionConfigDiagram"></a>
### Session Endpoint Flow Diagram

As described Session Configuration above, "remote" IP addr and UDP port values refer to stream source, and "local" values refer to stream destination, where a "stream" is a network socket or pcap.  Rx traffic (i.e. received by the user application or mediaTest app) should have destination IP addrs matching local IP addrs and source IP addrs matching remote IP addrs. Tx traffic (i.e. outgoing, or sent by the user application or mediaTest app) will use local IP addrs for source IP addrs and remote IP addrs for destination IP addrs.  Below is a visual explanation:

![session config file and pcap terminology -- remote vs. local, src vs. dest](https://github.com/signalogic/SigSRF_SDK/blob/master/images/session_config_pcap_terminology.png?raw=true "session config file and pcap terminology -- remote vs. local, src vs. dest")

Although terminations can be defined in any order, in general term1 remote should match incoming source values, and term1 local should match incoming destination values. If an outgoing stream is simply a pcap file or a UDP port that nobody is listening to, then term2 values don't have to be anything in particular, they can point to local or non-existing IP addr:port values.

<a name="mediaTest"></a>
# mediaTest

A key objective of mediaTEst is to provide tools to measure encode, decode, and transcoding performance for a wide range of codecs.
  
<a name="CodecAudioMode"></a>
## Codec + Audio Mode

Codec + audio mode allows testing with flexible and interchangeable audio I/O, including:

* codecs

* wav file acquisition, sampling rate conversion, file format conversions, etc

* a wide range of audio I/O, including waveform file formats, compressed bitstream file types, and USB audio

Codec tests perform encode and/or decode with audio I/O and codec type specified on the command line and/or in a codec config file.  The main objectives are to check for bit-exact results, measure audio quality, and measure performance.  Transcoding is not performed in a single command line (although it can be done with successive commands), and pktlib APIs are not used (for real-time streaming packet flow and transcoding, see [mediaMin](#user-content-mediamin) above).

Codec + audio mode supports the following functionality:

* for encoder tests, input can be from waveform file (several types supported) or USB audio. Output can be a "back-to-back feed" to the decoder (for example to test audio quality) or compressed bitstream file.  When possible, compressed bitstream files are saved in a format compatible with 3GPP reference programs (or other reference programs as appropriate for the codec type), in order to interoperate with reference encoders/decoders and allow independent testing and validation

* for decoder tests, input can be from encoder output, or compressed bitstream file. Output can be waveform file or USB audio

* sampling rate, bitrate, DTX control, RF aware enable, number of channels, and other parameters can be specified in a codec configuration file entered on the command line

* pass-thru mode (no codec) allowing raw audio file or USB audio to be converted / saved to wav file.  Sampling rate, number of channels, sample bitwidth, and sample bitwise justification can be specified in the confguration file.  This mode is useful for testing USB audio devices, for example some devices may have limited available sampling rates, 24-bit or 32-bit sample width, or other specs that need SNR and line amplitude testing to determine an optimum set of parameters for compatibility with 16-bit narrowband and wideband codecs

* sampling rate conversion is applied whenever input sampling rate does not match the specified codec (or pass-thru) rate

<a name="x86CodecTesting"></a>
### x86 Codec Testing

The mediaTest codec + audio mode command lines below run on x86 platforms, showing examples of encoding, decoding, and back-to-back encode and decode.

The following command line applies the EVS encoder to a 3GPP reference audio file (WB sampling rate, 13.2 kbps), generating a compressed bitstream file:

```C
./mediaTest -cx86 -itest_files/stv16c.INP -otest_files/stv16c_13200_16kHz_mime.COD -Csession_config/evs_16kHz_13200bps_config
```

To compare with the relevant 3GPP reference bitstream file:

```C
cmp reference_files/stv16c_13200_16kHz_mime_o3.COD test_files/stv16c_13200_16kHz_mime.COD
```

The following command line EVS encodes and then decodes a 3GPP reference audio file (WB sampling rate, 13.2 kbps), producing a .wav file you can listen to and experience EVS audio quality:

```C
./mediaTest -cx86 -itest_files/stv16c.INP -otest_files/stv16c_13200_16kHz_mime.wav 
```

The following command line EVS encodes a 3GPP reference file audio (SWB sampling rate, 13.2 kbps) to a compressed bitstream file:

```C
./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv32c_13200_32kHz_mime.COD -Csession_config/evs_32kHz_13200bps_config
```

To compare with the relevant 3GPP reference bitstream file:

```C
cmp reference_files/stv32c_13200_32kHz_mime_o3.COD test_files/stv32c_13200_32kHz_mime.COD
```

The following command line EVS encodes and then decodes a 3GPP reference bitstream file (SWB sampling rate, 13.2 kbps), producing a .wav file:

```C
./mediaTest -cx86 -itest_files/stv32c.INP -otest_files/stv32c_13200_32kHz_mime.wav -Csession_config/evs_32kHz_13200bps_config
```
<a name="coCPUCodecTesting"></a>
### coCPU Codec Testing

As explained on the SigSRF page, coCPU refers to Texas Instruments, FPGA, neural net, or other non x86 CPUs available in a server, typically on a PCIe card.  coCPUs are typically used to (i) "front" incoming network or USB data and perform real-time, latency-sensitive processing, or (ii) accelerate computationally intensive operations (e.g. convolutions in a deep learning application).

For transcoding, coCPU cores can be used to achieve extremely high capacity per box, for example in applications where power consumption and/or box size is constrained.  The following command lines specify Texas Insstruments c66x coCPU cores <sup>1</sup>.  The first one does the same EVS WB test as above, and the second one does an EVS NB test.  Both produce .wav files that contain a variety of speech, music, and other sounds that demonstrate fidelity and high definition achieved by wideband EVS encoding:

```C
./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv16c.INP -otest_files/c6x16c_j.wav 

./mediaTest -f1000 -m0xff -cSIGC66XX-8 -ecoCPU_c66x.out -itest_files/stv8c.INP -otest_files/c6x8c_j.wav -Csession_config/evs_8kHz_13200bps_config
```
Here are screen caps for the above two mediaTest commands (with frame count and run time highlighted):

![mediaTest display for EVS WB coCPU test](https://github.com/signalogic/SigSRF_SDK/blob/master/images/evs_wb_codec_test_cocpu_screencap.png?raw=true "mediaTest display for EVS WB test using coCPU card")

![mediaTest display for EVS NB coCPU test](https://github.com/signalogic/SigSRF_SDK/blob/master/images/evs_nb_codec_test_cocpu_screencap.png?raw=true "mediaTest display for EVS NB test using coCPU card")

In the above command lines, eight (8) coCPU cores are specified, although the SigSRF SDK demo is limited to one coCPU core per instance.  The coCPU clock rate can be set from 1 to 1.6 GHz (-f1000 to -f1600 in the command line).  Depending on which coCPU card you have, up to 64 coCPU cores can be specified.  Multiple instances of mediaTest can make use of more cards.

Below is a screen capture showing overlay comparison of the NB output with the 3GPP reference waveform:

![EVS NB comparison between coCPU fixed-point and 3GPP reference fixed-point](https://github.com/signalogic/SigSRF_SDK/blob/master/images/EVS_NB_compare_screen_cap.png?raw=true "EVS NB comparison between coCPU fixed-point and 3GPP reference fixed-point")

&nbsp;

![Frequency domain EVS NB comparison between coCPU fixed-point and 3GPP reference fixed-point](https://github.com/signalogic/SigSRF_SDK/blob/master/images/evs_codec_2d_spectrograph.png?raw=true "Frequency domain EVS NB comparison between coCPU fixed-point and 3GPP reference fixed-point")

Note the small differences due to coCPU optimization for high capacity applications.  These differences do not perceptually affect audio quality.  Especially in the frequency domain, differences are very slight and hard to find.  If you look carefully some slight differences can be found at higher frequencies.

<sup>1</sup> For some examples of c66x PCIe cards added to Dell and HP servers, see [this HPC TI wiki page](http://processors.wiki.ti.com/index.php/HPC).

<a name="LabAudioWorkstation"></a>
## Lab Audio Workstation with USB Audio

For professional codec and audio test purposes, below is an image showing a lab audio workstation, configured with:

* Dell R230 1U server (quad-core x86, 8 GB mem, multiple GbE and USB ports).  1U servers are notoriously noisy due to small fan size (higher rpm), but the Dell R230 series has a reputation as a very quiet -- yet high performance -- solution

* Focusrite 2i2 USB audio unit (dual line and/or mic input, dual line out, sampling rates from 44.1 to 192 kHz, 24-bit sample width).  Focusrite also makes quad I/O and other professional units with reasonable pricing

* Ubuntu 16.04 Linux, mediaTest v2.3, and recent versions of pktlib, voplib, diaglib, and aviolib

Also shown for test and demonstration purposes is an HP 33120A function generator, providing reference waveforms and USB audio device calibration.

![Lab Audio Workstation](https://github.com/signalogic/SigSRF_SDK/blob/master/images/lab_audio_workstation_Dell_R230_Focusrite2i2_sm.jpg?raw=true "Lab audio workstation based on Dell R230 1U server and Focusrite 2i2 USB audio unit")

Below is a mediaTest command line testing default capabilities of the USB audio device.  When no config file is specified, the device's default settings are used; for the Focusrite 2i2 this is 44.1 kHz and 2 channel input.
```C
./mediaTest -cx86 -iusb0 -ousb_codec_output.wav
```
The next command line includes a config file to control sampling rate and number of channels for the USB device:
```C
./mediaTest -cx86 -iusb0 -omelp_tst.wav -Csession_config/wav_test_config_melpe
```
Note that various USB devices have different capabilities and options for sampling rate and number of channels.  For example, the Focusrite 2i2 supports four (4) rates from 44.1 to 192 kHz.  In codec + audio mode, mediaTest selects a device rate that is the "nearest integer multiplier" (or nearest integer divisor, or combination of multiplier and divisor) to the test rate, and performs sampling rate conversion as needed.  As one typical example, when testing a narrowband codec (8 kHz sampling rate), mediaTest will select a device rate of 48 kHz, apply lowpass filtering, and then decimate by 1/6.

The mediaTest command lines below show (i) USB audio acquisition of a stereo wav file at 48 kHz, and (ii) processing USB audio with the EVS codec at 16 kHz sampling rate.

    ./mediaTest -cx86 -iusb0 -ousb_test.wav -Csession_config/wav_test_config_48kHz_2chan

    ./mediaTest -cx86 -iusb0 -ousb_codec_output.wav -Csession_config/evs_16kHz_13200bps_config

Below are waveform displays for a 1.5 kHz sine wave from the HP 33120A function generator, sampled by the Focusrite 2i2 at 48 kHz, downsampled to 16 kHz using a voplib API, and run through an EVS 13200 bps encode API:

![USB audio input with codec processing, time domain waveform](https://github.com/signalogic/SigSRF_SDK/blob/master/images/usb_audio_evs_encoder_time_domain.png?raw=true "Time domain waveform showing EVS encoded pure sine wave from an HP 33120A function generator")

![USB audio input with codec processing, freq domain waveform](https://github.com/signalogic/SigSRF_SDK/blob/master/images/usb_audio_evs_encoder_freq_domain.png?raw=true "Frequency domain waveform showing EVS encoded pure sine wave from an HP 33120A function generator")

Note that EVS -- unlike older codecs that rely only on a vocal tract model -- is able to handle a pure tone input.

<a name="FrameMode"></a>
## Frame Mode

Frame mode performs encode, decode, or transcoding based on specifications in a "configuration file" given in the command line (see notes below).  Voplib APIs in mediaTest source code examples include codec instance creation, encode, and decode.  The main objectives are to check for bit-exact results, measure audio quality, and measure basic transcoding performance, including sampling rate conversion.  The following examples use the EVS codec. 

    ./mediaTest -cx86 -M4 -Csession_config/frame_test_config -L

    ./mediaTest -cx86 -M4 -Csession_config/frame_test_config_wav_output -L

Below is a frame mode command line that reads a pcap file and outputs to wav file.  No jitter buffering is done, so any out-of-order packets, DTX packets, or SSRC changes are not handled.  The wav file sampling rate is determined from the session config file.

```C
./mediaTest -M4 -cx86 -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -oevs_16khz_13200bps_FH_IPv4.wav -Csession_config/pcap_file_test_config -L
```
<a name="ConvertingPcaps2Wav"></a>
### Converting Pcaps to Wav and Playing Pcaps

Simple mediaTest command lines can be used to convert Pcaps to wav file, listen to Pcaps over USB audio, or both.

<a name="EVSPlayer"></a>
### EVS Player

The following mediaTest command lines convert EVS pcaps to wav files:

```C
./mediaTest -M0 -cx86 -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -oevs_16khz_13200bps_FH_IPv4.wav -Csession_config/pcap_file_test_config -L

./mediaTest -M0 -cx86 -ipcaps/evs_16khz_13200bps_CH_PT127_IPv4.pcap -oevs_16khz_13200bps_CH_PT127_IPv4.wav -Csession_config/pcap_file_test_config -L
```

The following command line will play an EVS pcap over USB audio:

```C
./mediaTest -M0 -cx86 -ipcaps/evs_16khz_13200bps_FH_IPv4.pcap -ousb0 -Csession_config/pcap_file_test_config -L
```

The above command lines will work on any EVS pcap, including full header, compact header, and multiframe formats.  Combined with the .cod file input described above, this makes mediaTest an "EVS player" that can read pcaps or .cod files (which use MIME "full header" format per 3GPP specs).

In the above USB audio example, output is specified as USB port 0 (the -ousb0 argument).  Other USB ports can be specified, depending on what is physically connected to the server.

Depending on the number of sessions defined in the session config file, multiple inputs and outputs can be entered (session config files are given by the -C cmd line option, see [Static Session Configuration](#user-content-staticsessionconfig) above).

<a name="AMRPlayer"></a>
### AMR Player

The following mediaTest command lines convert AMR pcaps to wav files:

```C
./mediaTest -M0 -cx86 -ipcaps/AMRWB-23.85kbps-20ms_bw.pcap -oamr_WB_23850bps.wav -Csession_config/amrwb_packet_test_config_AMRWB-23.85kbps-20ms_bw -L

./mediaTest -M0 -cx86 -ipcaps/AMR-12.2kbps-20ms_bw.pcap -oevs_16khz_13200bps_CH_PT127_IPv4.wav -Camr_packet_test_config_AMR-12.2kbps-20ms_bw -L
```

The following command line will play an AMR pcap over USB audio:

```C
./mediaTest -M0 -cx86 -ipcaps/AMRWB-23.85kbps-20ms_bw.pcap -ousb0 -Csession_config/amrwb_packet_test_config_AMRWB-23.85kbps-20ms_bw -L
```
The above command lines will work on any AMR pcap, including octet aligned and bandwidth efficient formats.  Combined with the .cod file input described above, this makes mediaTest an "AMR player" that can read pcaps or .cod files (which use MIME "full header" format per 3GPP specs).

In the above USB audio example, output is specified as USB port 0 (the -ousb0 argument).  Other USB ports can be specified, depending on what is physically connected to the server.

<a name="ConvertingWav2Pcaps"></a>
### Converting Wav to Pcaps

Simple mediaTest command lines can be used to convert wav and other audio format files to pcap files.

<a name="EVSPcapGenerator"></a>
### EVS Pcap Generator

The following mediaTest command line converts a wav file to pcap:

    ./mediaTest -M0 -cx86 -itest_files/T018.wav -oasr_test.pcap -Csession_config/evs_16kHz_13200bps_config

A similar command line can be used with other audio format files. The config file allows EVS bitrate, header format, and other options to be specified. mediaTest automatically performs sample rate conversion if the wav file Fs is different than the sample rate specified in the config file.

<a name="AMRPcapGenerator"></a>
### AMR Player

The following mediaTest command line converts a wav file to pcap:

```C
./mediaTest -M0 -cx86 -itest_files/T018.wav -oasr_test.pcap -Csession_config/amrwb_16kHz_12650bps_config
```
<a name="Transcoding"></a>
## Transcoding

mediaTest can perform transcoding by encoding from an audio input (see list above in "Codec + Audio Mode") to a compressed bitstream file, and decoding the bitstream file to an audio output. Two command lines are required, and framesize (e.g. 20 msec, 25 msec, etc) must match between codecs.

The [mediaMin](#user-content-mediamin) section above describes real-time transcoding between input and output packet streams (either socket or pcap I/O).

<a name="mediaTestNotes"></a>
## mediaTest Notes

1) In mediaTest command lines above, input filenames following a naming convention where CH = Compact Header, FH = Full Header, PTnnn = Payload Type nn.  Some filenames contain values indicating sampling rate and bitrate (denoted by NNkhz and NNbps).  Some pcap filenames contain packets organized according to a specific RFC (denoted by RFCnnnn).
2) NB = Narrowband (8 kHz sampling rate), WB = Wideband (16 kHz), SWB = Super Wideband (32 kHz)
3) Comparison results are bit-exact if the cmp command gives no messages
4) .wav files are stored in either 16-bit linear (PCM) format or 8-bit G711 (uLaw) format, depending on the command line specs.  All generated .wav files can be played with Win Media, VLC, or other player
5) Codec compressed bitstreams are stored in files in ".cod" format, with a MIME header and with FH formatted frames (i.e. every frame includes a ToC byte). This format is compatible with 3GPP reference tools, for example you can take a mediaTest generated .cod file and feed it to the 3GPP decoder, and vice versa you can take a 3GPP encoder generated .cod file and feed it to the mediaTest command line.  See examples in the "Using the 3GPP Decoder" section below.
6) session config files (specified by the -C cmd line option), contain codec, sampling rate, bitrate, DTX, ptime, and other options. They may be edited.  See the [Static Session Configuration](#user-content-staticsessionconfig) above.
7) Transcoding in frame mode tests is not yet supported.

<a name="pktlib"></a>
# pktlib

<a name="DTXHandling"></a>
## DTX Handling

DTX (Discontinuous Transmission) handling can be enabled/disabled on per session basis, and is enabled by default (see the above session config file example).  When enabled, each DTX occurrence is expanded to the required duration as follows:

  - the Pktlib DSGetOrderedPackets() API reacts to SID packets emerging from the jitter buffer and inserts SID CNG (comfort noise generation) packets with adjusted timestamps and sequence numbers in the buffer output packet stream
  
  - the media decoder specified for the session generates comfort noise from the SID CNG packets
  
From this point, the media encoder specified for the session can be used normally, and outgoing packets can be formatted for transmission either with the DSFormatPacket() API or a user-defined method.

A log file example showing incoming SID packets and buffer output DTX expansion is included in [Packet Log](#user-content-packetlog) below.

If DTX handling is enabled with the SigSRF background process, then the user program does not need to call APIs or make any other intervention.

<a name="VariablePtimes"></a>
## Variable Ptimes

Variable ptimes refers to endpoints that have unequal payload times (ptimes); for example one endpoint might be sending/receiving media every 20 msec and another endpoint every 40 msec. SigSRF SDK mediaMin reference app examples include command lines that match, or "transrate" timing between endpoints with unequal ptimes.

Here are mediaMin command lines that convert incoming pcaps with 20 msec ptime to outgoing pcaps with 40 msec ptime:

```C
./mediaMin -cx86 -M0 -C../session_config/g711_20ptime_g711_40ptime_test_config -i../pcaps/pcmutest.pcap -opcmutest_40ptime.pcap -opcmutest_40ptime.wav -L
```

```C
./mediaMin -cx86 -M0 -C../session_config/evs_20ptime_g711_40ptime_test_config -i../pcaps/EVS_16khz_13200bps_FH_IPv4.pcap -ovptime_test1.pcap -L
```

For the above command lines, note in the mediaTest displayed statistics counters, the number of transcoded frames is half of the number of buffered / pulled frames, because of the 20 to 40 msec ptime conversion.

Here is a mediaMin command line that converts an incoming pcap with 240 msec ptime to 20 msec:

```C
./mediaMin -cx86 -M0 -C../session_config/evs_240ptime_g711_20ptime_test_config -i../pcaps/evs_16khz_16400bps_ptime240_FH_IPv4.pcap -ovptime_test2.pcap -ovptime_test2.wav -L
```

Note however that 240 msec is a very large ptime more suited to unidirectional media streams. For a bidirectional real-time media stream, for example a 2-way voice conversation, large ptimes would cause excessive delay and intelligibility problems between endpoints.

<a name ="DTMFHandling"></a>
## DTMF Handling

DTMF event handling can be enabled/disabled on per session basis, and is enabled by default (see comments in the above session config file example).  When enabled, DTMF events are interpreted by the Pktlib DSGetOrderedPackets() API according to the format specified in RFC 4733.  Applications can look at the "packet info" returned for each packet and determine if a DTMF event packet is available, and if so call the DSGetDTMFInfo() API to learn the event ID, duration, and volume.  Complete DTMF handling examples are shown in <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/packet_flow_media_proc.c" target="_blank">packet/media thread source code</a>.

Here is a mediaMin command line that processes a pcap containing DTMF event packets:

```C
./mediaMin -M0 -cx86 -i../pcaps/DtmfRtpEvent.pcap -oout_dtmf.pcap -C../session_config/g711_dtmfevent_config -L
```
A log file example showing incoming DTMF event packets and how they are translated to buffer output packets is included in [Packet Log](#user-content-packetlog) below.

If DTMF handling is enabled with the SigSRF background process, then DTMF events are fully automated and the user program does not need to call APIs or make any other intervention.

<a name="JitterBuffer"></a>
## Jitter Buffer

In line with SigSRF's emphasis on high performance streaming, the pktlib library module implements a jitter buffer with advanced features, including:

* Re-ordering of ooo (out-of-order) packets, including an optional automatic "holdoff" feature that dynamically adjusts jitter buffer depth to account for ooo outliers
* Accepts incoming packets in real-time, unlimited rate (i.e. as fast as possible), or user-specified rate
* Maximum buffer depth (or backpressure limit) can be specified on per-session basis
* Dynamic channel creation to support multiple RTP streams per session (see [Multiple RTP Streams (RFC8108)](#user-content-multiplertpstreams) above)
* Dynamic delay and depth adjustment control through APIs and command line options
* Statistics APIs, logging, and several options such as overrun control, probation control, flush, and bypass modes

<a name="PacketPushRateControl"></a>
### Packet Push Rate Control

mediaMin supports a "-rN" command line options to control packet push rate, where N is given in msec. For example typical telecom mode applications might specify -r20 for a 20 msec push rate, which corresponds to a 20 msec ptime (typical for a wide variety of media codecs). But for analytics applications, or for offline purposes (testing, analysis, speech recognition training, measurement, etc), it might be necessary to operate "faster than real-time", or as fast as possible. The command line below includes -r0 (same as no entry) to specify a fast-as-possible push rate:

    ./mediaMin -M0 -cx86 -i../pcaps/pcmutest.pcap -ipcaps/EVS_16khz_13200bps_FH_IPv4.pcap -C../session_config/pcap_file_test_config -L -d0x40c00 -r0

In addition to this level of control, mediaMin also implements an average packet push rate algorithm, which can be applied when pktlib is operating in analytics mode. The average push rate algorithm enable is the 0x80000 flag in the mediaMin -dN command line argument, and analytics mode is the 0x40000 flag.

Note that entering a session configuration file on the command line that contains a "ptime" value, along with no -rN entry, will use the session config ptime value instead (see [Static Session Configuration](#user-content-staticsessionconfig) above).

<a name="MultipleRTPStreams"></a>
## Multiple RTP Streams (RFC 8108)

[pktlib](#user-content-pktlib) implements RFC8108, which although not yet ratified, is widely used to allow multiple RTP streams per session, based on SSRC value transitions. pktlib allows creation of new RTP streams on-the-fly (dynamically) and resumption of prior streams. When pktlib creates a new RTP stream, it also creates new media encoder and decoder instances, in order to maintain separate and contiguous content for each stream. This is particularly important for advanced codecs such as EVS, which depend heavily on prior audio history for RF channel EDAC, noise modeling, and audio classification (e.g. voice vs. music).

<a name="DuplicatedRTPStreams"></a>
## Duplicated RTP Streams (RFC 7198)
 
[pktlib](#user-content-pktlib) implements RFC7198, a method to address packet loss that does not incur unbounded delay, by duplicating packets and sending as separate redundant RTP streams. Pktlib detects and unpacks streams with packets duplicated per RFC7198. [Run-time stats](#user-content-runtimestats) invoked and printed onscreen or in the event log by mediaMin or user-defined apps show duplicated packet counts in the "RFC7198 duplicates" field (under the "Packet Stats" subheading).

<a name="RunTimeStats"></a>
# Run-Time Stats

<a name="EventLog"></a>
# Event Log

The SigSRF <a href="https://github.com/signalogic/SigSRF_SDK/tree/master/libs/diaglib" target="_blank">diaglib library module</a> provides event logging APIs, which are used by the mediaMin and mediaTest reference apps and also available for user-defined applications.

<a name="PacketLog"></a>
# Packet Log

The SigSRF <a href="https://github.com/signalogic/SigSRF_SDK/tree/master/libs/diaglib" target="_blank">diaglib library module</a> provides packet stats and history logging APIs, which are used by the mediaMin and mediaTest reference apps and also available for user-defined applications. Diaglib APIs include packet statistics and history logging for:

  * incoming packets (network input, pcap file)
  * jitter buffer output
  * outgoing packets (network output, pcap file)
  * analysis / comparison between incoming packets and jitter buffer output

In example mediaMin command lines above, the -L entry activates packet logging, with the first output filename found taken as the log filename but replaced with a ".txt" extension.  If -Lxxx is given then xxx becomes the log filename.

Packet statistics logged include packets dropped, out-of-order (ooo), missing, and duplicated. Statistics are calculated separately for each SSRC, with individual packet entries showing sequence number, timestamp, and type (bitstream payload, DTX, SID, SID CNG, DTMF Event, etc).

Below is a packet stats log file excerpt:

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

Packet stats logging is part of the Diaglib module, which includes several flags (see the <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/includes/diaglib.h" target="_blank">diaglib.h header file</a>). Some of the more notable flags include:

  - DS_PKTSTATS_LOG_COLLATE_STREAMS, collate and sort packet logs by RTP stream (i.e. using SSRC values)
  - DS_PKTSTATS_LOG_LIST_ALL_INPUT_PKTS, list all current buffer input entries separately from Diaglib analysis sections
  - DS_PKTSTATS_LOG_LIST_ALL_OUTPUT_PKTS, list all current buffer output entries separately from Diaglib analysis sections

<a name="SupportedRFCs"></a>
## RFCs

Some of the RFCs supported by pktlib and voplib include:

* RFC 3550 (real-time transport protocol)
* RFC 2833 and 4733 (DTMF)
* RFC 4867 (RTP payload and file storage for AMR-NB and AMR-WB codecs)
* RFC 7198 (packet duplication)
* RFC 8108 (multiple RTP streams)
* RFC 3551, 3558, 4788, 5188, 5391, 5993, 6716
* RFC 8130 (payload format for MELPe / STANAG 4591) <sup> 1</sup>
* RFC 4566 (session description protocol)
 
 <sup>1 </sup>In progress, not yet in the SigSRF SDK

<a name="UserDefinedSignalProcessingInsertionPoints"></a>
## User-Defined Signal Processing Insertion Points

Pktlib and streamlib source codes in the SigSRF SDK include "user-defined code insert points" for signal processing and other algorithms to process media data, either or both (i) after extraction from ordered payloads and/or decoding, and (ii) after [stream group](#user-content-streamgroups) processing. For these two (2) locations, the specific source codes are:

> 1) In packet/media thread processing, after decoding, but prior to sampling rate conversion and encoding, inside <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/packet_flow_media_proc.c" target="_blank">packet/media thread source code</a>
> 
> 2) In stream group output processing, inside <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/apps/mediaTest/audio_domain_processing.c" target="_blank">media domain processing source code</a>

The default source codes listed above include sampling rate conversion and encoding (depending on required RTP packet output), speech recognition, stream deduplication, and other processing.

Examples of possible user-defined processing include advanced speech and sound recognition, speaker identification, image analytics, and augmented reality (overlaying information on video data).  Data buffers filled by SigSRF can be handed off to other processes, for instance to a Spark process for parsing / formatting of unstructured data and subsequent processing by machine learning libraries, or to a voice analytics process.  The alglib library contains sampling rate conversion, FFT, convolution, correlation, and other optimized, high performance signal processing functions. Alglib supports both x86 and coCPU&trade; cores, and is used by the [SigDL deep learning framework](https://github.com/signalogic/SigDL).

In addition to the above mentioned In SigSRF source codes, look also for the APIs DSSaveStreamData(), which saves ordered / extracted / decoded payload data, and DSGetStreamData(), which retrieves payload data. These APIs allow user-defined algorithms to control buffer timing between endpoints, depending on application objectives -- minimizing latency (real-time applications), maximizing bandwidth, matching or transrating endpoint timing, or otherwise as needed.

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
<a name="APIUsage"></a>
## API Usage

Below is source code example showing a basic packet processing loop with push/pull APIs.  PushPackets() accepts both IP/UDP/RTP packets and encapsulated TCP/IP streams, for example HI3 intercept streams.

```C
do {

      cur_time = get_time(USE_CLOCK_GETTIME); if (!base_time) base_time = cur_time;
  
   /* if specified, push packets based on arrival time (for pcaps a push happens when elapsed time exceeds the packet's arrival timestamp) */
  
      if (Mode & USE_PACKET_ARRIVAL_TIMES) PushPackets(pkt_in_buf, hSessions, session_data, thread_info[thread_index].nSessionsCreated, cur_time, thread_index);

   /* otherwise we push packets according to a specified interval. Options include (i) pushing packets as fast as possible (-r0 cmd line entry),
      (ii) N msec intervals (cmd line entry -rN),
      (iii) an average push rate based on output queue levels (the latter can be used with pcaps that don't have arrival timestamps) */

      if (cur_time - base_time < interval_count*frameInterval[0]*1000) continue; else interval_count++;  /* if the time interval has elapsed, push and pull packets and increment the interval. Comparison is in usec */

   /* read packets from input flows, push to packet/media threads */

      if (!(Mode & USE_PACKET_ARRIVAL_TIMES)) PushPackets(pkt_in_buf, hSessions, session_data, thread_info[thread_index].nSessionsCreated, cur_time, thread_index);

   /* pull available packets from packet/media threads, write to output flows */

      PullPackets(pkt_out_buf, hSessions, session_data, DS_PULLPACKETS_JITTER_BUFFER, sizeof(pkt_out_buf), thread_index);
      PullPackets(pkt_out_buf, hSessions, session_data, DS_PULLPACKETS_TRANSCODED, sizeof(pkt_out_buf), thread_index);
      PullPackets(pkt_out_buf, hSessions, session_data, DS_PULLPACKETS_STREAM_GROUP, sizeof(pkt_out_buf), thread_index);

   /* check for end of input flows, end of output packet flows sent by packet/media threads, flush sessions if needed */

      FlushCheck(hSessions, cur_time, queue_check_time, thread_index);

      UpdateCounters(cur_time, thread_index);  /* update screen counters */

   } while (!ProcessKeys(hSessions, cur_time, &dbg_cfg, thread_index));  /* handle user input or other loop control */
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

<a name="AudioQualityNotes"></a>
## Audio Quality Notes

1) Wav files are fine for basic audio quality measurements -- amplitude, background noise, intelligibility. However, when authenticating audio quality for a customer application in which it's important to know precisely which audio audio occurred when (for example in lawful intercept applications), wav files, although convenient, should not be used alone. Wav files assume a linear sampling rate and do not encode sampling points, and can thus obscure audio delays/gaps and interstream alignment issues. This is why packet audio should always be included for audio quality authentication purposes. 

2) Audio quality measurement should also include frequency domain analysis, as shown in the examples below taken from a customer case, in which one input stream contained embedded "chime markers" with very specific timing and tonal content.

![Audio quality frequency domain analysis, chime markers, zoom in](https://github.com/signalogic/SigSRF_SDK/blob/master/images/21161-ws_freq_domain_1sec_chimes.png?raw=true "Audio quality frequency domain analysis, chime markers, zoom in")

![Audio quality frequency domain analysis, chime markers](https://github.com/signalogic/SigSRF_SDK/blob/master/images/21161-ws_freq_domain_1sec_overall.png?raw=true "Audio quality frequency domain analysis, chime markers")
