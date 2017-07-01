# iaTest Demo

Assuming you have installed the [SigSRF SDK eval](https://github.com/signalogic/SigSRF_SDK), here are some command lines and notes for the iaTest demo.  The demo has two (2) purposes:

 - show how to implement an Atom-based vision + AI server with 34 total CPU cores under 75 W
 
 - provide an example application, including source code, to measure OpenCV performance between Atom only and Atom + coCPU cores, with no ARM or GPU

In addition to OpenCV, the next iteration of this demo will will include TensorFlow.

# Table of Contents

[Vision + AI Atom Server](#AtomServer)<br/>
[Atom Only Tests](#AtomTests)<br/>
[Atom + coCPU Tests](#coCPUTests)<br/>
[Install Notes](#InstallNotes)<br/>
[Demo Notes](#DemoNotes)<br/>

<a name="AtomServer"></a>
# Vision + AI Atom Server

The demo defines the requirements for a practical, deployable Atom vision + AI server as follows:

* Small size, 8" x 9" x 3"
* Low power -- target of 50 W, the current prototype shown here is 75 W
* High performance -- this demo shows a 16x increase in OpenCV capacity vs. a dual-core Atom
* Cloud compatible programming model -- no ARM, GPU, or FPGA, no special APIs or flow graphs, etc
* All cores have direct access to network I/O
* Ready to run OpenCV and TensorFlow

Below are some pictures of the demo Atom server, with 32 coCPU&trade; cores installed. coCPU cores are high performance CPU cores that run gcc compatible C/C++ code.

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Local_AI_server_C2358_32cores_iso_view.png?raw=true "AI + vision Atom server, iso view")

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Small_AI_server_32cores_iso_view.png?raw=true "AI + vision Atom server, with top removed showing coCPU card installation")

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Small_AI_server_32cores_top_view.png?raw=true "AI + vision Atom server, top view")

Specifics of the vision + AI server shown here include:

* Mini-ITX motherboard and case
* Dual core Atom (C2358, 1.74 GHz), 4x GbE interfaces, 8 GB DDR3 mem, 1333 MHz
* 32 coCPU cores (C6678, 1.6 GHz), GbE interface, 8 GB DDR3 mem, 1333 MHz, x8 PCIe
* 4x USB interfaces
* IPMI (dedicated GbE)
* Audio I/O interface (via USB)
* VGA optional display

<a name="AtomTests"></a>
# Atom Only Tests

```C
./iaTest -m1 -cx86 -s0 -i/test_files/hallway_352x288p_30fps_420fmt.yuv -x352 -y288 -ohallway_test.yuv -l0x01000003
```
<a name="coCPUTests"></a>
# Atom + coCPU Tests

To run coCPU tests, a coCPU card has to be installed in the Atom server.  The pictures above show a 32-core card; a 64-core card is also available.  Cards can be obtained from Signalogic, Advantech, or Texas Instruments.

Below are example command lines to use with coCPU cards.

```C
./iaTest -m1 -f1600 -eia.out -cSIGC66XX-8 -s0 -i/test_files/hallway_352x288p_30fps_420fmt.yuv -x352 -y288 -ohallway_test.yuv -l0x01000003

./iaTest -m1 -f1600 -eia.out -cSIGC66XX-8 -s0 -i/test_files/CCTV_640x360p_30fps_420fmt.yuv -x640 -y360 -occtv_test.yuv -l0x01100003
```

<a name="InstallNotes"></a>
# Install Notes

1) For Atom operation, the demo installs two OpenCV v3.2 libraries (libcore_sig.so and libimgproc_sig.so).  Demo-specific filenames are used so as not to interfere with an existing OpenCV installation.  For coCPU operation, the demo installs the libopencv.le66 library, which includes a number of OpenCV 2.4.2 libraries.

2) For Atom operation, demo source code (for example ia.c and yuv.c files) can be modified, rebuilt, and linked, but if new OpenCV functions are required then the iaTest Makefile will need to be modified to reference the required libraries.  For coCPU operation, demo source can also be modified; in this case you will need to download the c66x CGT tools for Linux from TI's website in order to rebuild.  The Makefile installed on the mCPU_target subfolder may need to be modified.

<a name="DemoNotes"></a>
### Demo Notes

1) iaTest = image analytics test.  The iaTest demo is one of several SigSRF demos, including mediaTest (media streaming and transcoding, and ffmpeg_accel (accelerated ffmpeg encoding and image processing).

2) iaTest source code files use #ifdef's to allow compilation for both x86 and c66x CPUs.  All source code examples make C or C++ OpenCV function calls per OpenCV standards.  All source code can have network I/O APIs added.
