# co-CPU SDK Overview

The co-CPU SDK introduces the co-CPU approach to HPC servers.  Unlike GPUs, DSPs, FPGAs, ASICS, and other specialized chips, co-CPUs can be added to 1U, 2U, and mini-ITX servers to create HPC and AI servers with 100s of CPU cores, and still maintain Linux compatibility, software ease-of-use, and low power consumption. In a KVM + QEMU virtualized environment, cores and network I/O interfaces appear as resources that can be allocated as needed to VMs.  In a bare-metal environment, the pool of cores can be shared by users as needed.

## co-CPU Software Download

The co-CPU download consists of an install script and .rar file containing:
  
    1) A limited trial version of the co-CPU software stack, including DirectCore libraries and drivers.

    2) HPC application demos and test programs, including C/C++ source code and Makefiles.  Demos include
       H.264 video streaming (ffmpeg acceleration), image analytics, and high capacity telecom transcoding.
       Multiple users can share a co-CPU card, as noted above.
    
To run the demo programs, you will need one of the co-CPU cards shown on <a href="http://processors.wiki.ti.com/index.php/HPC" target="_blank"> the TI HPC page</a>.  co-CPU cards can be obtained from TI, Advantech, or Signalogic.

## Installation Notes

Separate RAR packages are provided for different Linux distributions. Please choose the appropriate one or closest match.The install script will auto-check for kernel version and Linux distro version to decide which drivers to install.

All .rar files and the auto install script must stay together in the same folder after downloading.

Note that the install script checks for the presence of the unrar command, and if not found it will install the "UnRar" package.

### Sudo Privilege

The install script requires sudo root privilege.  In Ubuntu, allowing a user sudo root privilege can be done by adding the user to the “administrators” group (<a href=http://askubuntu.com/questions/168280/how#do#i#grant#sudo#privileges#to#an#existing#user target="_blank">as shown here</a>).  In CentOS a user can be added to the “/etc/sudoers” file (<a href="https://wiki.centos.org/TipsAndTricks/BecomingRoot" target="_blank">as shown here</a>).

### Building Test and Demo Applications

Test and demo application examples are provided as C/C++ source code and Makefiles, and must be built using gcc before they can be run.  To allow this, the install script also checks for the presence of the following run-time and build related packages:  gcc, ncurses, lib-explain, and redhat-lsb-core (RedHat and CentOS) and lsb-core (Ubuntu).  These are installed if not found.

### Running the Install Script

To run the install script: 

    • sh autoInstall_Sig_BSDK_2017v2.sh
 
The following command line options may be used:



### co-CPU Users Guide

The <a href = "http://goo.gl/Vs1b3R" target="_blank")>co-CPU User Guide</a> provides detailed information about co-CPU and software installation, test and demo applications, build instructions, etc.
