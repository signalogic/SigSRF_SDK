# iaTest Demo

Assuming you have installed the [SigSRF SDK eval](https://github.com/signalogic/SigSRF_SDK), here are some command lines and notes for the iaTest demo.  Input and output options include .yuv file formats.

iaTest serves two (2) purposes:

 - an example application, including source code, showing how to obtain 16x higher capacity OpenCV on small, low-power Atom servers
 
 - measure OpenCV performance between Atom only and Atom + coCPU card, with no ARM or GPU involved

# Table of Contents

[Atom Only Tests](#AtomTests)<br/>
[Atom + coCPU Tests](#coCPUTests)<br/>
[Atom Server](#AtomServer)<br/>
[Install Notes](#InstallNotes)<br/>

<a name="AtomTests"></a>
## Atom Only Tests

```C
./iaTest -m1 -cx86 -s0 -i/test_files/hallway_352x288p_30fps_420fmt.yuv -x352 -y288 -ohallway_test.yuv -l0x01000003
```
<a name="coCPUTests"></a>
### Atom + coCPU Tests


Below is a frame mode command line that reads a pcap file and outputs to wav file.  No jitter buffering is done, so any out-of-order packets, DTX packets, or SSRC changes are not handled.  The wav file sampling rate is determined from the session config file.

```C
./iaTest -m1 -f1600 -eia.out -cSIGC66XX-8 -s0 -i/test_files/hallway_352x288p_30fps_420fmt.yuv -x352 -y288 -ohallway_test.yuv -l0x01000003

./iaTest -m1 -f1600 -eia.out -cSIGC66XX-8 -s0 -i/test_files/CCTV_640x360p_30fps_420fmt.yuv -x640 -y360 -occtv_test.yuv -l0x01100003
```

<a name="AtomServer"></a>
### Atom Server

Here are some pictures of the Atom server, with coCPU card installed:

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/session_config_pcap_terminology.png?raw=true "session config file and pcap terminology -- remote vs. local, src vs. dest")



<a name="InstallNotes"></a>
### Install Notes

1) For Atom operation, the demo installs two OpenCV v3.2 libraries (libcore_sig.so and libimgproc_sig.so).  Demo-specific filenames are used so as not to interfere with an existing OpenCV installation.  For coCPU operation, the demo installs the libopencv.le66 library, which includes a number of OpenCV 2.4.2 libraries.

2) For Atom operation, demo source code (for example ia.c and yuv.c files) can be modified, rebuilt, and linked, but if new OpenCV functions are required then the iaTest Makefile will need to be modified to reference the required libraries.  For coCPU operation, demo source can also be modified; in this case you will need to download the c66x CGT tools for Linux from TI's website in order to rebuild.  The Makefile installed on the mCPU_target subfolder may need to be modified.
