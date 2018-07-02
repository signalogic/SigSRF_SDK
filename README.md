# Table of Contents

[SigSRF Overview](#Overview)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Software and Streaming I/O Architecture Diagram](#SoftwareArchitectureDiagram)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Packet and Media Processing Data Flow Diagram](#DataFlowDiagram)<br/>
[SDK and Demo Download](#SDKDemoDownload)<br/>
[Install Notes](#InstallNotes)<br/>
[Demo Pages](#DemoPages)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[mediaTest (streaming media, buffering, transcoding, and packet RFCs)](#mediaTestDemo)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[iaTest (image analytics)](#iaTestDemo)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[paTest (predictive analytics from log data)](#paTestDemo)<br/>
[Documentation, Support, and Contact](#DocumentationSupport)<br/>

<a name="Overview"></a>
# SigSRF Overview

The SigSRF (Streaming Resource Functions) SDK introduces a scalable approach to media, HPC, and AI servers.  The basic concept is to scale between cloud, private cloud, and very small form factors (including embedded and IoT servers) while maintaining a cloud programming model.

The primary objectives of SigSRF software are:

* provide high performance software modules for media, AI (deep learning), and analytics streaming applications
* scale up with or without GPU, and provide high capacity, "real-time at scale" streaming and processing
* scale down with or without ARM, and provide IoT and Edge embedded device solutions for SWaP <sup>1</sup> constrained applications
* maintain full program compatibility with cloud servers, including open source software support, server architectures, latest programming languages, etc.

x86 software is sometimes referred to as a "software only" solution, but there is no software without hardware.  High performance applications quickly encounter hardware limitations, and as much as people have wished to solve this using generic x86 processors, that hasn't happened for 30 years and isn't likely to happen any time in the near future.  One solution is heterogeneous CPU cores, for example one type of core to "front" streaming data and perform some initial, specific processing, and another type of core to perform general processing.  To enable mixed core processing, SigSRF supports coCPU&trade; technology, which adds NICs and up to 100s of coCPU cores to scale per-box streaming and performance density.  For example, coCPUs can turn conventional 1U, 2U, and mini-ITX servers into high capacity media, HPC, and AI servers, or they can allow an embedded AI server to operate independently of the cloud.  coCPU cards have NICs, allowing coCPUs to front streaming data and perform wirespeed packet filtering, routing decisions and processing.

For all platforms, SigSRF supports OpenCV, media transcoding, TensorFlow <sup>2</sup>, speech recognition <sup>2</sup>, and other calculation / data intensive applications.  For applications facing SWaP constraints, SigSRF software supports a wide range of coCPU and SoC embedded device targets that can avoid FPGAs, ASICs and other "hardwired" solutions which might dictate a non-cloud compatible software architecture.

SigSRF supports concurrent multiuser operation in a bare-metal environment, and in a KVM + QEMU virtualized environment, cores and network I/O interfaces appear as resources that can be allocated between VMs. VM and host users can share also, as the available pool of cores is handled by a physical layer back-end driver. This flexibility allows media, HPC, and AI applications to scale between cloud, enterprise, and remote vehicle/location servers.

<sup>1</sup> SWaP = size, weight, and power consumption<br/>
<sup>2</sup> In progress

<a name="SoftwareArchitectureDiagram"></a>
## SigSRF Software and Streaming I/O Architecture Diagram

Below is a SigSRF software and streaming I/O architecture diagram.

![SigSRF software and streaming I/O architecture diagram](https://github.com/signalogic/SigSRF_SDK/blob/master/images/SigSRF_Software_Architecture_and_Packet_IO_RevA2.png?raw=true "SigSRF software and streaming I/O architecture diagram")

<a name="DataFlowDiagram"></a>
## SigSRF Packet and Media Processing Data Flow Diagram

Below is a SigSRF software streaming packet and media processing data flow diagram.

![SigSRF streaming packet and media processing data flow diagram](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Streaming_packet_and_media_processing_data_flow_RevA1.png?raw=true "SigSRF streaming packet and media processing data flow diagram")

Some notes about the above data flow diagram:

   1) Data flow matches <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md">mediaTest</a> example application C source code (packet_mode section of x86_mediaTest.c).  Subroutine symbols are labeled with pktlib and voplib API names.

   2) A few areas of the flow diagram are somewhat approximated, to simplify and make easier to read.  For example, loops do not have "for" or "while" flow symbols, and some APIs, such as DSCodecEncode() and DSFormatPacket(), appear in the flow once, but actually may be called multiple times, depending on what signal processing algorithms are in effect.

   3) The orange vertical line divides the "packet domain" and "audio domain".  DSStoreStreamData() and DSGetStreamData() decouple these domains in the case of unequal ptimes.  The audio domain contains raw audio data in 16-bit PCM format, which allows signal processing operations, such as sample rate conversion, conferencing, echo cancellation, etc, to be performed.  Also this is where voice analytics takes place, for instance by handing audio data off to another process.

<a name="SDKDemoDownload"></a>
## SDK and Demo Download

The SigSRF SDK and demo download consists of an install script and .rar files and includes:
  
    1) A limited eval / demo version of several SigSRF demos, including telecom transcoding, image analytics, and H.264 video streaming (ffmpeg acceleration)
    
    2) C/C++ source code showing Pktlib and Voplib API usage (source and Makefiles for demo programs included)

    3) Multiple concurrent user/instance demonstration

    4) coCPU software stack, including DirectCore libraries and drivers (requires a coCPU card to be active)

A few demo programs are coCPU specific; for those you will need one of the coCPU cards <a href="http://processors.wiki.ti.com/index.php/HPC" target="_blank">shown here</a>.  coCPU cards can be obtained from TI, Advantech, or Signalogic.

<a name="InstallNotes"></a>
## Install Notes

Separate RAR packages are provided for different Linux distributions. Please choose the appropriate one or closest match. For some files, the install script will auto-check for kernel version and Linux distro version to decide which file version to install (including coCPU driver, if you have a coCPU card).

All .rar files and the auto install script must stay together in the same folder after downloading.

Note that the install script checks for the presence of the unrar command, and if not found it will install the unrar package.

Media transcoding demos require tcpreplay or other method to generate packet traffic (using as input pcap files included in the install).

### Sudo Privilege

The install script requires sudo root privilege.  In Ubuntu, allowing a user sudo root privilege can be done by adding the user to the “administrators” group (<a href=http://askubuntu.com/questions/168280/how#do#i#grant#sudo#privileges#to#an#existing#user target="_blank">as shown here</a>).  In CentOS a user can be added to the “/etc/sudoers” file (<a href="https://wiki.centos.org/TipsAndTricks/BecomingRoot" target="_blank">as shown here</a>).  Please make sure this is the case before running the script.

### Building Test and Demo Applications

Test and demo application examples are provided as executables, C/C++ source code and Makefiles.  Executables may run, but if not (due to Linux distribution or kernel differences), they should be rebuilt using gcc.  To allow this, the install script checks for the presence of the following run-time and build related packages:  gcc, ncurses, lib-explain, and redhat-lsb-core (RedHat and CentOS) and lsb-core (Ubuntu).  These are installed if not found.

### Running the Install Script

To run the install script enter:
    
    > source autoInstall_Sig_BSDK_2017v2.sh
 
The script will then prompt as follows:
    
    1) Host
    2) VM
    Please select target for co-CPU software install [1-2]:
    
After choosing an install target of either Host or VM, the script will next prompt for an install option:

    1) Install SigSRF software
    2) Uninstall SigSRF software
    3) Check / Verify
    4) Exit
    Please select install operation to perform [1-4]:
  
If the install operation (1.) is selected, the script will prompt for an install path:

    Enter the path for SigSRF software installation:

If no path is entered the default path is /usr/local.

If needed, the Check / Verify option can be selected to generate a log for troubleshooting and tech support purposes.

<a name="DemoPages"></a>
## Demo Pages

<a name="mediaTestDemo"></a>
### mediaTest Demo

The <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md">mediaTest demo page</a> gives example command lines for streaming media, buffering, transcoding, and packet RFCs.  The demo allows codec output comparison vs. 3GPP reference files, per-core performance measurement (both x86 and coCPU cores), .wav file generation to experience codec audio quality, RTP packet transcoding using pcap files, and more.  The state-of-the-art EVS codec is used for several of the command lines.  Application C/C++ source code is included.

<a name="iaTestDemo"></a>
### iaTest Demo

The <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/iaTest_readme.md">iaTest demo page</a> gives example command lines for image analytics and OpenCV testing.  The iaTest demo performs image analytics operations vs. example surveillance video files and allows per-core performance measurement and comparison for x86 and coCPU cores.  .yuv and .h264 file formats are supported.  Application C/C++ source code is included.

<a name="paTestDemo"></a>
### paTest Demo

The <a href="https://github.com/signalogic/SigSRF_SDK/blob/master/paTest_readme.md">paTest demo page</a> gives example command lines for a predictive analytics application that applies algorithms and deep learning to continuous log data in order to predict failure anomalies.  Application Java and C/C++ source code is included.

<a name="DocumentationSupport"></a>
## Documentation, Support, and Contact

### SigMRF Users Guide

SigMRF (Media Resource Functions) software is part of SigSRF software. The <a href="http://goo.gl/fU43oE" target="_blank">SigMRF User Guide</a> provides detailed information about SigMRF software installation, test and demo applications, build instructions, etc.

### coCPU Users Guide

The <a href="http://goo.gl/Vs1b3R" target="_blank">coCPU User Guide</a> provides detailed information about coCPU and software installation, test and demo applications, build instructions, etc.

### Technical Support / Questions

Limited tech support for the SigSRF SDK and coCPU option is available from Signalogic via e-mail and Skype.  You can ask for group skype engineer support using Skype Id "signalogic underscore inc" (replace with _ and no spaces).  For e-mail questions, send to "info at signalogic dot com".
