# Table of Contents

[EdgeStream and SigSRF Overview](#user-content-overview)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Applications and Use Cases](#user-content-applications)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Platforms Supported](#user-content-platformssupported)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Telecom Mode](#user-content-telecommode)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Telecom Mode Data Flow Diagram](#user-content-telecommodedataflowdiagram)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Analytics Mode](#user-content-analyticsmode)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Analytics Mode Data Flow Diagram](#user-content-analyticsmodedataflowdiagram)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Minimum API Interface](#user-content-minimumapiinterface)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Stream Groups](#user-content-streamgroups)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Encapsulated Streams](#user-content-encapsulatedstreams)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[HI2/HI3 and OpenLI Support](#user-content-openlisupport)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[High Capacity Multithreaded Operation](#user-content-multithreaded)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Deployment Grade](#user-content-deploymentgrade)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Software and I/O Architecture Diagram](#user-content-softwarearchitecturediagram)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Packet and Media Processing Data Flow Diagram](#user-content-packetmediathreaddataflowdiagram)<br/>
[Using the SDK - Run Demos and Reference Apps, Build User Apps](#user-content-sdkdownload)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Docker Containers](#user-content-dockercontainers)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[ASR Docker Container](#user-content-asrdockercontainer)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Rar Packages](#user-content-rarpackages)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Install Notes](#user-content-installnotes)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[ASR Install Notes](#user-content-asrinstallnotes)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Sudo Privilege](#user-content-sudoprivilege)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Running the Install Script](#user-content-runningtheinstallscript)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Test File Notes](#user-content-testfilenotes)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[ASR Notes](#user-content-asrnotes)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Run Notes](#user-content-runnotes)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[mediaMin and mediaTest (streaming media, transcoding, speech recognition, waveform file and USB audio processing, and more)](#user-content-mediamin_and_mediatest)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[iaTest (image analytics)](#user-content-iatest)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[paTest (predictive analytics from log data)](#user-content-patest)<br/>
[SDK / Demo Tested Platforms](#user-content-sdkdemotestedplatforms)<br/>
[Documentation, Support, and Contact](#user-content-documentationsupport)<br/>

<a name="Overview"></a>
# EdgeStream™ and SigSRF Overview

SigSRF is a series of modules (libraries) and [EdgeStream™](https://signalogic.com/edgestream) is a combination of SigSRF and applications that use SigSRF APIs. SRF stands for Streaming Resourcing Functions.

The combined EdgeStream + SigSRF SDK introduces a scalable approach to telecom, media, HPC, and AI servers. The basic concept is to scale between cloud, private cloud, and Edge and IoT servers, while maintaining a container programming model. Third-party source code is minimized and all modules are scanned and tested continuously for security vulnerabilities. 

The primary objectives of EdgeStream and SigSRF software are:

* provide high performance software modules for telecom, media, AI (deep learning), and analytics streaming applications
* provide both telecom and analytics modes for (i) telecom and CDN applications and (ii) data analytics and web IT applications
* maintain a deployment grade solution.  All modules and sources have been through customer acceptance testing
* scale up without GPU if needed, and provide high capacity, "real-time at scale" streaming and processing
* scale down without ARM if needed, and provide IoT and Edge solutions for SWaP <sup>1</sup> constrained applications
* maintain full program compatibility with containerized operation, including open source software support, server architectures, latest programming languages, etc.

<a name="Applications"></a>
## Applications and Use Cases

EdgeStream and SigSRF software are currently deployed in the following cloud and edge application areas:

### Cloud Use Cases
* Audio Quality Optimizer
* Lawful Intercept and Lawful Intelligence (LI)
* Call Recording, Call Transcription, Multiparty Call Reconstruction
* Session Border Controller (SBC) and Media Gateway
* Bulk Pcap Processing
* RTP Decoder (<a href="https://www.signalogic.com/evs_codec.html" target=_blank>EVS</a>, AMR, G729, MELPe, etc)
* RTP Malware Detection
* Network Analyzers

### Edge Use Cases
* Robotics -- implement voice commands (ASR, automatic speech recognition) (<a href="https://www.signalogic.com/RobotHPC" target=_blank> RobotHPC™ combined hw + sw product page</a>
* Factory, construction, warehouse equipment - implement accident avoidance for remotely controlled and automated equipment
* Vehicle automation - implement small server add-on / after-market products such as small form-factor Lidar
* Satcom and HF Radio Speech Compression
* R&D Labs and Workstations

<a name="PlatformsSupported"></a>
## Platforms Supported

EdgeStream and SigSRF software are designed to run on (i) private, public, or hybrid cloud servers and (ii) embedded system servers.  Reference SDK applications have low resource and footprint profiles and are intended to run on any Linux server based on x86, ARM, and PowerPC, and on form-factors as small as mini- and micro-ITX.

EdgeStream and SigSRF support media delivery, transcoding, deep learning <sup>1</sup>, OpenCV, speech recognition, and other calculation / data intensive applications.  High capacity operation exceeding 2000 concurrent sessions is possible on multicore x86 servers.  The High Capacity Operation section in [SigSRF Documentation](#user-content-documentationsupport) has information on thread affinity, htop verification, Linux guidelines, etc.

For applications facing SWaP <sup>2</sup>, latency, or bandwidth constraints, SigSRF software supports a wide range of coCPU&trade; and SoC embedded device targets while maintaining a cloud compatible software architecture, for an overview see <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/WhenSoftwareOnlyIsNotEnough.md">When Software Only Is Not Enough</a>.

<sup>1</sup> In progress<br/>
<sup>2</sup> SWaP = size, weight, and power consumption

<a name="TelecomMode"></a>
## Telecom Mode

Telecom mode is defined as direct handling of IP/UDP/RTP traffic.  This mode is sometimes also referred to as “clocked” mode, as a wall clock reference is required for correct jitter buffer operation.  Examples of telecom mode applications include network midpoints such as SBC (Session Border Controller) and media gateway, and endpoints such as handsets and softphones.  Typically telecom applications have hard requirements for real-time performance and latency.

<a name="TelecomModeDataFlowDiagram"></a>
### Telecom Mode Data Flow Diagram

![SigSRF software telecom mode data flow diagram](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Streaming_packet_and_media_processing_data_flow_telecom_mode_RevA9b.png?raw=true "SigSRF telecom mode data flow diagram")

<a name="AnalyticsMode"></a>
## Analytics Mode

Analytics mode is defined as indirect handling of IP/UDP/RTP traffic, where traffic is encapsulated or "one step removed", having been captured, copied, or relayed from direct traffic for additional processing.  This mode is sometimes also referred to as data driven or “clockless” mode, the latter description referring to jitter buffer packet processing either wholly or partially without a wall clock reference.  In general, analytics mode applications operate after real-time traffic has already occurred, although it may be incorrect to say "non-real-time" as they may need to reproduce or emulate the original real-time behavior.  Examples of analytics mode include Lawful Intercept (LI) and web IT data analytics such as speaker identification and automatic speech recognition (ASR).

Applications include user-defined apps and the mediaMin and mediaTest reference apps described on the <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md" target="_blank">mediaMin / mediaTest page</a>.

<a name="AnalyticsModeDataFlowDiagram"></a>
### Analytics Mode Data Flow Diagram

![SigSRF software analytics mode data flow diagram](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Streaming_packet_and_media_processing_data_flow_analytics_mode_RevA9b.png?raw=true "SigSRF analytics mode data flow diagram")

Applications include user-defined apps and the mediaMin and mediaTest reference apps described on the <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md" target="_blank">mediaMin / mediaTest page</a>.

<a name="MinimumAPIInterface"></a>
## Minimum API Interface

At a high-level the SigSRF API allows simple interfaces, for example a basic, continuous push-pull loop using bare minimum APIs, as shown in the <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md" target="_blank">mediaMin</a> source code excerpt below:
  
![SigSRF minimum API interface](https://github.com/signalogic/SigSRF_SDK/blob/master/images/minimum_api_interface_source_code_excerpt.png?raw=true "Minimum API interface")

Some notes about the above example:

> 1. PushPackets() and PullPackets() call [pktlib](https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-pktlib) APIs DSPushPackets() and DSPullPackets()
> 2. Sessions are created dynamically inside PushPackets()
> 3. PullPackets() saves (i) de-jittered and repaired packet streams and (ii) transcoded streams to local pcap files, and writes continuous merged audio to pcap or UDP port

<a name="StreamGroups"></a>
## Stream Groups

SigSRF supports the concept of "stream groups", allowing multiple streams to be grouped together for additional processing.  Examples including merging conversations for Lawful Intercept applications, conferencing, multiparty call reconstruction, and identifying and tagging different individuals in a conversation (sometimes referred to as "diarization").

<a name="EncapsulatedStreams"></a>
## Encapsulated Streams

SigSRF supports encapsulated streams, specifically ASN.1 DER encoded HI2 and HI3 intercept streams. Additional encapsulated stream support is planned.

For information on DER decoding library API functions, see derlib.h in the SigSRF_SDK/libs/derlib folder.

<a name="OpenLISupport"></a>
### HI2/HI3 and OpenLI Support

For information on HI2 and HI3 intercept decoding with <a href="https://openli.nz" target="_blank">OpenLI</a> example pcaps, see the <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-encapsulatedstreams">Encapsulated Streams section</a> on the <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md">mediaMin and mediaTest page</a>.

<a name="Multithreaded"></a>
## High Capacity Multithreaded Operation

Both SigSRF library modules and EdgeStream applications support multiple, concurrent packet + media processing threads. Session-to-thread allocation modes include linear, round-robin, and "whole group" in the case of stream groups. Thread stats include profiling, performance, and session allocation. Threads support an optional "energy saver" mode, after a specified amount of inactivity time. The [SigSRF packet media thread data flow diagram](#user-content-packetmediathreaddataflowdiagram) below shows per thread data flow.

High capacity operation exceeding 2000 concurrent sessions is possible on multicore x86 servers. The High Capacity Operation section in [SigSRF Documentation](#user-content-documentationsupport) has information on thread affinity, htop measurement and verification, Linux guidelines, etc.

Below is a Linux htop measurement showing SigSRF software high capacity operation with (i) multiple media/packet threads, and (ii) multiple application threads:

![SigSRF software high capacity operation](https://github.com/signalogic/SigSRF_SDK/blob/master/images/media_packet_thread_high_capacity_operation.png?raw=true "SigSRF software high capacity operation")

The measurement was taken on an HP DL380 server with 16 (sixteen) E5-2660 v0 cores, 8 physical cores per CPU, and 2 (two) CPUs) each with 16 GB of RAM. Some key aspects include:

  - **Workload** Dynamic session creation and management, RTP packet media decoding, all packet processing options, partial media stream processing options (including RTP stream merging), and real-time pcap and wav file generation are enabled. ASR (automatic speech recognition) is not enabled, as it has a substantial impact on capacity (see [ASR Notes](#user-content-asrnotes) below). Streams contain a mix of EVS and AMR-WB codecs
  
  - **Thread allocation ratio** Media/packet worker threads and application threads are allocated in a 5:3 ratio. Application threads are responsible for UDP port monitoring and I/O, pcap and wav file I/O, session management, and APIs interfacing to packet/media threads <sup>1</sup>
  
  - **Thread distribution** htop shows 10 packet/media threads and 12 mediaTest application threads, and the mediaTest command line shows each application thread reusing inputs 13 times. The 2922.0 pcap shown in the command line contains 3 streams, resulting in 504 total sessions <sup>2</sup> (12\*3\*(1+13)) and 168 total stream groups. As noted in **Workload**, streams contain a mix of EVS and AMR-WB codecs

  - **Hyperthreading** Hyperthreading for media + packet threads is effectively disabled through use of core affinity (aka CPU pinning). Application threads continue to utilize hyperthreading

Using the thread ratio and per stream workload given above, necessary per CPU amount of RAM and number cores can be estimated as:

![server capacity equations](https://github.com/signalogic/SigSRF_SDK/blob/master/images/server_capacity_math_equations.png?raw=true "server capacity math equations")

where N is the required number of concurrent streams, numCPUs is the number of available CPUs <sup>3</sup>, and memSize is RAM in GB. For example, a dual-socket server processing 2000 concurrent streams needs 64 GB RAM and 32 cores per CPU. For applications with server memory or core constraints, custom builds are possible to achieve tradeoffs between capacity and functionality.

<sup>1</sup> [pktlib](https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-pktlib) provides packet/media APIs, examples include DSCreateSession(), DSPushPacket(), and DSPullPackets(), and DSGetSessionInfo()
  
<sup>2</sup> [Sessions](https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-sessions) create unique stream identifiers (from IP header, UDP port, and SSRC information) allowing management of a stream during its lifespan

<sup>3</sup> Available CPUs can be located in one or more servers

<a name="DeploymentGrade"></a>
## Deployment Grade

EdgeStream and SigSRF software are currently deployed by major carriers, LEAs, research organizations, and B2B enterprises. Under NDA, and with end customer permission, it may be possible to provide more information on deployment use cases and/or locations.

EdgeStream and SigSRF software, unlike many open source repositories, are not experimental or prototype. Some of the signal processing modules have deployment histories dating back to 2005, including telecom, communications, and aviation systems. Packet processing modules include some components dating back to 2010, such as jitter buffer and some voice codecs.  The origins of SigSRF software are in telecom system deployment, with emphasis in the last few years on deep learning. Both EdgeStream and SigSRF continuously undergo rigorous customer production testing. 

For calculation-intensive shared library components, such as codecs, signal processing, and inference, SigSRF implements the XDAIS standard made popular by Texas Instruments.  XDAIS was designed to manage shared resources and conflict between calculation- and memory-intensive algorithms.  Originally XDAIS was intended by TI to help produce robust, reliable software on highly resource-constrained embedded platforms.  It continues to help achieve that on today's modern Linux servers.

In addition to customer production testing, stress tests are always ongoing in Signalogic lab servers.  New releases must pass 672 hours (4 weeks) of continuous stress test at full capacity, running on HP DL380 series servers.  For more information on these tests, and Linux configuration used for high capacity operation, see [SigSRF Documentation](#user-content-documentationsupport) below.

<a name="SoftwareArchitectureDiagram"></a>
## SigSRF Software and I/O Architecture Diagram

Below is a SigSRF software and I/O architecture diagram.

![SigSRF software and streaming I/O architecture diagram](https://github.com/signalogic/SigSRF_SDK/blob/master/images/SigSRF_Software_Architecture_and_Packet_IO_RevA2.png?raw=true "SigSRF software and streaming I/O architecture diagram")

<a name="PacketMediaThreadDataFlowDiagram"></a>
## SigSRF Packet and Media Processing Data Flow Diagram

Below is a SigSRF software streaming packet and media processing data flow diagram.  This is an expansion of the telecom mode and analytics mode data flow diagrams above, including shared library APIs used within a packet/media thread.

In addition to the APIs referenced below, SigSRF offers a simplified set of APIs that minimize user applications to session create/delete and packet push/pull.  mediaMin and mediaTest are the reference applications for the minimum API level and more detailed level, respectively.  Source code is published for both.

![SigSRF streaming packet and media processing data flow diagram](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Streaming_packet_and_media_processing_data_flow_RevA4.png?raw=true "SigSRF streaming packet and media processing data flow diagram")

Some notes about the above data flow diagram:

   1) Data flow matches <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md">mediaTest</a> application C source code (packet_flow_media_proc.c).  Subroutine symbols are labeled with pktlib, voplib, and alglib API names.

   2) A few areas of the flow diagram are somewhat approximated, to simplify and make easier to read.  For example, loops do not have "for" or "while" flow symbols, and some APIs, such as DSCodecEncode() and DSFormatPacket(), appear in the flow once, but actually may be called multiple times, depending on what signal processing algorithms are in effect.

   3) <b>Multisession</b>.  The "Input and Packet Buffering", "Packet Processing", and "Media Processing and Output" stages are per-session, and repeat for multiple sessions.  See <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md#user-content-sessionconfig">Session Config</a> for more info.

   4) <b>Multichannel</b>.  For each session, The "Input and Packet Buffering", "Packet Processing", and "Media Processing and Output" stages of data flow are multichannel and optimized for high capacity channel processing.

   5) <b>Multithreaded</b>.  A copy of the above diagram runs per thread, with each thread typically consuming one CPU core in high performance applications.

   6) <b>Media signal processing and inference</b>.  The second orange vertical line divides the "packet domain" and "media domain".  DSStoreStreamData() and DSGetStreamData() decouple these domains in the case of unequal ptimes.  The media domain contains raw audio or video data, which allows signal processing operations, such as sample rate conversion, conferencing, filtering, echo cancellation, convolutional neural network (CNN) classification, etc. to be performed.  Also this is where image and voice analytics takes place, for instance by handing video and audio data off to another process.

<a name="SDKDownload"></a>
# Using the SDK - Run Demos and Reference Apps, Build User Apps

There are two (2) options to run and test the EdgeStream and SigSRF SDK (i) Docker containers, or (ii) download a RAR package and install script. Option (i) is fastest and easiest, option (ii) will give precise performance results for specific VM or bare-metal configurations. See the [Docker Containers](#user-content-dockercontainers) and [Rar Packages](#user-content-rarpackages) sections below.

The SDK contains:

   1) A limited eval / reference version of SigSRF libraries and EdgeStream applications, including media packet streaming and decoding, media transcoding, image analytics, and H.264 video streaming (ffmpeg acceleration).  For notes on reference application limits, see [Run Notes](#user-content-RunNotes) below.

   2) Makefiles and C/C++ source code for
   &nbsp;&nbsp;&nbsp;&nbsp;<ul>
     <li>media/packet real-time threads, including API usage for packet queue receive/send, jitter buffer add/retrieve, codec decode/encode, stream group processing, and packet diagnostics</li>
     <li>reference applications, including API usage for session create/modify/delete, packet push/pull, and event and packet logging. Also includes static and dynamic session creation, RTP stream auto-detect, packet pcap and UDP input</li>
     <li>stream group output audio processing, user-defined signal processing</li>
   </ul>
 
   3) Concurrency examples, including stream, instance, and multiple user
   
All reference applications and demos were built and tested on x86 servers with a range of Ubuntu and CentOS distributions and g++/gcc tools.

For servers augmented with a coCPU card, the mediaTest, mediaMin, and iaTest reference apps will utilize coCPU cards if found at run-time (coCPU drivers and libs are included in SDK .rar files).  Example coCPU cards are <a href="http://processors.wiki.ti.com/index.php/HPC" target="_blank">shown here</a>, and can be obtained from TI, Advantech, or Signalogic.

<a name="DockerContainers"></a>
## Docker Containers

Ubuntu and CentOS docker containers with EdgeStream and SigSRF SDK and demos installed and ready to run are located at:

    https://hub.docker.com/r/signalogic/sigsrf_sdk_demo_ubuntu

    https://hub.docker.com/r/signalogic/sigsrf_sdk_demo_centos
    
    https://hub.docker.com/r/signalogic/sigsrf_sdk_demo_ubuntu_asr

After pulling the container, use the following run command:

    docker run -it --cap-add=sys_nice signalogic/sigsrf_sdk_demo_ubuntu /bin/bash
    
or

    docker run -it --cap-add=sys_nice signalogic/sigsrf_sdk_demo_centos /bin/bash
    
 After running the container, change your current folder ...

    cd /home/sigsrf_sdk_demo/Signalogic/apps/mediaTest

or

    cd /home/sigsrf_sdk_demo/Signalogic/apps/mediaTest/mediaMin

... depending on which EdgeStream app you want to run. The <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md">mediaMin and mediaTest page</a> gives example command lines for streaming media, transcoding, speech recognition, waveform file and USB audio processing, and more.

<a name="ASRDockerContainer"></a>
### ASR Docker Container

The ASR (automatic speech recognition) container is larger than other containers, but otherwise the procedure is the same as in [Docker Containers](#user-content=dockercontainers) above. For ASR specific info, see [ASR Notes](#user-content-asrnotes) below.

<a name="GeneralDockerContainerNotes"></a>
### General Docker Container Notes

1. All available containers have been tested on x86 Linux bare metal servers, including performance and capacity measurements given on the <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md">mediaMin and mediaTest page</a>. Measurements on x86 VMs on other OS (e.g MacOS, WinXX) are likely to be slower.

2. All available containers are configured for root privileges which makes modifying and rebuilding EdgeStream applications and test and measurement file transfers easier.

3. App performance inside containers can be slightly improved by adding the "privileged" flag to the run command, for example:

    docker run -it --cap-add=sys_nice --privileged signalogic/sigsrf_sdk_demo_ubuntu /bin/bash
 
  This has more of an impact on slower CPUs (e.g. Atom), for instance it might make packet output intervals slightly more consistent.

4. Rar packages are included in each container, if for any reason it should be needed to re-run the SDK/demo install from scratch.

<a name="WinSCPAccesDockerContainers"></a>
### WinSCP Access to Docker Containers

You may want to remotely access inside a running/active container pcap and media files output by EdgeStream apps -- for example to run WireShark or a media player (for more information on pcap, waveform, event log, packet log, and other test files provided/generated using the SDK, see [Test File Notes](#user-content-testfilenotes) below). This can be convenient for test and measurement purposes, as it avoids manual copying of files between host and container.

To do this with WinSCP, you can set up a second host SSH port NNNN and then add:

    -p NNNN:8080
  
to your docker run command. Per Docker documentation this "creates a firewall rule which maps a container port to a port on the Docker host to the outside world", i.e. remote access over port NNNN is relayed to port 8080 on the "bridge network" that Docker maintains for the container made active by the run command. In this way, you can set up two (2) WinSCP remote sessions, one for the host and one for the container under test.

As an additional note, there is online forum discussion about WinSCP-ing into a container with the following steps:

    docker-machine ls

    NAME ACTIVE DRIVER STATE URL SWARM DOCKER ERRORS
    manager - hyperv Running tcp://192.168.111.63:2376 v1.13.0
    worker - hyperv Running tcp://192.168.111.64:2376 v1.13.0

and then using the reported IP and port info in the remote WinSCP client, along with userid docker, and password tcuser. However, this only works when Docker is running inside a VM, for example on a WinXX or MacOS host.

## Ubuntu Docker Container Notes

1) Ubuntu distro is 20.04.2, with gcc/g++ 11.2.0 installed
2) wget and unrar are installed, along with a symlink /usr/bin/g++ pointing to /gcc/bin/g++
3) The base Ubuntu container comes from a repository hosted by user "gmao", who is associated with NASA, as shown below:

    https://hub.docker.com/u/gmao

<img src="https://github.com/signalogic/SigSRF_SDK/blob/master/images/ubuntu_base_container_docker_hub_profile.png" width=50% alt="Ubuntu base container Docker hub profile info" title="Ubuntu base container Docker hub profile info"></br>

## CentOS Docker Container Notes

1) CentOS distro is 8.2.2004, with gcc/g++ 8.3.1-5 installed
2) wget and unrar are installed, along with symlink /usr/bin/g++ pointing to /gcc/bin/g++
3) The base CentOS container comes from a repository hosted by user "images4dev", as shown below:

    https://hub.docker.com/u/images4dev

<img src="https://github.com/signalogic/SigSRF_SDK/blob/master/images/centos_base_container_docker_hub_profile.png" width=50% alt="CentOS base container Docker hub profile info" title="CentOS base container Docker hub profile info"></br>

<a name="RarPackages"></a>
## Rar Packages

As an alternative to Docker containers, Ubuntu and CentOS RAR packages are available to install SigSRF and EdgeStream software on VMs or bare-metal servers. The install script auto-checks kernel version and Linux distro version to decide which .rar file to look for. If you have downloaded more than one RAR package, for example you are upgrading to a newer SDK version, the install script will install the most recent .rar file.

<a name="InstallNotes"></a>
### Install Notes

To download the install script and one or more rar files directly from Github (i.e. without checking out a clone repository), use the following commands:

Install script
```
wget https://raw.githubusercontent.com/signalogic/SigSRF_SDK/master/rar_packages/autoInstall_SigSRF_SDK_2022v7.sh -O- | tr -d '\r' > autoInstall_SigSRF_SDK_2022v7.sh
```
Ubuntu and Debian .rar
```
wget https://raw.githubusercontent.com/signalogic/SigSRF_SDK/master/rar_packages/Signalogic_sw_host_SigSRF_SDK_Ubuntu12.04-22.04_15May23.rar --content-disposition -O Signalogic_sw_host_SigSRF_SDK_Ubuntu12.04-22.04_27Apr24.rar
```
CentOS .rar
```
wget https://raw.githubusercontent.com/signalogic/SigSRF_SDK/master/rar_packages/Signalogic_sw_host_SigSRF_SDK_CentOS6-8_15May23.rar --content-disposition -O Signalogic_sw_host_SigSRF_SDK_CentOS6-8_27Apr24.rar
```
For the ASR version of the SDK, the following multi-line command can be used:
```
wget https://raw.githubusercontent.com/signalogic/SigSRF_SDK/master/rar_packages/Signalogic_sw_host_SigSRF_SDK_Ubuntu12.04-22.04_15May23.rar --content-disposition -O Signalogic_sw_host_SigSRF_SDK_ASR_Ubuntu12.04-20.04_2Apr22.part01.rar; \
wget https://raw.githubusercontent.com/signalogic/SigSRF_SDK/master/rar_packages/Signalogic_sw_host_SigSRF_SDK_Ubuntu12.04-22.04_15May23.rar --content-disposition -O Signalogic_sw_host_SigSRF_SDK_ASR_Ubuntu12.04-20.04_2Apr22.part02.rar; \
wget https://raw.githubusercontent.com/signalogic/SigSRF_SDK/master/rar_packages/Signalogic_sw_host_SigSRF_SDK_Ubuntu12.04-22.04_15May23.rar --content-disposition -O Signalogic_sw_host_SigSRF_SDK_ASR_Ubuntu12.04-20.04_2Apr22.part03.rar; \
wget https://raw.githubusercontent.com/signalogic/SigSRF_SDK/master/rar_packages/Signalogic_sw_host_SigSRF_SDK_Ubuntu12.04-22.04_15May23.rar --content-disposition -O Signalogic_sw_host_SigSRF_SDK_ASR_Ubuntu12.04-20.04_2Apr22.part04.rar
```
The ASR version is separated into .partN.rar files because the overall .rar file size is substantially larger (approx 270 MB vs 65 MB), and Github has a 100MB per file limit. See [ASR Install Notes](#user-content-asrinstallnotes) below for details about the ASR version SDK.

<i><b>Important Note 1:</b> all .rar files and the install script should be downloaded to the same folder. The actual install folder can be different, as the install script prompts for an install path (see below).</i>

<i><b>Important Note 2:</b> the install script contains Windows CR/LF line endings. The above wget command automatically converts line-endings to Linux. If you download the script manually then you will need to convert line endings, for example: echo "$(tr -d '\r' < autoInstall_SigSRF_SDK_2022v7.sh)" > autoInstall_SigSRF_SDK_2022v7.sh.</i>

Note that the install script checks for the presence of the unrar package, and if not found attempts to install it; if this happens there may be some additional prompts depending on the Linux version.

For more information on pcap, waveform, and other test files provided with the SDK, see [Test File Notes](#user-content-testfilenotes) below.
 
<a name="ASRInstallNotes"></a>
### ASR Install Notes

To install the ASR RAR package, first follow the instructions in [Rar Packages](#user-content-rarpackages) above, and then in [Running the Install Script](#user-content-runningtheinstallscript), below. The install procedure is the same as the standard SDK version, except you should choose item "2) Install EdgeStream and SigSRF Software with ASR Option" instead of item 1).

Note that downloading the ASR .rar files takes longer as the .rar size is  substantially larger. Also the install itself takes a little longer.
    
<a name="SudoPrivilege"></a>
### Sudo Privilege

<i>Running the install script requires being logged in as root or as a user with sudo root privilege.</i>  In Ubuntu, allowing a user sudo root privilege can be done by adding the user to the “administrators” group (<a href="http://askubuntu.com/questions/168280/how#do#i#grant#sudo#privileges#to#an#existing#user" target="_blank">as shown here</a>). In CentOS a user can be added to the “/etc/sudoers” file (<a href="https://wiki.centos.org/TipsAndTricks/BecomingRoot" target="_blank">as shown here</a>).  Please make sure this is the case before running the script.

<a name="RunningTheInstallScript"></a>
### Running the Install Script

To run the install script enter:

    source autoInstall_SigSRF_SDK_2022v7.sh

The script will then prompt as follows:

    1) Host
    2) VM
    Please select platform for SigSRF software install [1-2]:

Host is the default. VM should only be selected when (i) platform acceleration is in use, for example a coCPU card, DPDK, GPU board inference, etc. <sup>[1]</sup> and (ii) you are not running in a container. Host is valid for containers, VMs, bare-metal, or cloud compute instances. After choosing a platform, the script will next prompt for an install option:

    1) Install EdgeStream and SigSRF Software
    2) Install EdgeStream and SigSRF Software with ASR Option
    3) Install EdgeStream and SigSRF Software with coCPU Option
    4) Uninstall EdgeStream and SigSRF Software
    5) Check / Verify EdgeStream and SigSRF Software Install
    6) Exit
    Please select install operation to perform [1-6]:

If operations 1) thru 3) are selected, the script will prompt for an install path:

    Enter the path for EdgeStream and SigSRF software installation:

If no path is entered the default path is /usr/local. <i>Do not enter a path such as "Signalogic" or "/home/Signalogic"</i> as during the install a "Signalogic" symlink is created for the base install folder, which would conflict. Here are a few possible install path examples:

```
  /home
  /home/user_name
  /root
```

To be compatible with the install path in the [Docker Containers](#user-content-dockercontainers) you can use:

```
  /home/sigsrf_sdk_demo
```

After entering an install path, you will be prompted for confirmation. After confirming the install path, if unrar is not available you will be prompted whether to install it (note - without unrar the install will fail). During the install, if g++/gcc build tools are not found on your system, the install script will ask permission to install them, and then build EdgeStream applications as part of the install process. This is important, as the EdgeStream app Makefiles look for OS distro version, tools version, libraries such as libmvec, and packages like ALSA and Kaldi, and set compiler defines based on what is found on the system. For example, if you are installing the ASR option, and Kaldi is not found on your system, then the mediaMin Makefile will set compiler defines to use Kaldi libs included in the Rar package.

If at any time you want to abort the install and start over, press Ctrl-C.

<sup>[1]</sup> Selecting a VM platform enables additional resource management needed when host and guest share DirectCore and/or hardware resources.

<a name="NoPrompts"></a>
#### No Prompts Argument

If needed, the install script can be run with a "no prompts" argument:

    source autoInstall_SigSRF_SDK_2022v7.sh -noprompts

in which case the script will assume default values and issue no user prompts. This can be helpful for automated setup and maintenance.

<a name="CheckVerify"></a>
#### Check / Verify

If needed, the Check / Verify option can be selected to generate a log for troubleshooting and tech support purposes. The Check / Verify option also generates screen output, here is an example:

```
Distro Info
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 20.04.1 LTS
Release:        20.04
Codename:       focal
Kernel Version: 5.4.0-86-generic

EdgeStream and SigSRF Install Path and Options Check
Install path: /home/labuser
Install options:

EdgeStream and SigSRF Symlinks Check
Signalogic Symlink ..............................................[ OK ]
Apps Symlink ....................................................[ OK ]
Linux Symlink ...................................................[ OK ]

SigSRF Libs Check
hwlib ...........................................................[ OK ]
pktlib ..........................................................[ OK ]
voplib ..........................................................[ OK ]
streamlib .......................................................[ OK ]
diaglib .........................................................[ OK ]
hwmgr ...........................................................[ OK ]
filelib .........................................................[ OK ]
cimlib ..........................................................[ OK ]

EdgeStream Apps Check
iaTest ..........................................................[ OK ]
mediaTest .......................................................[ OK ]
mediaMin ........................................................[ OK ]

DirectCore Residual Files Check
hwlib_mutex .....................................................[ OK ]
hwlib_info ......................................................[ OK ]
```

The generated log file will have a filename something like DirectCore_diagnostic_report_01.06.2021-14:48:56.txt.

If you are installing with the coCPU option, the install script will also load the DirectCore coCPU driver. In that case during installation you should see something like:

```
Loading coCPU driver ...

make load
sync
insmod sig_mc_hw.ko
chmod 666 /dev/sig_mc_hw
../hw_utils/driver_query

Module: sig_mc_hw
   Number of C6678 devices detected: 8
   Number of C6678 cores detected:   64
   Number of reserved C6678 cores:   0

Chip Status:
   Chip 0: Available
   Chip 1: Available
   Chip 2: Available
   Chip 3: Available
   Chip 4: Available
   Chip 5: Available
   Chip 6: Available
   Chip 7: Available

coCPU driver is loaded
```

If for any reason it should be needed, the driver can be manually loaded using:

```
cd driver   # note -- assumes you're on subfolder install_path/Signalogic/DirectCore
make load
```

If the driver is already loaded you will see:

```
sync
insmod sig_mc_hw.ko
insmod: error inserting 'sig_mc_hw.ko': -1 File exists
make: *** [load] Error 1
```

which although it shows an error message will cause no problems.

### Building Reference Applications

Reference application examples are provided as executables, C/C++ source code and Makefiles. After installing a Rar package, reference apps may run as-is, but also they may not, as the EdgeStream app Makefiles look for system configuration (such as OS distro version, tools version, libraries like libmvec, and packages like ALSA and Kaldi), and set compiler defines based on what is found on the system. Note this does not apply to Docker containers, as they are already pre-configured with ready-to-run EdgeStream apps.

EdgeStream apps can be rebuilt at any time using gcc and/or g++.  To allow this, the Rar package install script checks for the presence of the following run-time and build related packages:  gcc, ncurses, lib-explain, and redhat-lsb-core (RedHat and CentOS) and lsb-core (Ubuntu). These are prompted for and installed if not found.

<a name="TestFileNotes"></a>
## Test File Notes

Several pcap and wav files are included in the default install, providing input for example command lines. After these are verified to work, user-supplied pcaps, UDP input, and wav files can be used.

Additional advanced pcap examples are also available, including:

    -multiple streams with different LTE codecs and DTX configurations
    -multiple RFC8108 (SSRC transitions)
    -sustained packet loss (up to 10%), both media and SID
    -call gaps
    -media server playout packet rate variation (up to +/-10%)
    -sustained packet rate mismatches between streams
    -dormant SSRC ("stream takeover")
    -RFC7198 (temporal packet duplication)

For these pcaps, the "advanced pcap" .rar file must also be downloaded. This rar is password protected; to get the password please register with Signalogic (either from the website homepage or through e-mail). Depending on the business case, a short NDA covering only the advanced pcaps may be required. These restrictions are in place as as these pcaps were painstakingly compiled over several years of deployment and field work; they provide an advanced test suite our competitors don't have. If you already have multistream pcaps the reference apps will process these without limitation. Depending on your results you may want the Signalogic pcap examples for comparison. Both libpcap and pcapng formats are supported.

Example command lines for both the default set of pcaps and wav files and advanced pcaps are given on the <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md">mediaMin and mediaTest page</a>. 

<a name="ASRNotes"></a>
## ASR Notes
                 
The SigSRF "inferlib" (inference library) module provides an interface to Kaldi ASR. In the SigSRF and EdgeStreawm SDK the Kaldi "mini-librispeech" model is used, which is trained with English speech and has a vocabulary size around 20k words. More information is at <a href="https://medium.com/@qianhwan/understanding-kaldi-recipes-with-mini-librispeech-example-part-1-hmm-models-472a7f4a0488"> Understanding Kaldi with mini-librispeech</a>.

To run ASR, you can either download a Docker container (see [Docker Containers](#user-content-dockercontainers) above) or install the ASR version Rar package (see [ASR Install Notes](#user-content-asrinstallnotes) above). The <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md">mediaMin page</a> has command line examples running ASR on RTP encoded pcaps. mediaTest can be used to generate pcaps from USB input or audio file (wav, au, etc). pcaps may be encoded with any of the codecs supported in the SDK. mediaMin is currently being modified to support direct .wav and USB audio input.

For ASR performance and accuracy, system performance is crucial -- unless you are running a Xeon E5-25xx core or similar you won't see sustained real-time performance. However, because SigSRF libs have been developed and deployed on Linux systems for telecoms who are extremely sensitive to losing any amount of data, near-real-time ASR is possible even on slow CPUs such as Atom C2300 series. On slower CPUs, thread preemption and buffer management built into SigSRF libs allows for several seconds of real-time ASR operation separated by pauses.
                 
Currently SigSRF attempts to maintain real-time performance for one stream group per x86 core (a stream group may have up to 8 input stream contributors). Without ASR enabled, typical capacity is around 30 to 50 stream groups per core, including all packet handling, jitter buffer, codecs, stream group merging and signal processing. Enabling ASR reduces capacity by a wide margin, as state-of-the-art ASR is heavily dependent on calculation intensive processing including DNNs (deep neural networks), HMM/GMM acoustic modeling, and HCLG based finite state transducers.
     
<a name="RunNotes"></a>
## Run Notes

Docker containers are pre-configured with ready-to-run EdgeStream executables. For Rar packages, the install script builds executables as its last step -- <i>if that fails for any reason, make sure you initially rebuild the demo and reference apps before running them. This is important, as the Makefiles look for system configuration (such as OS distro version, tools version, libraries like libmvec, and packages like ALSA and Kaldi), and then set compiler defines based on what's found on the system.</i>

Reference applications limits are listed below.  The iaTest and paTest apps do not have a functionality limit. mediaMin and mediaTest app functionality is limited as follows:

   1) Data limit.  Processing is limited to 100,000 frames / payloads of data.  There is no limit on data sources, which include various file types (audio, encoded, pcap), network sockets, and USB audio.

   2) Concurrency limit.  Maximum number of concurrent instances is two and maximum number of channels per instance is 2 (total of 4 concurrent channels).

If you would prefer an evaluation version with increased data and concurrency limits for a trial period, [contact us](#DocumentationSupport). This requires a business case and possibly an NDA.

<a name="mediaMin_and_mediaTest"></a>
### mediaMin and mediaTest Reference Applications

The <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md">mediaMin and mediaTest page</a> gives example command lines for streaming media, transcoding, speech recognition, waveform file and USB audio processing, and more.

mediaMin is a production reference application, using a minimum set of APIs (create/delete session, push/pull packets), it can handle a wide range of RTP audio packet inputs. mediaTest is focused on test and measurement functionality and accepts USB and wav file audio input. Some things you can do with mediaMin and mediaTest command lines:

  * transcode between pcaps, for example EVS to AMR-WB, AMR-NB to G711, etc.
  * "AMR Player", play an AMR pcap (either AMR-WB or AMR-NB)
  * "EVS Player", play an EVS pcap
  * transcode multistream pcaps and merge all streams together into one output audio (for voice pcaps, this generates a "unified conversation")
  * Kaldi speech recognition on pcaps or audio files (ASR, 200k word vocabulary)
  * test codecs and compare output vs. 3GPP or ITU reference files <sup>1</sup>
  * insert user-defined signal processing or inference into the real-time data flow
  * input and output .wav file and other audio format files
  * input and output USB audio
  * test and measure packet RFCs, jitter buffer, packet loss and other stats, and more

For both mediaMin and mediaTest, reference application C/C++ source code is included.  The apps are based on deployed production code used in high capacity, real-time applications.  Performance measurements can be made that are accurate and competitive with other commercially available software.

<sup>1 </sup>Includes non-3GPP and non-ITU codecs such as MELPe

<a name="iaTest"></a>
### iaTest

The <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/iaTest_readme.md">iaTest page</a> gives example command lines for image analytics and OpenCV testing.  The iaTest app performs image analytics operations vs. example surveillance video files and allows per-core performance measurement and comparison for x86 and coCPU cores.  .yuv and .h264 file formats are supported.  Application C/C++ source code is included.

<a name="paTest"></a>
### paTest

The <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/paTest_readme.md">paTest page</a> gives example command lines for a predictive analytics application that applies algorithms and deep learning to continuous log data in order to predict failure anomalies.  Application Java and C/C++ source code is included.

<a name="SDKDemoTestedPlatforms"></a>
# SDK / Demo Tested Platforms

The EdgeStream and SigSRF SDK and demos have been tested on the following platforms:
 
<b>Ubuntu</b>
 
  Ubuntu 20.04.1 LTS</br>
  g++/gcc (Ubuntu 9.3.0-17ubuntu1\~20.04) 9.3.0
   
  Ubuntu 18.04.5 LTS</br>
  g++/gcc (Ubuntu 8.4.0-1ubuntu1\~18.04) 8.4.0
   
  Ubuntu 16.04.6 LTS</br>
  g++/gcc (Ubuntu 7.4.0-1ubuntu1\~16.04\~ppa1) 7.4.0
   
  Ubuntu 12.04.5 LTS</br>
  gcc (Ubuntu/Linaro 4.6.4-1ubuntu1\~12.04) 4.6.4</br>
  g++ (Ubuntu 4.8.4-1ubuntu15\~12.04.1) 4.8.4
 
<b>CentOS</b>
 
  CentOS Linux release 8.2.2004 (Core)</br>
  g++/gcc (GCC) 8.3.1 20191121 (Red Hat 8.3.1-5)
   
  CentOS Linux release 7.6.1810 (Core)</br>
  g++/gcc (GCC) 6.5.0

  CentOS Linux release 7.9.2009 (Core)</br>
  g++/gcc (GCC) 5.5.0</br>
  
  CentOS Linux release 7.9.2009 (Core)</br>
  g++/gcc (GCC) 5.3.1 20160406 (Red Hat 5.3.1-6)

  CentOS Linux release 7.6.1810 (Core)</br>
  g++/gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-44)
   
<a name="DocumentationSupport"></a>
# Documentation, Support, and Contact

## SigSRF Software Documentation

SigSRF documentation, including Quick Start command line examples, High Capacity Operation, API Usage, and other sections is located at:

&nbsp;&nbsp;&nbsp;&nbsp;<a href="https://bit.ly/2UZXoaW" target="_blank">SigSRF Documentation</a>
 
## coCPU Users Guide

The <a href="https://bit.ly/2J18F3f" target="_blank">coCPU User Guide</a> provides information about coCPU hardware and software installation, test and reference applications, build instructions, etc.

## Technical Support / Questions

Limited tech support for the SigSRF SDK and coCPU option is available from Signalogic via e-mail and Skype.  You can request group skype engineer support using Skype Id "signalogic underscore inc" (replace with _ and no spaces).  For e-mail questions, send to "info at signalogic dot com".
