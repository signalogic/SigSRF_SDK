# co-CPU SDK Overview

The co-CPU SDK introduces the co-CPU approach to HPC servers.  Unlike GPUs, DSPs, FPGAs, ASICS, and other specialized chips, co-CPUs can be added to 1U, 2U, and mini-ITX servers to create HPC and AI servers with 100s of CPU cores, and still maintain Linux compatibility, software ease-of-use, and low power consumption. In a KVM + QEMU virtualized environment, cores and network I/O interfaces appear as resources that can be allocated as needed to VMs.  In a bare-metal environment, the pool of cores can be shared by users as needed.

## co-CPU Software Download

The co-CPU download consists of an install script and .rar file containing:
  
    1) A limited trial version of the co-CPU software stack, including DirectCore libraries and drivers.

    2) HPC application demos and test programs, including C/C++ source code.
    
To run the basic programs, you will need one of the co-CPU cards shown here http://processors.wiki.ti.com/index.php.   co-CPU cards can be obtained from TI, Advantech, or Signalogic.

## Installation Notes

Separate RAR packages are provided for different Linux distributions. The install script will auto-check for kernel version and Linux distro version to decide which drivers to install.

The install script requires superuser privileges and will require you to log in as root. To run the script, use the command below: 

    • sh autoInstall_Sig_BSDK_2017v2.sh
 
The .rar files and auto install script file must stay together in the same folder after downloading.
 
The install script checks for the presence of the unrar command, and if not found it installs the "UnRar" package.

The install script also checks for the presence of the following packages:  gcc, ncurses, lib-explain, and redhat-lsb-core (RedHat and CentOS) and lsb-core (Ubuntu).  These are installed if not found.  These packages are required to build the test and demo application examples (provided as C/C++ source code).

The <a href ="ftp://ftp.signalogic.com/documentation/Hardware/SigC667x/SigC667x_UserGuide_RevD2.pdf">co-CPU User Guide</a> provides detailed information about co-CPU and software installation, test and demo applications, build instructions, etc.
