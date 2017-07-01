# iaTest Demo

Assuming you have installed the [SigSRF SDK eval](https://github.com/signalogic/SigSRF_SDK), here are some command lines and notes for the iaTest demo.  Input and output options include .yuv file formats.

iaTest serves two (2) purposes:

 - an example application, including source code, showing how to obtain 16x higher capacity OpenCV on small, low-power Atom servers
 
 - measure OpenCV performance between Atom only and Atom + coCPU card, with no ARM or GPU involved

# Table of Contents

[Atom Only Tests](#AtomTests)<br/>
[Atom + coCPU Tests](#coCPUTests)<br/>

<a name="AtomTests"></a>
## Atom Only Tests

<a name="coCPUTests"></a>
### Atom + coCPU Tests


Below is a frame mode command line that reads a pcap file and outputs to wav file.  No jitter buffering is done, so any out-of-order packets, DTX packets, or SSRC changes are not handled.  The wav file sampling rate is determined from the session config file.

```C
./mediaTest -M4 -cx86 -Csession_config/pcap_file_test_config -ipcaps/EVS_13.2_16000.pcap -oEVS_13.2_16000.wav
```


![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/session_config_pcap_terminology.png?raw=true "session config file and pcap terminology -- remote vs. local, src vs. dest")
