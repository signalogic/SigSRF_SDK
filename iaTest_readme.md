# iaTest Demo

After installing the [SigSRF SDK eval](https://github.com/signalogic/SigSRF_SDK), below are notes and example command lines for the iaTest<sup> 1</sup> demo.  The demo has two (2) purposes:

 - show how to implement an Atom-based vision<sup> 2</sup> + AI server with 34 total CPU cores under 75 W
 
 - provide an example application, including source code, to measure OpenCV performance between Atom only and Atom + coCPU cores, with no ARM or GPU

In addition to OpenCV, the next iteration of this demo will include TensorFlow.

<sup>1 </sup>iaTest = image analytics test<br/>
<sup>2 </sup>vision = computer vision, machine vision, vehicle vision, etc

# Other Demos

[mediaTest Demo (Image Analytics)](https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md)

[paTest Demo (Predictive Analytics)](https://github.com/signalogic/SigSRF_SDK/blob/master/paTest_readme.md)


# Table of Contents

[Vision + AI Server](#AtomServer)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Architecture Diagram](#ArchitectureDiagram)<br/>
[Atom Only Tests](#AtomTests)<br/>
[Atom + coCPU Tests](#coCPUTests)<br/>
[Output Frame Grabs](#OutputFrameGrabs)<br/>
[coCPU Notes](#coCPUNotes)<br/>
[Install Notes](#InstallNotes)<br/>
[Demo Notes](#DemoNotes)<br/>
[Power Consumption Notes](#PowerConsumptionNotes)<br/>
[Embedded AI Comparison](#EmbeddedAIComparison)<br/>

<a name="AtomServer"></a>
# Vision + AI Server

The demo defines as follows the requirements for a practical, deployable vision + AI server:

* Small size, 8" x 9" x 3"
* Low power -- target of 50 W, the current prototype shown here is 75 W
* High performance -- this demo shows a 16x increase in OpenCV capacity vs. dual-core Atom
* Cloud compatible programming model -- use Atom x86 + Linux, with no ARM, GPU, or FPGA, no special APIs or flow graphs, etc
* All cores have direct access to network I/O
* Ready to run OpenCV and TensorFlow

Low SWaP<sup> 3</sup> requirements are obvious enough; what is less obvious, yet cannot be overemphasized, is the importance of a fully cloud compatible programming model.  All new vision and AI algorithms, including low SWaP applications, are tested in the cloud prior to production.  No one wants to be forced to port complex, performance sensitive algorithm based code to ARM and end up in an unsupported backwater.

Below are some pictures of the demo Atom server, with 32 coCPU&trade; cores installed. coCPU cores are high performance CPU cores that run gcc compatible C/C++ code

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Local_AI_server_C2358_32cores_iso_view.png?raw=true "AI + vision Atom server, iso view")

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Small_AI_server_32cores_iso_view.png?raw=true "AI + vision Atom server, with top removed showing coCPU card installation")

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Small_AI_server_32cores_top_view.png?raw=true "AI + vision Atom server, top view")

Specifics of the Atom-based vision + AI demo server include:

* Mini-ITX motherboard and case
* Dual core Atom (C2358, 1.74 GHz), 4x GbE interfaces, 8 GB DDR3 mem, 1333 MHz
* 32 coCPU cores (C6678, 1.6 GHz), GbE interface, 8 GB DDR3 mem, 1333 MHz, x8 PCIe
* 4x USB interfaces
* IPMI (dedicated GbE)
* Audio I/O interface (via USB)
* VGA optional display

<sup>3 </sup>SWaP = Size, Weight, and Power consumption

<a name="ArchitectureDiagram"></a>
## Architecture Diagram

Below is an architecture diagram showing network I/O and processing performed by x86 CPUs and c66x coCPUs.

&nbsp;<br/>

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/SigSRF_Software_Architecture_Image_Analytics_w_cameras.png?raw=true "Atom server software architecture diagram")

&nbsp;<br/>

<a name="AtomTests"></a>
# Atom Only Tests

Below are example command lines to use with with or without a coCPU card installed.  The demo source code performs motion detection and tracking, with a rudimentary algorithm that compensates for camera motion (such as jerky hand-held mobile phone video).

```C
./iaTest -m1 -cx86 -s0 -itest_files/hallway_352x288p_30fps_420fmt.yuv -x352 -y288 -ohallway_test.yuv -l0x01000003
```
<a name="coCPUTests"></a>
# Atom + coCPU Tests

To run coCPU tests, a coCPU card must be installed in the Atom server.  The pictures above show a 32-core card; a 64-core card is also available (but would increase the size of the enclosure).  If supported by the riser, multiple 32-core cards can be installed.  Cards can be obtained from Signalogic, Advantech, or Texas Instruments.

Below are example command lines to use with coCPU cards.

```C
./iaTest -m1 -f1600 -eia.out -cSIGC66XX-8 -s0 -itest_files/hallway_352x288p_30fps_420fmt.yuv -x352 -y288 -ohallway_test.yuv -l0x01000003

./iaTest -m1 -f1600 -eia.out -cSIGC66XX-8 -s0 -itest_files/CCTV_640x360p_30fps_420fmt.yuv -x640 -y360 -occtv_test.yuv -l0x01100003
```
<a name="OutputFrameGrabs"></a>
# Output Frame Grabs

Below are some example frame grabs from output .yuv files.  After running the demo, this is what you should see for output yuv file results, prior to making source code modifications.

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Surveillance_video_detection_algorithm_concurrent_suspects.png?raw=true "Object tracking and stats printout in CCTV surveillance video with unstable camera (video was captured with a hand-held mobile phone)")

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Hallway_men_walking.png?raw=true "Object tracking and stats printout or men walking in a hallway carrying objects, stable camera")

<a name="coCPUNotes"></a>
# coCPU&trade; Notes

coCPU cores must meet the following requirements:

* High performance, including extensive SIMD capability, 8 or more cores per CPU, L1 and L2 cache, and advanced DMA capability
* Contain onchip network I/O and packet processing and onchip PCIe
* Access to 2 (two) GB or more external DDR3 mem
* Able to efficiently decode camera input, e.g. H.264 streams arriving as input via onchip network I/O
* CGT<sup> 4</sup> supports gcc compatible C/C++ build and link, mature and reliable debug tools, RTOS, and numerous libraries

The current vision + AI server demo uses TI C6678 CPUs, which meet these requirements.  Over time, other suitable CPUs may become available.

<sup>4 </sup>CGT = Code Generation Tools

<a name="InstallNotes"></a>
# Install Notes

1) For Atom operation, the demo installs two OpenCV v3.2 libraries (libopencv_core_sig.so and libopencv_imgproc_sig.so).  Demo-specific filenames (with \_sig suffix) are used so as not to interfere with existing OpenCV installations.  For coCPU operation, the demo installs the libopencv.le66 combined library, which includes a number of OpenCV v2.4.2 modules.  This link has [more information on how libopencv.le66 was created](http://processors.wiki.ti.com/index.php/C66x_opencv).

2) For Atom operation, demo source code (for example ia.c and yuv.c files) can be modified, rebuilt, and linked, but if new OpenCV functions are required then the iaTest Makefile will need to be modified to reference the required libraries.  For coCPU operation, demo source can also be modified; in this case you will need to download the c66x CGT tools for Linux from TI's website in order to rebuild.  The Makefile installed on the mCPU_target subfolder may need to be modified.

<a name="DemoNotes"></a>
# Demo Notes

1) The iaTest demo is one of several SigSRF demos, including mediaTest (media streaming and transcoding), and ffmpeg_accel (accelerated ffmpeg encoding and video streaming).

2) iaTest source code files use #ifdef's to allow compilation for both x86 and c66x CPUs.  All source code examples invoke C or C++ OpenCV function calls per OpenCV standards.  All source code can be augmented with network I/O APIs.

3) The demo includes optimized YUV - RGB conversion routines not provided with OpenCV.  These are in the yuv.c file.

<a name="PowerConsumptionNotes"></a>
# Power Consumption Notes

The demo server consumes up to 75 W.  Ongoing R&D work aims to reduce usage to under 50 W, and also add a 35 W "minimum mode", under which reduced processing is performed until some event or trigger enables full processing.

<a name="EmbeddedAIComparison"></a>
# Embedded AI Comparison

The advantages of a miniaturized x86 based AI server are compelling:  immediately run cloud software with no code rewrite, flexible, reliable peripherals and storage like any other server, and high performance using coCPU cores.  The disadvantage is power consumption.  Even though this approach consumes relatively low power, some of the embedded AI boxes out there, such as [PerceptIn](https://www.perceptin.io/) contain numerous ARM cores, which can reduce power consumption to the 20W - 30W range and potentially to under 10W.

To make an x86 based AI server completely portable requires a small Lithium battery, for example [this one](https://www.amazon.com/100-Watt-Portable-Generator-Hurricane-Emergency/dp/B01M3S00H0), but this increases overall solution weight.

SWaP tradeoffs -- as always -- depend on specific application requirements.
