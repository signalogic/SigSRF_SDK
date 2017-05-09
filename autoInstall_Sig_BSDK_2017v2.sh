#!/bin/bash
#================================================================================================
# Copyright (C) Signalogic Inc 2017
# Script provides Binary SDK install/uninstall of Signalogic SW
# Rev 1.0

	# Requirements
		# Internet connection
		# Install unrar package
			# For Ubuntu, use command:
				# apt-get install unrar
			# For RHEL/CentOS,
				# yum -y install epel-release && rpm -Uvh http://li.nux.ro/download/nux/dextop/el7/x86_64/nux-dextop-release-0-5.el7.nux.noarch.rpm
				# yum install unrar

# Revision History
# Created:  Feb, 2016 by HP
# Modified: Feb, 2017 by AM
#================================================================================================

packageSetup() {			# This func. prompts for Signalogic installation path, extarct package, set envionment var

	unrarInstalled=`type -p unrar`
	if [ ! $unrarInstalled ]; then
		echo "Need to install unrar..."
		exit
	fi
	
	echo "Enter the path for SigSRF software and dependency package installation:"
	read installPath
	if [ $installPath ]; then
		echo "Install path: $installPath"
	else
		installPath="/usr/local"
		echo "Default Install path (/) is chosen"
	fi
	
	unrar x Signalogic_sw_host_DirectCore_BSDK*.rar $installPath/
	unrar x Signalogic_sw_host_video_files*.rar $installPath/

}

depInstall () {

	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
			yum localinstall $line
	elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
		dpkg -i $line
		if [ $? -gt 0 ]; then
			apt-get -f --force-yes --yes install
		fi
	fi
	
}
dependencyCheck() {			# It will check for generic non-Signalogic SW packages and prompt for installation if not installed
	
	DOTs='................................................................'
	
	if [ "$opt" = "Install SigSRF software" ]; then
		dependencyInstall="Dependency Check + Install"
	elif [ "$opt" = "Dependency Check" ]; then
		installPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
		dependencyInstall="Dependency Check"
		
		if [ ! $installPath ]; then
			echo 
			echo "SigSRF software install path could not be found."
			echo "Exiting..."
			echo
			exit
		fi
	fi
	
	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
	{
		if [ "$dependencyInstall" = "Dependency Check + Install" ]; then
			package=$(rpm -qa gcc-c++)
			if [ ! $package ]; then
			        echo -e "gcc compiler and toolchain is needed\n"
				yum install gcc-c++
			fi
			
                	unrarInstalled=`type -p lsb_release`
	                if [ ! $unrarInstalled ]; then
		  		echo "lsb_release package is needed"
				yum install redhat-lsb-core
			fi
                         
                        yum install libpcap-devel
		fi
		cd $installPath/Signalogic_2017v6/installation_rpms/RHEL
		filename="rhelDependency.txt"
		while read -r -u 3 line
		do
		
		d=$(sed 's/.rpm//g' <<< $line)
		package=`rpm -qa | grep -w $d | head -n1`
		if [ ! $package ]; then
			if [ "$dependencyInstall" = "Dependency Check + Install" ]; then
				if [ ! $totalInstall ]; then
					echo "Do you wish to install $d package?"
					read -p "Please insert [Y]ES, [N]O, [Aa]ll: " Dn
						if [[ ($Dn = "a") || ($Dn = "A") ]]; then
							totalInstall=1
						fi
				fi
				case $Dn in
					[YyAa]* ) depInstall ; ;;
					[Nn]* ) ;;
					* ) echo "Please retry with just *Yy/Aa* or *Nn*";;
				esac
			elif [ "$dependencyInstall" = "Dependency Check" ]; then
				printf "%s %s[ NOT INSTALLED ]\n" $d "${DOTs:${#d}}"
			fi
		else
			printf "%s %s[ ALREADY INSTALLED ]\n" $d "${DOTs:${#d}}"
		fi
		done 3< "$filename"
	}
	
	elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
	{
		if [ "$dependencyInstall" = "Dependency Check + Install" ]; then
			package=$(dpkg -s g++-4.8 2>/dev/null | grep Status | awk ' {print $4} ')
			if [ ! $package ]; then
				package=$(dpkg -s g++ 2>/dev/null | grep Status | awk ' {print $4} ')
			fi
			
			if [ ! $package ]; then
				apt-get -y --purge remove gcc g++ gcc-4.8 g++-4.8
				unlink /usr/bin/gcc
				unlink /usr/bin/g++
			fi
		fi
		cd $installPath/Signalogic*/installation_rpms/Ubuntu
		filename="UbuntuDependency.txt"
		while read -r -u 3 line
		do
		d=$(sed 's/_.*//g' <<< $line)
		package=$(dpkg -s $d 2>/dev/null | grep Status | awk ' {print $4} ')
		if [ ! $package ]; then
			if [ "$dependencyInstall" = "Dependency Check + Install" ]; then
				if [ ! $totalInstall ]; then
					read -p "Do you wish to install $d packages? Please insert [Y]ES, [N]O, [Aa]ll: " Dn
						if [[ ($Dn = "a") || ($Dn = "A") ]]; then
							totalInstall=1
						fi
				fi
				case $Dn in
					[YyAa]* ) depInstall ; ;;
					[Nn]* ) ;;
					* ) echo "Please retry with just *y* or *n*";;
				esac
			elif [ "$dependencyInstall" = "Dependency Check" ]; then
				printf "%s %s[ NOT INSTALLED ]\n" $d "${DOTs:${#d}}"
			fi
		elif [ $package ]; then
			printf "%s %s[ ALREADY INSTALLED ]\n" $d "${DOTs:${#d}}"
		fi
		done 3< "$filename"
		
		if [ "$dependencyInstall" = "Dependency Check + Install" ]; then
			# Dependencies gcc and g++ will be installed as gcc-4.8 and g++-4.8 so it is necessary to create a symmlink (gcc and g++) otherwise SW installation might be fail
			if [ ! -L  /usr/bin/gcc ]; then
                        	ln -s /usr/bin/gcc-4.8 /usr/bin/gcc
                        fi
			if [ ! -L  /usr/bin/g++ ]; then
				ln -s /usr/bin/g++-4.8 /usr/bin/g++
                        fi
		fi
	}
	fi
	
}

swInstall() {				# It will install Signalogic SW on specified path

	# Set up environment var permanently
	export SIGNALOGIC_INSTALL_PATH=$installPath 
	echo "SIGNALOGIC_INSTALL_PATH=$installPath" >> /etc/environment
	
	echo
	echo "SigSRF software Installation will be performed..."
	mv $installPath/Signalogic_*/etc/signalogic /etc
	rm -rf $installPath/Signalogic*/etc
	echo
	kernel_version=`uname -r`
	echo $kernel_version
	echo "Creating symlinks..."
	
	if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
		if [ ! -L  /usr/src/linux ]; then
			ln -s /usr/src/kernels/$kernel_version /usr/src/linux
		fi 
	elif [ "$OS" = "Ubuntu" ]; then
		if [ ! -L  /usr/src/linux ]; then
			ln -s /usr/src/linux-headers-$kernel_version /usr/src/linux
		fi
	fi
		
        if [ ! -L  $installPath/Signalogic ]; then
		ln -s $installPath/Signalogic_* $installPath/Signalogic
        fi
        if [[ ! -L   $installPath/Signalogic_*/DirectCore/apps/SigC641x_C667x ]]; then
		ln -s  $installPath/Signalogic_*/DirectCore/apps/SigC641x_C667x $installPath/Signalogic_2017v6/DirectCore/apps/coCPU 
        fi
	echo	
	echo "loading driver"
	if [ "$target" = "Host" ]; then
		#cd $installPath/Signalogic_*/DirectCore/hw_utils; make clean; make
		cd $installPath/Signalogic_*/DirectCore/hw_utils; make
		cd ../driver; 
                distribution=$(lsb_release -d)
                kernel=$(uname -r) 
                if [[ $kernel == 3.2.0-49-generic ]]; then
                   cp sig_mc_hw_ubuntu_12.04.5.ko sig_mc_hw.ko
                elif [[ $kernel == 3.16.0-67-generic ]]; then
                   cp sig_mc_hw_ubuntu_14.04.4.ko sig_mc_hw.ko
                elif [[ $kernel == 4.4.0-31-generic ]]; then
                   cp sig_mc_hw_ubuntu_14.04.5.ko sig_mc_hw.ko
                elif [[ $kernel == "4.4.0-59-generic" ]]; then
                   cp sig_mc_hw_ubuntu_16.04.1.ko sig_mc_hw.ko
                elif [[ $distribution == *12.04.5* ]]; then
                   cp sig_mc_hw_ubuntu_12.04.5.ko sig_mc_hw.ko
                elif [[ $distribution == *14.04.4* ]]; then
                   cp sig_mc_hw_ubuntu_14.04.4.ko sig_mc_hw.ko
                elif [[ $distribution == *14.04.5* ]]; then
                   cp sig_mc_hw_ubuntu_14.04.5.ko sig_mc_hw.ko
                elif [[ $distribution == *16.04.1* ]]; then
                   cp sig_mc_hw_ubuntu_16.04.1.ko sig_mc_hw.ko
                fi
	        if lsmod | grep sig_mc_hw &> /dev/null ; then
			echo "Signalogic Driver is loaded"
  		else		
                	make load
		fi
	elif [ "$target" = "VM" ]; then
		cd $installPath/Signalogic_*/DirectCore/virt_driver; make load
	fi
	
	echo  "Setting up autoload of Signalogic driver on boot"
	
	if [ "$target" = "Host" ]; then
		if [ ! -f /lib/modules/$kernel_version//sig_mc_hw.ko ]; then
			ln -s $installPath/Signalogic/DirectCore/driver/sig_mc_hw.ko /lib/modules/$kernel_version
		fi
	elif [ "$target" = "VM" ]; then
		if [ ! -L /lib/modules/$kernel_version ]; then
			ln -s $installPath/Signalogic/DirectCore/virt_driver/virtio-sig.ko /lib/modules/$kernel_version
		fi
	fi
	depmod -a
	if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
		cp $installPath/Signalogic/DirectCore/driver/sig_mc_hw.modules /etc/sysconfig/modules/
		chmod 755 /etc/sysconfig/modules/sig_mc_hw.modules
		echo "chmod 666 /dev/sig_mc_hw" >> /etc/rc.d/rc.local
		chmod 755 /etc/rc.d/rc.local
	elif [ "$OS" = "Ubuntu" ]; then
		echo "sig_mc_hw" >> /etc/modules
		sed -i '/exit*/d' /etc/rc.local
		echo "chmod 666 /dev/sig_mc_hw" >> /etc/rc.local
		echo "exit 0" >> /etc/rc.local
		chmod 755 /etc/rc.local
	fi
	echo
	echo "Building Signalogic libraries..."
	cd $installPath/Signalogic_*/DirectCore/lib/
	for d in *; do
		#if [[ "$d" != "voplib" ]]; then
			cd $d; cp lib* /usr/lib; ldconfig; cd -
		#fi
	done
	cd $installPath/Signalogic_*/DirectCore/apps/coCPU/
	for d in *; do
	if [[ "$d" != "appTest" ]]; then
			cd $d; make clean; make; cd -
		fi
	done
	cd $wdPath
	
}

unInstall() {			# It will uninstall Signalogic SW completely

	OS=$(cat /etc/os-release | grep -w NAME=* | sed -n -e '/NAME/ s/.*\= *//p' | sed 's/"//g')
	echo
	echo "Signalogic SW uninstallation will be performed..."
	echo
	unInstallPath=$SIGNALOGIC_INSTALL_PATH
	if [ ! $unInstallPath ]; then
		unInstallPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
		if [ ! $unInstallPath ]; then
			echo 
			echo "Signalogic install path could not be found."
			echo "Exiting..."
			echo
			exit
		fi
	fi
	
	echo "Signalogic Install Path: $unInstallPath"
	rm -rf $unInstallPath/Signalogic*
	MODEXIST=/sbin/lsmod | grep "sig_mc_hw"
	if [ -n "$MODEXIST" ]; then
	   rmmod sig_mc_hw
    fi
	rm -rf /etc/signalogic
	unlink /usr/src/linux
	
	if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
		rm -rf /etc/sysconfig/modules/sig_mc_hw.modules
	fi
	
	kernel_version=`uname -r`
	
		if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
			if [ $target = "Host" ]; then
				rm -rf /usr/lib/modules/$kernel_version/sig_mc_hw.ko
			elif [ $target = "VM" ]; then
				rm -rf /usr/lib/modules/$kernel_version/virtio-sig.ko
			fi
		elif [ "$OS" = "Ubuntu" ]; then
			if [ $target = "Host" ]; then
				rm -rf /lib/modules/$kernel_version/sig_mc_hw.ko
			elif [ $target = "VM" ]; then
				rm -rf /lib/modules/$kernel_version/virtio-sig.ko
			fi
		fi
	
	if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
		sed -i '/chmod 666 \/dev\/sig_mc_hw/d' /etc/rc.d/rc.local 
	elif [ "$OS" = "Ubuntu" ]; then
		sed -i '/chmod 666 \/dev\/sig_mc_hw/d' /etc/rc.local
	fi
	
	rm -rf /usr/lib/libcallmgr.a
	rm -rf /usr/lib/libcimlib.so
	rm -rf /usr/lib/libenmgr.a
	rm -rf /usr/lib/libfilelib.a
	rm -rf /usr/lib/libhwlib.so
	rm -rf /usr/lib/libhwmgr.a
	rm -rf /usr/lib/libtdmlib.a
	rm -rf /usr/lib/libevscom_sig.x86.so
	rm -rf /usr/lib/libevsdec_sig.x86.so
	rm -rf /usr/lib/libevsenc_sig.x86.so
	rm -rf /usr/lib/libpktlib.so
	rm -rf /usr/lib/libvoplib.so
	unset SIGNALOGIC_INSTALL_PATH
	sed -i '/SIGNALOGIC_INSTALL_PATH*/d' /etc/environment
	echo "Uninstall complete..."
	
}

installCheck () {

line='................................................................'

	installPath=$SIGNALOGIC_INSTALL_PATH
	if [ ! $installPath ]; then
		installPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
		if [ ! $installPath ]; then
			echo 
			echo "Signalogic install path could not be found."
			echo "Exiting..."
			echo
			exit
		fi
	fi

current_time=$(date +"%m.%d.%Y-%H:%M:%S")
diagReportFile=DirectCore_diagnostic_report_$current_time.txt
touch $diagReportFile

# Signalogic Module Check#

echo | tee -a $diagReportFile
echo "Signalogic Module Check:" | tee -a $diagReportFile
d="sig_mc_hw"
if [ -c /dev/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

# Symlinks Check #

echo >>$diagReportFile
echo "Symlinks Check:">>$diagReportFile

d="Signalogic_Symlink"
if [ -L $installPath/Signalogic ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="Linux_Symlink"
if [ -L /usr/src/linux ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

# Signalogic Library Install Check #

echo | tee -a $diagReportFile
echo "Signalogic Library Install Check:" | tee -a $diagReportFile
d="cimlib"
if [ -f /usr/lib/libcimlib.so ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="hwlib"
if [ -f /usr/lib/libhwlib.so ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="libhwmgr"
if [ -f /usr/lib/libhwmgr.a ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="filelib"
if [ -f /usr/lib/libfilelib.a ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile 
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="libenmgr"
if [ -f /usr/lib/libenmgr.a ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="libcallmgr"
if [ -f /usr/lib/libcallmgr.a ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="libtdm"
if [ -f /usr/lib/libtdmlib.a ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

# Signalogic App Executables Check #

echo | tee -a $diagReportFile
echo "Signalogic App Executables Check:" | tee -a $diagReportFile
d="memTest"
if [ -f $installPath/Signalogic/DirectCore/apps/coCPU/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="boardTest"
if [ -f $installPath/Signalogic/DirectCore/apps/coCPU/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="iaTest"
if [ -f $installPath/Signalogic/DirectCore/apps/coCPU/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="streamTest"
if [ -f $installPath/Signalogic/DirectCore/apps/coCPU/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="fftTest"
if [ -f $installPath/Signalogic/DirectCore/apps/coCPU/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="videoTest"
if [ -f $installPath/Signalogic/DirectCore/apps/coCPU/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

# Checks Leftover HW Lib Files under /dev/shm

echo | tee -a $diagReportFile
echo "Leftover HW Lib Files Check:" | tee -a $diagReportFile
d="hwlib_mutex"
if [ -f /dev/shm/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="hwlib_info"
if [ -f /dev/shm/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

}

#*************************************************************************************************************
wdPath=$PWD
OS=$(cat /etc/os-release | grep -w NAME=* | sed -n -e '/NAME/ s/.*\= *//p' | sed 's/"//g')
echo "Host Operating System: $OS"
PS3="Please select target for SigSRF software install [1-2]: "
select target in "Host" "VM" 
do
	case $target in
		"Host") break;;
		"VM") break;;
	esac
done
echo "*****************************************************"
echo
PS3="Please select install operation to perform [1-4]: "
select opt in "Install SigSRF software" "Uninstall SigSRF software" "SigSRF software Install Check" "exit"
do
    case $opt in
		"Install SigSRF software") packageSetup; dependencyCheck; swInstall; break;;
		"Uninstall SigSRF software") unInstall; break;;
		"SigSRF software Install Check") installCheck; break;;
		"exit") echo "Exiting..."; break;;
      *) echo invalid option;;
    esac
done
