# co-CPU&trade; SDK Overview

The co-CPU&trade; SDK introduces the co-CPU approach to HPC and AI servers.  Unlike GPUs, DSPs, FPGAs, ASICs, and other specialized chips, co-CPUs can be added to 1U, 2U, and mini-ITX servers to create HPC and AI servers with 100s of CPU cores, and still maintain Linux compatibility, software ease-of-use, and low power consumption. In a bare-metal environment, concurrent multiuser operation is supported. In a KVM + QEMU virtualized environment, cores and network I/O interfaces appear as resources that can be allocated between VMs. VM and host users can share also, as the available pool of cores is handled by a physical layer back-end driver. This flexibility allows AI and HPC applications to scale between cloud, enterprise, and remote vehicle/location servers.

## co-CPU&trade; Software Download

The co-CPU SDK download consists of an install script and .rar file containing:
  
    1) A limited eval / demo version of the co-CPU software stack, including DirectCore libraries and drivers.

    2) HPC application demos and test programs, including C/C++ source code and Makefiles.  Demos include
       H.264 video streaming (ffmpeg acceleration), image analytics, and high capacity telecom transcoding.
       Multiple users can share a co-CPU card, as noted above.
    
To run the demo programs, you will need one of the co-CPU cards <a href="http://processors.wiki.ti.com/index.php/HPC" target="_blank">shown here</a>.  co-CPU cards can be obtained from TI, Advantech, or Signalogic.

## Installation Notes

Separate RAR packages are provided for different Linux distributions. Please choose the appropriate one or closest match. The install script will auto-check for kernel version and Linux distro version to decide which drivers to install.

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

    1) Install co-CPU software
    2) Uninstall co-CPU software
    3) Check / Verify
    4) Exit
    Please select install operation to perform [1-4]:
  
If the install operation (1.) is selected, the script will prompt for an install path:

    Enter the path for co-CPU software installation:

If no path is entered the default path is /usr/local.

If needed, the Check / Verify option can be selected to generate a log for troubleshooting and tech support purposes.

## co-CPU Users Guide

The <a href = "http://goo.gl/Vs1b3R" target="_blank")>co-CPU User Guide</a> provides detailed information about co-CPU and software installation, test and demo applications, build instructions, etc.

## Technical Support

Tech support for the co-CPU SDK is available from Signalogic at no charge via e-mail and Skype.
