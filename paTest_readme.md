# paTest Demo

*** This demo is not yet available for download ***

After installing the [SigSRF SDK eval](https://github.com/signalogic/SigSRF_SDK), below are notes and example command lines for the paTest<sup> 1</sup> demo.  The demo has two (2) purposes:

 - show a predictive analytics application that applies algorithms and deep learning to continuous log data in order to predict failure anomalies
 
 - provide an example application, including Java and C/C++ source code, that interfaces to Spark and SigSRF software, and shows examples of API usage for both
 
<sup>1 </sup>paTest = predictive analytics test<br/>

# Other Demos

[mediaTest Demo (Streaming Media, Buffering, Transcoding, and Packet RFCs)](https://github.com/signalogic/SigSRF_SDK/blob/master/mediaTest_readme.md)

[iaTest Demo (Image Analytics)](https://github.com/signalogic/SigSRF_SDK/blob/master/iaTest_readme.md)

# Table of Contents

[Predictive Analytics from Log Data](#PredictiveAnalyticsLogData)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;[Data Flow Diagram](#DataFlowDiagram)<br/>
[Install Notes](#InstallNotes)<br/>
[Demo Notes](#DemoNotes)<br/>
[coCPU Notes](#coCPUNotes)<br/>

<a name="PredictiveAnalyticsLogData"></a>
# Predictive Analytics from Log Data

The demo defines as follows the requirements for a practical, deployable vision + AI server:

<a name="DataflowDiagram"></a>
## DataFlow Diagram

Below is a data flow diagram showing I/O, algorithms, and convolutional neural networks used to predict anomalies in log data.

&nbsp;<br/>

![Image](https://github.com/signalogic/SigSRF_SDK/blob/master/images/Log_flow_diagram_algorithm_cnn.png?raw=true "Log data predictive analytics data flow diagram")

&nbsp;<br/>

<a name="InstallNotes"></a>
# Install Notes

TBD

<a name="DemoNotes"></a>
# Demo Notes

TBD

<a name="coCPUNotes"></a>
# coCPU&trade; Notes

As explained on the main SigSRF SDK page, the demos support coCPU™ technology, which adds NICs and up to 100s of coCPU cores to scale per-box streaming and performance density. For example, coCPUs can turn conventional 1U, 2U, and mini-ITX servers into high capacity media, HPC, and AI servers, or they can allow an embedded AI server to operate independently of the cloud. coCPU cards have NICs, allowing coCPUs to front streaming data and perform wirespeed packet filtering, routing decisions and processing.
The coCPU cards supported by the demos include:

* High performance, including extensive SIMD capability, 8 or more cores per CPU, L1 and L2 cache, and advanced DMA capability
* Contain onchip network I/O and packet processing and onchip PCIe
* Access to 2 (two) GB or more external DDR3 mem
* Able to efficiently decode camera input, e.g. H.264 streams arriving as input via onchip network I/O
* CGT<sup> 4</sup> supports gcc compatible C/C++ build and link, mature and reliable debug tools, RTOS, and numerous libraries

The current vision + AI server demo uses TI C6678 CPUs, which meet these requirements.  Over time, other suitable CPUs may become available.

Combining x86 and c66x CPUs and running software components necessary for AI applications such as H.264 decode, OpenCV and TensorFlow, is another form of an ["AI Accelerator"](https://en.wikipedia.org/wiki/AI_accelerator). The architecture described here favors fast, reliable development: mature semiconductors and tools, open source software, standard server format, and a wide range of easy-to-use peripherals and storage. 

<sup>4 </sup>CGT = Code Generation Tools
