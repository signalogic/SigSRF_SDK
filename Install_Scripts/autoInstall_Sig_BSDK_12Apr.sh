#!/bin/bash

#============================================
# Copyright (C) Signalogic Inc 2014
# Script provides full install/uninstall of Signalogic SW
# Rev 1.0
# Revision History
	# Added options:
	# It will ask if target system is a Host or a VM, and then it will offer following options,
	
	# 1. Binary SDK version of Signalogic SW which includes binary version of Signalogic driver and libraries
	# 2. Uninstall of Signalogic SW
	# 3. Signalogic install check
	# 4. Exit the script
# Created: March, 2016 by HP
#============================================

packageSetup() {			# This func. prompts for Signalogic installation path, extarct package, set envionment var

	unrarInstalled=`type -p unrar`
	if [ ! $unrarInstalled ]; then
		echo "Need to install unrar..."
		exit
	fi
	
	echo "Enter the path for Signalogic SW and dependency package installation:"
	read installPath
	if [ $installPath ]; then
		echo "Install path: $installPath"
	else
		installPath="/"
		echo "Default Install path (/) is chosen"
	fi
	
	# Set up env. var permanently
	export SIGNALOGIC_INSTALL_PATH=$installPath 
	echo "SIGNALOGIC_INSTALL_PATH=$installPath" >> /etc/environment
	
	unrar x Signalogic_sw_host_DirectCore_BSDK*.rar $installPath/
	unrar x Signalogic_sw_host_video_files*.rar $installPath/

}

dependencyCheck() {			# It will check for generic non-Signalogic SW packages and prompt for installation if not installed
	
	DOTs='................................................................'
	
	if [ "$opt" = "Signalogic SW installation" ]; then
		dependencyInstall="Dependency Check + Install"
	elif [ "$opt" = "Dependency Check" ]; then
		installPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
		dependencyInstall="Dependency Check"
		
		if [ ! $installPath ]; then
			echo 
			echo "Signalogic install path could not be found."
			echo "Exiting..."
			echo
			exit
		fi
	fi
	
	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
	{

		cd $installPath/Signalogic_2014v5/installation_rpms/RHEL
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
		if [ $dependencyInstall = "Dependency Check + Install" ]; then
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
			if [ $dependencyInstall = "Dependency Check + Install" ]; then
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
			elif [ $dependencyInstall = "Dependency Check" ]; then
				printf "%s %s[ NOT INSTALLED ]\n" $d "${DOTs:${#d}}"
			fi
		elif [ $package ]; then
			printf "%s %s[ ALREADY INSTALLED ]\n" $d "${DOTs:${#d}}"
		fi
		done 3< "$filename"
		
		if [ $dependencyInstall = "Dependency Check + Install" ]; then
			# Dependencies gcc and g++ will be installed as gcc-4.8 and g++-4.8 so it is necessary to create a symmlink (gcc and g++) otherwise SW installation might be fail
			ln -s /usr/bin/gcc-4.8 /usr/bin/gcc
			ln -s /usr/bin/g++-4.8 /usr/bin/g++
		fi
	}
	fi
	
}

depInstall () {

	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
		rpm -Uvh $line
	elif [ "$target" = "VM" -o $OS = "Ubuntu" ]; then
		dpkg -i $line
	fi
	
}

swInstall() {				# It will install Signalogic SW on specified path

	echo
	echo "Signalogic SW Installation will be performed..."
	mv $installPath/Signalogic*/etc/signalogic /etc
	rm -rf $installPath/Signalogic*/etc
	echo
	kernel_version=`uname -r`
	echo $kernel_version
	echo "Creating symlinks..."
	
	if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
		ln -s /usr/src/kernels/$kernel_version /usr/src/linux 
	elif [ "$OS" = "Ubuntu" ]; then
		ln -s /usr/src/linux-headers-$kernel_version /usr/src/linux
	fi
	
	ln -s $installPath/Signalogic_* $installPath/Signalogic
	echo
	
	echo "loading driver"
	if [ "$target" = "Host" ]; then
		cd $installPath/Signalogic_*/DirectCore/hw_utils; make clean; make
		cd ../driver; make load
	elif [ "$target" = "VM" ]; then
		cd $installPath/Signalogic_*/DirectCore/virt_driver; make load
	fi
	
	echo  "Setting up autoload of Signalogic driver on boot"
	
	if [ "$target" = "Host" ]; then
		ln -s $installPath/Signalogic/DirectCore/driver/sig_mc_hw.ko /lib/modules/$kernel_version
	elif [ "$target" = "VM" ]; then
		ln -s $installPath/Signalogic/DirectCore/virt_driver/virtio-sig.ko /lib/modules/$kernel_version
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
		if [[ "$d" != "voplib" ]]; then
			cd $d; cp lib* /usr/lib; ldconfig; cd -
			#cd $d; make clean; make; cd -
		fi
	done
	cd $installPath/Signalogic_*/DirectCore/apps/SigC641x_C667x/
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
	echo "Signalogic Install Path: $unInstallPath"
	rm -rf $unInstallPath/Signalogic*
	rm -rf /etc/signalogic
	rmmod sig_mc_hw
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
	unset SIGNALOGIC_INSTALL_PATH
	sed -i '/SIGNALOGIC_INSTALL_PATH*/d' /etc/environment
	echo "Uninstall complete..."
	
}

installCheck () {

line='................................................................'

if [ !$installPath ]; then
	installPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
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
if [ -f $installPath/Signalogic/DirectCore/apps/SigC641x_C667x/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="boardTest"
if [ -f $installPath/Signalogic/DirectCore/apps/SigC641x_C667x/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="iaTest"
if [ -f $installPath/Signalogic/DirectCore/apps/SigC641x_C667x/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="streamTest"
if [ -f $installPath/Signalogic/DirectCore/apps/SigC641x_C667x/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="fftTest"
if [ -f $installPath/Signalogic/DirectCore/apps/SigC641x_C667x/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="videoTest"
if [ -f $installPath/Signalogic/DirectCore/apps/SigC641x_C667x/$d/$d ]; then
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
PS3="Please select target for Signalogic SW install [1-2]: "
select target in "Host" "VM" 
do
	case $target in
		"Host") break;;
		"VM") break;;
	esac
done
echo "*****************************************************"
echo
PS3="Please select install operation to perform [1-6]: "
select opt in "Signalogic SW installation" "Uninstall Signalogic SW" "Signalogic Install Check" "exit"
do
    case $opt in
		"Signalogic SW installation") packageSetup; dependencyCheck; swInstall; break;;
      "Uninstall Signalogic SW") unInstall; break;;
		"Signalogic Install Check") installCheck; break;;
      "exit") echo "Exiting..."; break;;
      *) echo invalid option;;
    esac
done