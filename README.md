# HPC_SDK
This SDK (Software Development Kit) folder contains:

1) A limited trial version of DirectCore software

2) Several HPC application demos and test programs, including C/C++ source code

The SDK requires one of the PCIe cards listed shown here (http://processors.wiki.ti.com/index.php/HPC) to be installed in a Linux server. Linux distributions supported include:

    • Ubuntu

    • RHEL

    • CentOS

Virtualized demos require KVM + QEMU to be installed

#Install Scripts
autoInstall_Sig_BSDK_XXX.sh script is used to install Signalogic_sw_host_DirectCore_XXX.rar package

Run the script, and it will prompt blow shown options to choose from:

    1) Signalogic SW installation

    2) Uninstall Signalogic SW

    3) Signalogic Install Check

    4) exit

•	Option 1) will perform a full Signalogic software installation. On choosing this option, the install script will ask for software installation path and then proceed

•	Option 2) will perform a software uninstall.This option can be used for a full software uninstall; it may also be useful in case of errors or a need to repeat the installation process

•	Option 3) This option is for diagnostic purpose. if there is any issue in sw installation or using Signalogic module etc..., user can choose this option whoch will create DirectCore_diagnostic_report_XXX.txt showing sucessfully/unsuccessfully installed software parts that can help debug the issue

#RAR Packages
There are seperate RAR packages has been provided for different Operating Systems and thier versions for example,

RHEL_7.1 (Red Hat version 7.1)

Ubuntu_14.04.4_Trusty (Ubuntu 14.04)

**NOTE: User needs to download and use suitable version of rar package (depending on thier OS) with the auto install script**
