#!/bin/bash
#================================================================================================
# Bash script to install/uninstall SigSRF SDK
# Copyright (C) Signalogic Inc 2017-2022
# Rev 1.7.1

# Requirements
   # Internet connection
   # Install unrar package -- handled automatically in unrarCheck() below, but if that fails ...
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
#  Modified Jan 2021 JHB, fix minor issues in installCheckVerify(), add SIGNALOGIC_INSTALL_OPTIONS in /etc/environment, add preliminary check for valid .rar file
#  Modified Jan 2021 JHB, unrar only most recent .rar file in case user has downloaded before, look for .rar that matches installed distro
#  Modified Feb 2021 JHB, add ASR version to install options, add swInstallSetup(), fix problems in dependencyCheck(), add install path confirmation
#  Modified Jan 2022 JHB, mods for CentOS 8, remove reference to specific gcc version
#  Modified Jan 2022 JHB, assume distro other than Ubunto or CentOS / RHEL as possible Ubuntu/Debian, let user know this is happening
#================================================================================================

depInstall_wo_dpkg() {

	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
		yum localinstall $line_pkg
#	elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
   else  # else includes Ubuntu, Debian, VM target, or anything else
		apt-get install $line_pkg
	fi
}

unrarCheck() {

	unrarInstalled=`type -p unrar`  # see if unrar is recognized on cmd line
	if [ ! $unrarInstalled ]; then  # if not then need to install

		while true; do
			read -p "Unrar not installed, ok to install now ?" yn
			case $yn in
				[Yy]* ) line_pkg="unrar"
						depInstall_wo_dpkg;
						if [[ $? > 0 ]]; then
                     if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
                        echo "Attempting to install rarlab unrar ..."
                        wget https://www.rarlab.com/rar/rarlinux-x64-6.0.2.tar.gz
                        tar -zxvf rarlinux-x64-6.0.2.tar.gz
                        mv rar/rar rar/unrar /usr/local/bin/
#                    elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
                     else  # else includes Ubuntu, Debian, VM target, or anything else
                        echo "Attempting to install older version of unrar ..."  # old version of unrar was called "unrar-nonfree" due to licensing restrictions, Linux guys hate that enough they stuck it in the Necromonger underverse (well, close)
                        sed -i "/^# deb .* multiverse$/ s/^# //" /etc/apt/sources.list; apt-get update
                        depInstall_wo_dpkg;
                     fi

							if [[ $? = 0 ]]; then
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

packageSetup() { # check for .rar file and if found, prompt for Signalogic installation path, extract files

   # match SigSRF .rar files by ASR version and distro type, ignore the following:  JHB Jan2021
   #  -SDK vs license
   #  -host vs target
   #  -distro version and date

	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
		if [ "$installOptions" = "ASR" ]; then  # demo includes automatic speech recognition
			rarFile="Signalogic_sw_host_SigSRF_*_ASR_*CentOS*.rar"
		else
			rarFile="Signalogic_sw_host_SigSRF_*CentOS*.rar"
		fi
	else  # add other distro types later.  Currently Debian defaults to Ubuntu .rar, JHB Jan2021
		if [ "$installOptions" = "ASR" ]; then
			rarFile="Signalogic_sw_host_SigSRF_*_ASR_*Ubuntu*.rar"
		else
			rarFile="Signalogic_sw_host_SigSRF_*Ubuntu*.rar"
		fi
	fi

   # unrar only most recent .rar file found (also this avoids unraring more than one file, due to wildcard), JHB Jan2021
   # notes - Github doesn't support last-modified headers (has been that way for years), so wget and curl are unable to preserve the file date. But we still search for most recent .rar as a best practice

	rarFileNewest=""
	for iFileName in `ls -tr $rarFile`; do
		rarFileNewest=$iFileName;
	done;

	if [ "$rarFileNewest" = "" ]; then  # add check for no .rar files found, JHB Jan2021
		echo "Install package rar file not found"
		return 0
	fi

	while true; do

		echo "Enter path for SigSRF software and dependency package installation:"
		read installPath

		if [ ! $installPath ]; then
			installPath="/usr/local"  # default if nothing entered
		fi

		read -p "Please confirm install path $installPath  [Y] or [N] " Confirm

		case $Confirm in
			[Yy]* ) break;;
		esac
	done

	unrar x -o+ $rarFileNewest $installPath/

	return 1
}

depInstall() {

	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
		yum localinstall $line
#	elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
   else  # else includes Ubuntu, Debian, VM target, or anything else
		dpkg -i $line
		if [ $? -gt 0 ]; then
			apt-get -f --force-yes --yes install  # package name not needed if run immediately after dpkg, JHB Sep2020
		fi
	fi
}

swInstallSetup() {  # basic setup needed by both dependencyCheck() and swInstall()

   # Set up environment vars, save install path and install options in env vars

	export SIGNALOGIC_INSTALL_PATH=$installPath
	sed -i '/SIGNALOGIC_INSTALL_PATH*/d' /etc/environment  # first remove any install paths already there, JHB Jan2021
	echo "SIGNALOGIC_INSTALL_PATH=$installPath" >> /etc/environment
	export SIGNALOGIC_INSTALL_OPTIONS=$installOptions
	sed -i '/SIGNALOGIC_INSTALL_OPTIONS*/d' /etc/environment  # first remove any install options already there, JHB Jan2021
	echo "SIGNALOGIC_INSTALL_OPTIONS=$installOptions" >> /etc/environment
	
	echo
	echo "SigSRF software Installation will be performed..."
	mv -f $installPath/Signalogic_*/etc/signalogic /etc
	rm -rf $installPath/Signalogic*/etc
	echo
	kernel_version=`uname -r`
	echo $kernel_version
	echo
	echo "Creating symlinks..."
	
	if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
		if [ ! -L /usr/src/linux ]; then
			ln -s /usr/src/kernels/$kernel_version /usr/src/linux
		fi 
#	elif [ "$OS" = "Ubuntu" ]; then
   else  # else includes Ubuntu, Debian, VM target, or anything else
		if [ ! -L /usr/src/linux ]; then
			ln -s /usr/src/linux-headers-$kernel_version /usr/src/linux
		fi
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

dependencyCheck() {  # Check for generic sw packages and prompt for installation if not installed

	echo
	echo "Dependency check..."

	DOTs='................................................................'
	
	if [[ "$opt" == "Install"* ]]; then  # to handle ASR and coCPU options, use wildcard to find first part of string (note -- didn't get white spaces working yet), JHB Feb2021

		dependencyInstall="Dependency Check + Install"

	elif [ "$opt" = "Dependency Check" ]; then  # currently this is a deprecated install option

		installPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
		dependencyInstall="Dependency Check"
      gcc_package=""

		if [ ! $installPath ]; then
			echo 
			echo "SigSRF software install path not found"
			echo
			return 0
		fi
	fi
	
	if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
	{
		if [ "$dependencyInstall" = "Dependency Check + Install" ]; then

			gcc_package=$(rpm -qa gcc-c++)  # generic g++ check, should come back with version installed

			if [ ! $gcc_package ]; then
				echo -e "gcc compiler and toolchain is needed\n"
				yum install gcc-c++
            gcc_package=$(rpm -qa gcc-c++)  # recheck
			fi

#			lsbReleaseInstalled=`type -p lsb_release`
#			if [ ! $lsbReleaseInstalled ]; then
#		  		echo "lsb_release package is needed"
#				yum install redhat-lsb-core
#			fi
      fi

		cd $installPath/Signalogic/installation_rpms/RHEL
		filename="rhelDependency.txt"
   }
#	elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
   else  # else includes Ubuntu, Debian, VM target, or anything else
	{

      if [ "$OS" != "Ubuntu" ]; then
         echo
         echo "Not CentOS or Ubunto distro $(OS) ... attempting to install assuming Ubuntu / Debian derivative"
      fi

		if [ "$dependencyInstall" = "Dependency Check + Install" ]; then

#			gcc_package=$(dpkg -s g++-4.8 2>/dev/null | grep Status | awk ' {print $4} ')
#			if [ ! $gcc_package ]; then
#           gcc_package=$(dpkg -s g++ 2>/dev/null | grep Status | awk ' {print $4} ')  # generic g++ check, should come back with "installed"
#        fi

			gcc_package=$(dpkg -s g++ 2>/dev/null | grep Status | awk ' {print $4} ')  # generic g++ check, should come back with "installed"

			if [ ! $gcc_package ]; then
				echo -e "gcc compiler and toolchain is needed\n"
#				apt-get -y --purge remove gcc g++ gcc-4.8 g++-4.8
#				unlink /usr/bin/gcc
#				unlink /usr/bin/g++
				apt install build-essential

            gcc_package=$(dpkg -s g++ 2>/dev/null | grep Status | awk ' {print $4} ')  # recheck
			fi

			lsbReleaseInstalled=`type -p lsb_release`
			if [ ! $lsbReleaseInstalled ]; then
		  		echo "lsb_release package is needed"
				apt-get install lsb-release
			fi
		fi

		cd $installPath/Signalogic/installation_rpms/Ubuntu
		filename="UbuntuDependency.txt"
   }
   fi
 
   while read -r -u 3 line
	do

		d=$(sed 's/_.*//g' <<< $line)
		if [[ "$d" == "make" ]]; then
			e=$d
		else
         if [[ "$d" == *"-devel-"* ]]; then
			   e=$(sed 's/-$//2' <<< $line)  # search for second "-" to set e with generic developer package name
         else
            e=$(sed 's/-.*//g' <<< $line)  # search for first "-" to set e with generic package name
         fi
		fi

		package=$(dpkg -s $e 2>/dev/null | grep Status | awk ' {print $4} ')

		if [[ ( "$e" == "libncurses"* || "$e" == "ncurses"* || "$e" == "libncurses-devel"* || "$e" == "ncurses-devel"* ) && "$installOptions" != "coCPU" ]]; then  # libncurses only referenced in memTest Makefile
			package="not needed"
		fi

		if [[ ( "$e" == "libexplain"* || "$e" == "libexplain-devel"* ) && "$installOptions" != "coCPU" ]]; then  # libexplain only referenced in streamTest Makefile
			package="not needed"
		fi

		if [[ "$e" == "gcc"* && "$gcc_package" != "" ]]; then  # gcc of some version already installed. Since we retro-test back to 4.6 (circa 2011), we don't worry about minimum version
			package="already installed"
		fi

		if [ "$package" == "" ]; then
			if [ "$dependencyInstall" = "Dependency Check + Install" ]; then
				if [ ! $totalInstall ]; then
					read -p "Do you wish to install $e package? Please enter [Y]es, [N]o, [A]ll: " Dn
					if [[ ($Dn = "a") || ($Dn = "A") ]]; then
						totalInstall=1
					fi
				fi
				case $Dn in
					[YyAa]* ) depInstall ; ;;  # depInstall uses "line" var
					[Nn]* ) ;;
					* ) echo "Please retry with just y, n, or a";;
				esac
			elif [ "$dependencyInstall" = "Dependency Check" ]; then
				printf "%s %s[ NOT INSTALLED ]\n" $e "${DOTs:${#e}}"
			fi
		elif [ "$package" = "not needed" ]; then
			printf "%s %s[ NOT NEEDED ]\n" $e "${DOTs:${#e}}"
		else
			printf "%s %s[ ALREADY INSTALLED ]\n" $e "${DOTs:${#e}}"
   	fi
	done 3< "$filename"

#	if [ "$dependencyInstall" = "Dependency Check + Install" ]; then
#		# Dependencies gcc and g++ will be installed as gcc-4.8 and g++-4.8 so it is necessary to create a symmlink (gcc and g++) otherwise SW installation might fail
#		if [ ! -L  /usr/bin/gcc ]; then
#	  	  	ln -s /usr/bin/gcc-4.8 /usr/bin/gcc
#		fi
#		if [ ! -L  /usr/bin/g++ ]; then
#			ln -s /usr/bin/g++-4.8 /usr/bin/g++
#		fi
#	fi
}

swInstall() {  # install Signalogic SW on specified path

# note -- assumes dependencyCheck() and swInstallSetup() have run

   if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
      cp_prefix="/bin/"
#   elif [ "$OS" = "Ubuntu" ]; then
   else  # else includes Ubuntu, Debian, VM target, or anything else
      cp_prefix=""
   fi

	if [ "$installOptions" = "coCPU" ]; then

		echo
		echo "Loading coCPU driver ..."
		echo

		if [ "$target" = "Host" ]; then

         if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
            distribution=$(cat /etc/centos-release)
         #elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
         else  # else includes Ubuntu, Debian, VM target, or anything else
            distribution=$(lsb_release -d)
         fi

			cd $installPath/Signalogic/DirectCore/hw_utils; make
			cd ../driver; 
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

			make load;  # load driver -- note if already loaded then an error message is shown, but causes no problems, JHB Jan2021
			echo

			if lsmod | grep sig_mc_hw &> /dev/null ; then
				echo "coCPU driver is loaded"
				echo
			fi
		elif [ "$target" = "VM" ]; then
			cd $installPath/Signalogic/DirectCore/virt_driver;
			make load;
			echo
		fi

		echo "Setting up autoload of coCPU driver on boot"

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
#		elif [ "$OS" = "Ubuntu" ]; then
      else  # else includes Ubuntu, Debian, VM target, or anything else
			echo "sig_mc_hw" >> /etc/modules
			sed -i '/exit*/d' /etc/rc.local
			echo "chmod 666 /dev/sig_mc_hw" >> /etc/rc.local
			echo "exit 0" >> /etc/rc.local
			chmod 755 /etc/rc.local
		fi
	fi

	echo
	echo "Installing SigSRF libs for packet handling, stream group processing, inference, diagnostic, etc..."
	cd $installPath/Signalogic/DirectCore/lib/
	for d in *; do
		cd $d; "$cp_prefix"cp -p lib* /usr/lib; ldconfig; cd ~-; cd -; cd ~-  # go back with no output, then go to subfolder again to show it onscreen, then go back and continue
	done

	echo
	echo "Installing SigSRF codec libs..."
	cd $installPath/Signalogic/SIG_LIBS/Voice/AMR/lib 2>/dev/null
	"$cp_prefix"cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/AMR-WB/lib 2>/dev/null
	"$cp_prefix"cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/AMR-WB+/lib 2>/dev/null
	"$cp_prefix"cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/EVS_floating-point/lib 2>/dev/null
	"$cp_prefix"cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/G726/lib 2>/dev/null
	"$cp_prefix"cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/G729AB/lib 2>/dev/null
	"$cp_prefix"cp -p lib* /usr/lib 2>/dev/null;
	cd $installPath/Signalogic/SIG_LIBS/Voice/MELPe_floating-point/lib 2>/dev/null
	"$cp_prefix"cp -p lib* /usr/lib 2>/dev/null;
	ldconfig;

	echo
	echo "Building SigSRF applications..."

	if [ "$installOptions" = "coCPU" ]; then

		cd $installPath/Signalogic/apps/memTest
		make clean; make all;

		cd $installPath/Signalogic/apps/boardTest
		make clean; make all;

		cd $installPath/Signalogic/apps/streamTest
		make clean; make all;
	fi

	cd $installPath/Signalogic/apps/iaTest
	make clean; make all;

	cd $installPath/Signalogic/apps/mediaTest
	make clean; make all;

	cd $installPath/Signalogic/apps/mediaTest/mediaMin
	make clean; make all;

	cd $wdPath
}

unInstall() { # uninstall Signalogic SW completely

	OS=$(cat /etc/os-release | grep -w NAME=* | sed -n -e '/NAME/ s/.*\= *//p' | sed 's/"//g')
	echo
	echo "Signalogic SW uninstallation will be performed..."
	echo
	unInstallPath=$SIGNALOGIC_INSTALL_PATH
	if [ ! $unInstallPath ]; then
		unInstallPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
		if [ ! $unInstallPath ]; then
			echo 
			echo "Signalogic install path not found"
			echo
			return 0
		fi
	fi

	unInstallOptions=$SIGNALOGIC_INSTALL_OPTIONS
	if [ ! $unInstallOptions ]; then
		unInstallOptions=$(grep -w "SIGNALOGIC_INSTALL_OPTIONS=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_OPTIONS/ s/.*\= *//p')
	fi
	
	echo "Signalogic Install Path: $unInstallPath"
	rm -rf $unInstallPath/Signalogic*
	rm -rf /etc/signalogic

	if [ "$uninstallOptions" = "coCPU" ]; then

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
#		elif [ "$OS" = "Ubuntu" ]; then
      else  # else includes Ubuntu, Debian, VM target, or anything else
			if [ $target = "Host" ]; then
				rm -rf /lib/modules/$kernel_version/sig_mc_hw.ko
			elif [ $target = "VM" ]; then
				rm -rf /lib/modules/$kernel_version/virtio-sig.ko
			fi
		fi
	
		if [ "$OS" = "CentOS Linux" -o "$OS" = "Red Hat Enterprise Linux Server" ]; then
			sed -i '/chmod 666 \/dev\/sig_mc_hw/d' /etc/rc.d/rc.local 
#		elif [ "$OS" = "Ubuntu" ]; then
      else  # else includes Ubuntu, Debian, VM target, or anything else
			sed -i '/chmod 666 \/dev\/sig_mc_hw/d' /etc/rc.local
		fi
	fi

	rm -rf /usr/lib/libcimlib*
	rm -rf /usr/lib/libhwmgr*
	rm -rf /usr/lib/libfilelib*
	rm -rf /usr/lib/libhwlib*
	rm -rf /usr/lib/libpktib*
	rm -rf /usr/lib/libvoplib*
	rm -rf /usr/lib/libalglib*
	rm -rf /usr/lib/libinferlib*
	rm -rf /usr/lib/libaviolib*
	rm -rf /usr/lib/libdiaglib*
	rm -rf /usr/lib/libstublib*
	rm -rf /usr/lib/libtdmlib*

	unset SIGNALOGIC_INSTALL_PATH
	sed -i '/SIGNALOGIC_INSTALL_PATH*/d' /etc/environment
	unset SIGNALOGIC_INSTALL_OPTIONS
	sed -i '/SIGNALOGIC_INSTALL_OPTIONS*/d' /etc/environment

	echo "Uninstall complete..."
}

diagLibPrint() {

	if [ -f /usr/lib/"$libfile" ]; then
		printf "%s %s[ OK ]\n" $libname "${line:${#libname}}" | tee -a $diagReportFile
	else
		printf "%s %s[ X ]\n" $libname "${line:${#libname}}" | tee -a $diagReportFile
	fi
}

diagAppPrint() {

	if [ -f $installPath/Signalogic/apps/"$appfile"/"$appfile" ]; then
		printf "%s %s[ OK ]\n" $appname "${line:${#appname}}" | tee -a $diagReportFile
	else
		printf "%s %s[ X ]\n" $appname "${line:${#appname}}" | tee -a $diagReportFile
	fi
}

installCheckVerify() {

	line='................................................................'

	installPath=$SIGNALOGIC_INSTALL_PATH
	if [ ! $installPath ]; then
		installPath=$(grep -w "SIGNALOGIC_INSTALL_PATH=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_PATH/ s/.*\= *//p')
		if [ ! $installPath ]; then
			echo 
			echo "Signalogic install path could not be found"
			echo
			return 0
		fi
	fi

	installOptions=$SIGNALOGIC_INSTALL_OPTIONS
	if [ ! $installOptions ]; then
		installOptions=$(grep -w "SIGNALOGIC_INSTALL_OPTIONS=*" /etc/environment | sed -n -e '/SIGNALOGIC_INSTALL_OPTIONS/ s/.*\= *//p')
	fi

	current_time=$(date +"%m.%d.%Y-%H:%M:%S")
	diagReportFile=DirectCore_diagnostic_report_$current_time.txt
	touch $diagReportFile

	# Path check

	echo
	echo "Distro Info" | tee -a $diagReportFile
   if [ "$OS" = "Red Hat Enterprise Linux Server" -o "$OS" = "CentOS Linux" ]; then
      cat /etc/centos-release | tee -a $diagReportFile
#  elif [ "$target" = "VM" -o "$OS" = "Ubuntu" ]; then
   else   # else includes Ubuntu, Debian, VM target, or anything else
      lsb_release -a | tee -a $diagReportFile
   fi

	echo | tee -a $diagReportFile
	echo "SigSRF Install Path and Options Check" | tee -a $diagReportFile
	echo "Install path: $installPath" | tee -a $diagReportFile
	echo "Install options: $installOptions" | tee -a $diagReportFile

	if [ "$installOptions" = "coCPU" ]; then

		# Driver check

		echo "SigSRF coCPU Driver Check" | tee -a $diagReportFile

		libfile="sig_mc_hw"; libname="sig_mc_hw";
		if [ -c /dev/$libfile ]; then
			printf "%s %s[ OK ]\n" $libname "${line:${#libname}}" | tee -a $diagReportFile
		else
			printf "%s %s[ X ]\n" $libname "${line:${#libname}}" | tee -a $diagReportFile
		fi
 	fi

	# Symlinks check

	echo | tee -a $diagReportFile
	echo "SigSRF Symlinks Check" | tee -a $diagReportFile

	d="Signalogic Symlink"
	if [ -L $installPath/Signalogic ]; then
		printf "%s %s[ OK ]\n" "$d" "${line:${#d}}" | tee -a $diagReportFile
	else
		printf "%s %s[ X ]\n" "$d" "${line:${#d}}" | tee -a $diagReportFile
	fi

	d="Apps Symlink"
	if [ -L $installPath/Signalogic/apps ]; then
		printf "%s %s[ OK ]\n" "$d" "${line:${#d}}" | tee -a $diagReportFile
	else
		printf "%s %s[ X ]\n" "$d" "${line:${#d}}" | tee -a $diagReportFile
	fi

	d="Linux Symlink"
	if [ -L /usr/src/linux ]; then
		printf "%s %s[ OK ]\n" "$d" "${line:${#d}}" | tee -a $diagReportFile
	else
		printf "%s %s[ X ]\n" "$d" "${line:${#d}}" | tee -a $diagReportFile
	fi

	# Libs check

	echo | tee -a $diagReportFile
	echo "SigSRF Libs Check" | tee -a $diagReportFile

	libfile="libhwlib.so"; libname="hwlib"; diagLibPrint;
	libfile="libpktlib.so"; libname="pktlib"; diagLibPrint;
	libfile="libvoplib.so"; libname="voplib"; diagLibPrint;
	libfile="libstreamlib.so"; libname="streamlib";	diagLibPrint;
	libfile="libdiaglib.so"; libname="diaglib"; diagLibPrint;
	libfile="libhwmgr.a"; libname="hwmgr"; diagLibPrint;
	libfile="libfilelib.a"; libname="filelib"; diagLibPrint;
	libfile="libcimlib.a"; libname="cimlib"; diagLibPrint;

	# Apps check

	echo | tee -a $diagReportFile
	echo "SigSRF Apps Check" | tee -a $diagReportFile

	if [ "$installOptions" = "coCPU" ]; then

		appfile="memTest"; appname="memTest"; diagAppPrint;
		appfile="boardTest"; appname="boardTest"; diagAppPrint;
		appfile="fftTest"; appname="fftTest"; diagAppPrint;
		appfile="streamTest"; appname="streamTest"; diagAppPrint;
	fi

	appfile="iaTest"; appname="iaTest"; diagAppPrint;
	appfile="mediaTest"; appname="mediaTest"; diagAppPrint;

	appfile="mediaMin"; appname="mediaMin";
	if [ -f $installPath/Signalogic/apps/mediaTest/$appfile/$appfile ]; then
		printf "%s %s[ OK ]\n" $appname "${line:${#appname}}" | tee -a $diagReportFile
	else
		printf "%s %s[ X ]\n" $appname "${line:${#appname}}" | tee -a $diagReportFile
	fi

	# Leftover /dev/shm hwlib files check

	echo | tee -a $diagReportFile
	echo "SigSRF Leftover hwlib Files Check" | tee -a $diagReportFile
	d="hwlib_mutex"

	if [ -f /dev/shm/$d ]; then
		printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile  # change polarity -- no leftover files is Ok, leftover is not, JHB Jan2021
	else
		printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
	fi

	d="hwlib_info"
	if [ -f /dev/shm/$d ]; then
		printf "%s %s[ X ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
	else
		printf "%s %s[ OK ]\n" $d "${line:${#d}}" | tee -a $diagReportFile
	fi
}

# *********** script entry point ************

wdPath=$PWD
OS=$(cat /etc/os-release | grep -w NAME=* | sed -n -e '/NAME/ s/.*\= *//p' | sed 's/"//g')  # OS var is used throughout script
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

COLUMNS=1  # force single column menu, JHB Jan2021
PS3="Please select install operation to perform [1-6]: "
select opt in "Install SigSRF Software" "Install SigSRF Software with ASR Option" "Install SigSRF Software with coCPU Option" "Uninstall SigSRF Software" "Check / Verify SigSRF Software Install" "Exit"
do
	case $opt in
		"Install SigSRF Software") if ! unrarCheck; then
			if ! packageSetup; then
				swInstallSetup; dependencyCheck; swInstall;
			fi
		fi
		break;;
		"Install SigSRF Software with ASR Option") if ! unrarCheck; then
			if ! packageSetup; then
				swInstallSetup; dependencyCheck; installOptions="ASR"; swInstall;
			fi
		fi
		break;;
		"Install SigSRF Software with coCPU Option") if ! unrarCheck; then
			if ! packageSetup; then
				swInstallSetup; dependencyCheck; installOptions="coCPU"; swInstall;
			fi
		fi
		break;;
		"Uninstall SigSRF Software") unInstall; break;;
		"Check / Verify SigSRF Software Install") installCheckVerify; break;;
		"Exit") echo "Exiting..."; break;;
		*) echo invalid option $opt;;
	esac
done