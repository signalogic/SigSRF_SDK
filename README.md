# SigSRF SDK Overview

The SigSRF (Streaming Resource Functions) SDK introduces a scalable approach to media, HPC, and AI servers.  SigSRF software is x86 Linux centric, with *expandable performance* per box using coCPU&trade; technology, which adds NICs and up to 100s of CPU cores to increase per-box streaming and performance density.  coCPUs can turn conventional 1U, 2U, and mini-ITX servers into high capacity media, HPC, and AI servers, supporting OpenCV, TensorFlow, media transcoding, speech recognition, and other high performance applications -- and do so without resorting to GPUs, FPGAs, and ASICs that increase server size and power consumption, or dictate an ARM based or other non-cloud compatible software architecture.

SigSRF supports concurrent multiuser operation in a a bare-metal environment, and in a KVM + QEMU virtualized environment, cores and network I/O interfaces appear as resources that can be allocated between VMs. VM and host users can share also, as the available pool of cores is handled by a physical layer back-end driver. This flexibility allows media, HPC, and AI applications to scale between cloud, enterprise, and remote vehicle/location servers.

## SigSRF SDK Download

The SigSRF SDK download consists of an install script and .rar files and includes:
  
    1) A limited eval / demo version of several SigSRF demos, including C/C++ source code and Makefiles.  Demos include
       H.264 video streaming (ffmpeg acceleration), image analytics, and high capacity telecom transcoding
    
    2) coCPU software stack, including DirectCore libraries and drivers (requires a coCPU card to be active)

    3) Multiple concurrent user/instance demonstration
    
To run coCPU specific demo programs, you will need one of the coCPU cards <a href="http://processors.wiki.ti.com/index.php/HPC" target="_blank">shown here</a>.  coCPU cards can be obtained from TI, Advantech, or Signalogic.

## Installation Notes

Separate RAR packages are provided for different Linux distributions. Please choose the appropriate one or closest match. The install script will auto-check for kernel version and Linux distro version to decide which co-CPU drivers to install.

All .rar files and the auto install script must stay together in the same folder after downloading.

Note that the install script checks for the presence of the unrar command, and if not found it will install the unrar package.

### Sudo Privilege

The install script requires sudo root privilege.  In Ubuntu, allowing a user sudo root privilege can be done by adding the user to the “administrators” group (<a href=http://askubuntu.com/questions/168280/how#do#i#grant#sudo#privileges#to#an#existing#user target="_blank">as shown here</a>).  In CentOS a user can be added to the “/etc/sudoers” file (<a href="https://wiki.centos.org/TipsAndTricks/BecomingRoot" target="_blank">as shown here</a>).  Please make sure this is the case before running the script.

### Building Test and Demo Applications

Test and demo application examples are provided as C/C++ source code and Makefiles, and must be built using gcc before they can be run.  To allow this, the install script checks for the presence of the following run-time and build related packages:  gcc, ncurses, lib-explain, and redhat-lsb-core (RedHat and CentOS) and lsb-core (Ubuntu).  These are installed if not found.

## Running the Install Script

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

## SigMRF Users Guide

SigMRF (Media Resource Functions) software is part of SigSRF software. The <a href="http://goo.gl/Vs1b3R" target="_blank">SigMRF User Guide</a> provides detailed information about SigMRF software installation, test and demo applications, build instructions, etc.

## coCPU Users Guide

The <a href="http://goo.gl/Vs1b3R" target="_blank">coCPU User Guide</a> provides detailed information about coCPU and software installation, test and demo applications, build instructions, etc.

## Technical Support

Tech support for the co-CPU SDK is available from Signalogic at no charge via e-mail and Skype.
