# Github SigSRF License, Version 1.0

This Github SigSRF License Agreement Version 1.0 (the “Agreement”) is a modified form of the the [Confluent Community License Agreement](https://www.confluent.io/confluent-community-license/), and sets forth the terms on which Signalogic, Inc. ("Signalogic") makes available Github "SigSRF_SDK" repository software (the "Github SigSRF Software"). BY INSTALLING, DOWNLOADING, ACCESSING, USING OR DISTRIBUTING ANY OF THE GITHUB SIGSRF SOFTWARE, YOU AGREE TO THE TERMS AND CONDITIONS OF THIS AGREEMENT. IF YOU DO NOT AGREE TO SUCH TERMS AND CONDITIONS, YOU MUST NOT USE GITHUB SIGSRF SOFTWARE. IF YOU ARE RECEIVING GITHUB SIGSRF SOFTWARE ON BEHALF OF A LEGAL ENTITY, YOU REPRESENT AND WARRANT THAT YOU HAVE THE ACTUAL AUTHORITY TO AGREE TO THE TERMS AND CONDITIONS OF THIS AGREEMENT ON BEHALF OF SUCH ENTITY. "Licensee" means you, an individual, or the entity on whose behalf you are receiving Github SigSRF Software.

## 1 LICENSE GRANT AND CONDITIONS

### 1.1 Software Scope and Definitions

SigSRF software has been developed and commercialized by Signalogic since the 1990s as a series of libraries, drivers, and reference applications, primarily focused on packet, media, and signal, and neural net processing, and is currently deployed worldwide by analytics, telecom, and gov/mil customers and end-customers of Signalogic. Github SigSRF Software is a subset of SigSRF Software and is available on the Github SigSRF_SDK repository intended for demonstration ("demo"), evaluation, Research & Development ("R&D"), test and measurement, and limited commercial purposes. Github SigSRF Software makes available certain C/C++ source codes for use-cases including, but not limited to:

<ol type="a">
  <li>Media + packet real-time threads, including SigSRF API usage for packet queue receive/send, jitter buffer add/retrieve, codec decode/encode, stream group processing, and packet diagnostics</li>
  <li>Reference applications, including SigSRF API usage for session create/modify/delete, packet push/pull, and event and packet logging. Also included are static and dynamic session creation, RTP stream auto-detect, packet pcap and UDP input, and UDP encapsulated TCP protocols</li>
  <li>Stream group output audio processing for lawful intelligence, lawful interception, speech recognition, robotics, robotaxis, and user-defined signal processing</li>
</ol>

As a Software Development Kit (SDK), Github SigSRF software includes Makefiles and header files (.h files) needed to build modified reference applications and generate new, user-defined applications.

Github SigSRF software also includes binary codes for shared object libraries used in SigSRF Software, including but not limited to:

<ol type="a">
  <li>pktlib (packet library)</li>
  <li>voplib (voice-over-packet library)</li>
  <li>codec libraries including EVS, AMR, and AMR-WB, AMR-WB-Plus, G729, G726, MELPe</li>
  <li>streamlib (stream group library)</li>
</ol>

The above listed binary shared object libraries are limited in capacity or other manner of restricted functionality.

### 1.2 License

Subject to the terms and conditions of this Agreement, Signalogic hereby grants to Licensee a non-exclusive, royalty-free, worldwide, non-transferable, 
non-sublicenseable license during the term of this Agreement to: (a) use Github SigSRF Software for demo, evaluation, R&D, test and measurement,
and limited commercial purposes; (b) prepare modifications and derivative works of Github SigSRF Software; (c) distribute Github SigSRF Software (including 
without limitation in source code or object code form); and (d) reproduce copies of Github SigSRF Software (the “License”). Licensee is not granted the right to, 
and Licensee shall not, exercise the License for an Excluded Purpose.

For purposes of this Agreement, "Excluded Purpose" means making available any (i) server based platform, including but not limited to bare-metal, container, and VM, 
or (ii) online service, including but not limited to software-as-a-service, platform-as-a-service, and infrastructure-as-a-service, that compete(s) with Signalogic products or services incorporating SigSRF Software functionality.

### 1.3 Conditions

In consideration of the License, Licensee’s distribution of Github SigSRF Software is subject to the following conditions:

<ol type="a">
  <li>Licensee must cause any Github SigSRF Software modified by Licensee to carry prominent notices stating that Licensee modified Github SigSRF Software.</li>

  <li>On each Github SigSRF Software copy, Licensee shall reproduce and not remove or alter all Signalogic or third party copyright or other proprietary notices 
contained in Github SigSRF Software, and Licensee must provide the notice below with each copy.</li>
</ol>

“This software is made available by Signalogic, Inc., under the terms of the Signalogic Github SigSRF License Agreement, Version 1.0 located at 
https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md. BY INSTALLING, DOWNLOADING, ACCESSING, USING OR DISTRIBUTING ANY OF GITHUB SIGSRF SOFTWARE, YOU 
AGREE TO THE TERMS OF SUCH LICENSE AGREEMENT.”

### 1.4 Licensee Modifications

Licensee may add its own copyright notices to modifications made by Licensee and may provide additional or different license terms and conditions for use, 
reproduction, or distribution of Licensee’s modifications. While redistributing Github SigSRF Software or modifications thereof, Licensee may choose to offer, for 
a fee or free of charge, support, warranty, indemnity, or other obligations. Licensee, and not Signalogic, will be responsible for any such obligations.

### 1.5 No Sublicensing

The License does not include the right to sublicense Github SigSRF Software, however, each recipient to which Licensee provides Github SigSRF Software may 
exercise the Licenses so long as such recipient agrees to the terms and conditions of this Agreement.

## 2 TERM AND TERMINATION

This Agreement will continue unless and until earlier terminated as set forth herein. If Licensee breaches any of its conditions or obligations under this Agreement, 
this Agreement will terminate automatically and the License will terminate automatically and permanently.

## 3 INTELLECTUAL PROPERTY

As between the parties, Signalogic will retain all right, title, and interest in Github SigSRF Software, and all intellectual property rights therein. Signalogic 
hereby reserves all rights not expressly granted to Licensee in this Agreement. Signalogic hereby reserves all rights in its trademarks and service marks, and no 
licenses therein are granted in this Agreement.

## 4 DISCLAIMER

SIGNALOGIC HEREBY DISCLAIMS ANY AND ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, AND SPECIFICALLY DISCLAIMS ANY WARRANTY OF 
MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, WITH RESPECT TO GITHUB SIGSRF SOFTWARE.

## 5 LIMITATION OF LIABILITY

SIGNALOGIC WILL NOT BE LIABLE FOR ANY DAMAGES OF ANY KIND, INCLUDING BUT NOT LIMITED TO, LOST PROFITS OR ANY CONSEQUENTIAL, SPECIAL, INCIDENTAL, INDIRECT, OR DIRECT 
DAMAGES, HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, ARISING OUT OF THIS AGREEMENT. THE FOREGOING SHALL APPLY TO THE EXTENT PERMITTED BY APPLICABLE LAW.

## 6 GENERAL

### 6.1  Governing Law

This Agreement will be governed by and interpreted in accordance with the laws of the state of Texas, without reference to its conflict of laws principles. If 
Licensee is located within the United States, all disputes arising out of this Agreement are subject to the exclusive jurisdiction of courts located in Dallas County, 
Texas, USA. If Licensee is located outside of the United States, any dispute, controversy or claim arising out of or relating to this Agreement will be referred to 
and finally determined by arbitration in accordance with the JAMS International Arbitration Rules. The tribunal will consist of one arbitrator. The place of 
arbitration will be Dallas, Texas. The language to be used in the arbitral proceedings will be English. Judgment upon the award rendered by the arbitrator may be 
entered in any court having jurisdiction thereof.

### 6.2  Assignment

Licensee is not authorized to assign its rights under this Agreement to any third party. Signalogic may freely assign its rights under this Agreement to any third 
party.

### 6.3  Other

This Agreement is the entire agreement between the parties regarding the subject matter hereof. No amendment or modification of this Agreement will be valid or 
binding upon the parties unless made in writing and signed by the duly authorized representatives of both parties. In the event that any provision, including without 
limitation any condition, of this Agreement is held to be unenforceable, this Agreement and all licenses and rights granted hereunder will immediately terminate. 
Waiver by Signalogic of a breach of any provision of this Agreement or the failure by Signalogic to exercise any right hereunder will not be construed as a waiver 
of any subsequent breach of that right or as a waiver of any other right.
