#!/bin/bash
#================================================================================================
# Copyright (C) Signalogic Inc 2017-2020
# Script for installing/uinstalling Signalogic SW (includes demo + SRF, license versions)
# Rev 2.1

	# Requirements
		# Internet connection
		# Install unrar package
			# For Ubuntu, use command:
				# apt-get install unrar
			# For RHEL/CentOS,
				# yum -y install epel-release && rpm -Uvh http://li.nux.ro/download/nux/dextop/el7/x86_64/nux-dextop-release-0-5.el7.nux.noarch.rpm
				# yum install unrar

# Revision History
#  Created Feb 2016 HP
#  Modified Feb 2017 AM
#  Modified Aug 2018 JHB
#  Modified Sep 2019 JHB, fix issues in install menu options
#  Modified Sep 2020 JHB, correct various problems with unrar
#                         -if unrar was not installed the script was exiting both itself and the terminal window also (because script must be run as sourced)
#                         -add unrarCheck() function, which prompts to install unrar if needed
#                         -add depInstall_wo_dpkg() function, which does OS-dependent install based on line_pkg var
#                         -add method to install unrar for Ubuntu 17.04 and earlier, where unrar was in a weird repository due to licensing restrictions
#  Modified Sep 2020 JHB, fix problems with re-install (i.e. installing over existing files), including unrar command line, symlinks
#  Modified Sep 2020 JHB, other minor fixes, such as removing "cd -" commands after non DirectCore/lib installs (which are in a loop). Test in Docker containers, including Ubuntu 12.04, 18.04, and 20.04
#  Modified Oct 2020 JHB, assume symlink exists in rpm dependency checks
#================================================================================================

depInstall_wo_dpkg() {

	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
		yum localinstall $line_pkg
	elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
		apt-get install $line_pkg
	fi
}

unrarCheck() {

	unrarInstalled=`type -p unrar`
	if [ ! $unrarInstalled ]; then

		while true; do
			read -p "Unrar not installed, ok to install now ?" yn
			case $yn in
#				[Yy]* ) apt-get install unrar; return 1; break;;
				[Yy]* ) line_pkg="unrar"
                    depInstall_wo_dpkg;
                    if [[ $? > 0 ]]
                    then
                       echo "Attempting to install older version of unrar ..."  # old version of unrar was called "unrar-nonfree" due to licensing restrictions, Linux guys hate that enough they stuck it in the Necromonger underverse (well, close)
                       sed -i "/^# deb .* multiverse$/ s/^# //" /etc/apt/sources.list; apt-get update
                       depInstall_wo_dpkg;
                       if [[ $? = 0 ]]
                       then
                          return 1
                       fi;
                    else
                       return 1
                    fi
                    break;;
				[Nn]* ) return 0;;
				* ) echo "Please enter y or n";;
			esac
		done
	else
		return 1
   fi
}

packageSetup() { # prompt for Signalogic installation path, extarct package, set envionment var

	echo "Enter path for SigSRF software and dependency package installation:"
	read installPath
	if [ $installPath ]; then
		echo "Install path: $installPath"
	else
		installPath="/usr/local"
		echo "Default Install path (/) is chosen"
	fi

# add -o+ to force overwrite if files already exist, JHB Sep2020

	unrar x -o+ Signalogic_sw_*.rar $installPath/
}

depInstall () {

	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
		yum localinstall $line
	elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
		dpkg -i $line
		if [ $? -gt 0 ]; then
			apt-get -f --force-yes --yes install  # package name not needed if run immediately after dpkg, JHB Sep2020
		fi
	fi
}

dependencyCheck() { # check for generic non-Signalogic SW packages and prompt for installation if not installed
	
	DOTs='................................................................'
	
	if [ "$opt" = "Install SigSRF software" ]; then
		dependencyInstall="Dependency Check + Install"
	elif [ "$opt" = "Dependency Check" ]; then
		installPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
		dependencyInstall="Dependency Check"
		
		if [ ! $installPath ]; then
			echo 
			echo "SigSRF software install path could not be found"
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
		fi

		cd $installPath/Signalogic/installation_rpms/RHEL  # assume symlink exists, JHB Oct2020
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
		cd $installPath/Signalogic/installation_rpms/Ubuntu  # assume symlink exists, JHB Oct2020
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
			# Dependencies gcc and g++ will be installed as gcc-4.8 and g++-4.8 so it is necessary to create a symmlink (gcc and g++) otherwise SW installation might fail
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

createSymLinks() { # create symlinks

	echo

	kernel_version=`uname -r`
	echo $kernel_version
	echo "Creating symlinks..."
	
	if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
		if [ ! -L /usr/src/linux ]; then
			ln -s /usr/src/kernels/$kernel_version /usr/src/linux
		fi 
	elif [ "$OS" = "Ubuntu" ]; then
		if [ ! -L /usr/src/linux ]; then
			ln -s /usr/src/linux-headers-$kernel_version /usr/src/linux
		fi
	fi

# if unrar fails for any reason, we need to avoid attempting to symlink to "Signalogic_2*", otherwise we'll end up with a symlink with a literal "*", JHB Oct2020

	if [ ! -d $installPath/Signalogic_2* ]; then
		return 0;
	fi

# Create symlinks. Assume _2xxx in the name, otherwise ln command might try to symlink the .rar file :-(

   if [ ! -L $installPath/Signalogic ]; then
	   ln -s $installPath/Signalogic_2* $installPath/Signalogic
   fi

   if [ ! -L $installPath/Signalogic/apps ]; then
	   ln -s $installPath/Signalogic_2*/DirectCore/apps/SigC641x_C667x $installPath/Signalogic/apps
   fi

	if [ ! -L $installPath/Signalogic/DirectCore/apps/coCPU ]; then
	   ln -s $installPath/Signalogic_2*/DirectCore/apps/SigC641x_C667x $installPath/Signalogic/DirectCore/apps/coCPU 
   fi
}

swInstall() {				# It will install Signalogic SW on specified path

	# Set up environment var permanently
	export SIGNALOGIC_INSTALL_PATH=$installPath 
	echo "SIGNALOGIC_INSTALL_PATH=$installPath" >> /etc/environment
	
	echo
	echo "SigSRF software Installation will be performed..."

	mv $installPath/Signalogic/etc/signalogic /etc
	rm -rf $installPath/Signalogic/etc

	kernel_version=`uname -r`
	echo	

   if [ "$installOptions" = "coCPU" ]; then

      echo "loading driver"
      if [ "$target" = "Host" ]; then
         #cd $installPath/Signalogic/DirectCore/hw_utils; make clean; make
         cd $installPath/Signalogic/DirectCore/hw_utils; make
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
  		      # make load
		   fi
      elif [ "$target" = "VM" ]; then
         cd $installPath/Signalogic/DirectCore/virt_driver; make load
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
         cp -p $installPath/Signalogic/DirectCore/driver/sig_mc_hw.modules /etc/sysconfig/modules/
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
   fi

	echo
	echo "Installing SigSRF pktlib, voplib, streamlib, diaglib, and other shared libraries..."
	cd $installPath/Signalogic/DirectCore/lib/
	for d in *; do
		cd $d; cp -p lib* /usr/lib; ldconfig; cd -
	done

	echo
	echo "Installing SigSRF codec shared libraries..."
	cd $installPath/Signalogic/SIG_LIBS/Voice/AMR/lib 2>/dev/null
	cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/AMR-WB/lib 2>/dev/null
	cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/AMR-WB+/lib 2>/dev/null
	cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/EVS_floating-point/lib 2>/dev/null
	cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/G726/lib 2>/dev/null
	cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/G729AB/lib 2>/dev/null
	cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/MELPe_floating-point/lib 2>/dev/null
	cp -p lib* /usr/lib 2>/dev/null;
	ldconfig;

	echo
	echo "Building SigSRF applications..."

   if [ "$installOptions" = "coCPU" ]; then

		cd $installPath/Signalogic/apps/boardTest
		make clean; make all;

		cd $installPath/Signalogic/apps/streamTest
		make clean; make all;

		cd $installPath/Signalogic/apps/iaTest
		make clean; make all;
	fi

	cd $installPath/Signalogic/apps/mediaTest
	make clean; make all;

	cd $installPath/Signalogic/apps/mediaTest/mediaMin
	make clean; make all;

	cd $wdPath
}

unInstall() {			# uninstall Signalogic SW completely

	OS=$(cat /etc/os-release | grep -w NAME=* | sed -n -e '/NAME/ s/.*\= *//p' | sed 's/"//g')
	echo
	echo "Signalogic SW uninstallation will be performed..."
	echo
	unInstallPath=$SIGNALOGIC_INSTALL_PATH
	if [ ! $unInstallPath ]; then
		unInstallPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
		if [ ! $unInstallPath ]; then
			echo 
			echo "Signalogic install path could not be found"
			echo "Exiting..."
			echo
			exit
		fi
	fi
	
	echo "Signalogic Install Path: $unInstallPath"
	rm -rf $unInstallPath/Signalogic/*  # remove all installed folders
	rm -rf $unInstallPath/Signalogic*  # remove symlink, rar files
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

# remove libs from /user/lib

#	rm -rf /usr/lib/libcallmgr.a
#	rm -rf /usr/lib/libenmgr*
	rm -rf /usr/lib/libcimlib*
	rm -rf /usr/lib/libhwmgr*
	rm -rf /usr/lib/libfilelib*
	rm -rf /usr/lib/libhwlib*
	rm -rf /usr/lib/libpktib*
	rm -rf /usr/lib/libstreamlib*
	rm -rf /usr/lib/libvoplib*
	rm -rf /usr/lib/libalglib*
	rm -rf /usr/lib/libinferlib*
	rm -rf /usr/lib/libaviolib*
	rm -rf /usr/lib/libdiaglib*
	rm -rf /usr/lib/libstublib*
	rm -rf /usr/lib/libtdmlib*.a
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
			echo "Signalogic install path could not be found"
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

#d="libenmgr"
#if [ -f /usr/lib/libenmgr.a ]; then
#	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
#else
#	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
#fi

#d="libcallmgr"
#if [ -f /usr/lib/libcallmgr.a ]; then
#	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
#else
#	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
#fi

#d="libtdm"
#if [ -f /usr/lib/libtdmlib.a ]; then
#	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
#else
#	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
#fi

# Signalogic App Executables Check #

echo | tee -a $diagReportFile
echo "Signalogic App Executables Check:" | tee -a $diagReportFile
d="memTest"
if [ -f $installPath/Signalogic/apps/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="boardTest"
if [ -f $installPath/Signalogic/apps/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="iaTest"
if [ -f $installPath/Signalogic/apps/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="streamTest"
if [ -f $installPath/Signalogic/apps/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="fftTest"
if [ -f $installPath/Signalogic/apps/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="mediaTest"
if [ -f $installPath/Signalogic/apps/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

d="mediaMin"
if [ -f $installPath/Signalogic/apps/$d/$d ]; then
	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
else
	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
fi

#d="videoTest" -- no longer used
#if [ -f $installPath/Signalogic/apps/$d/$d ]; then
#	printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
#else
#	printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
#fi

# Checks leftover hwlib files in /dev/shm

echo | tee -a $diagReportFile
echo "Leftover hwlib files check:" | tee -a $diagReportFile
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

PS3="Please select install operation to perform [1-5]: "
select opt in "Install SigSRF Software" "Install SigSRF Software with coCPU Option" "Uninstall SigSRF Software" "Check / Verify SigSRF Software Install" "Exit"
do
   case $opt in
		"Install SigSRF Software") if ! unrarCheck; then
			packageSetup; createSymLinks; dependencyCheck; swInstall;
		fi
		break;;
		"Install SigSRF Software with coCPU Option") if ! unrarCheck; then
			packageSetup; createSymLinks; dependencyCheck; installOptions = "coCPU"; swInstall;
		fi
		break;;
		"Uninstall SigSRF Software") unInstall; break;;
		"Check / Verify SigSRF Software Install") installCheck; break;;
		"Exit") echo "Exiting..."; break;;
      *) echo invalid option $opt;;
   esac
done