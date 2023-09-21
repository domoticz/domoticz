#!/usr/bin/env bash
# Domoticz: Open Source Home Automation System
# (c) 2012, 2016 by GizMoCuz
# Big thanks to Jacob Salmela! (This is based on the excelent Pi-Hole install script ;)
# http://www.domoticz.com
# Installs Domoticz
#
# Domoticz is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.

# Donations are welcome via the website or application
#
# Install with this command (from your Pi):
#
# curl -L install.domoticz.com | bash

set -e
######## VARIABLES #########
setupVars=/etc/domoticz/setupVars.conf

useUpdateVars=false

Dest_folder=""
IPv4_address=""
Enable_http=true
Enable_https=true
HTTP_port="8080"
HTTPS_port="443"
Current_user=""

lowercase(){
    echo "$1" | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"
}

OS=`lowercase \`uname -s\``
MACH=`uname -m`
ARCH=`dpkg --print-architecture`
if [ ${MACH} = "armv6l" ] || [ ${ARCH} = "armhf" ]
then
 MACH="armv7l"
fi

# Find the rows and columns will default to 80x24 is it can not be detected
screen_size=$(stty size 2>/dev/null || echo 24 80) 
rows=$(echo $screen_size | awk '{print $1}')
columns=$(echo $screen_size | awk '{print $2}')

# Divide by two so the dialogs take up half of the screen, which looks nice.
r=$(( rows / 2 ))
c=$(( columns / 2 ))
# Unless the screen is tiny
r=$(( r < 20 ? 20 : r ))
c=$(( c < 70 ? 70 : c ))

######## Undocumented Flags. Shhh ########
skipSpaceCheck=false
reconfigure=false

######## FIRST CHECK ########
# Must be root to install
echo ":::"
if [[ ${EUID} -eq 0 ]]; then
	echo "::: You are root."
else
	echo "::: Script called with non-root privileges. The Domoticz installs server packages and configures"
	echo "::: system networking, it requires elevated rights. Please check the contents of the script for"
	echo "::: any concerns with this requirement. Please be sure to download this script from a trusted source."
	echo ":::"
	echo "::: Detecting the presence of the sudo utility for continuation of this install..."

	if [ -x "$(command -v sudo)" ]; then
		echo "::: Utility sudo located."
		exec curl -sSL https://install.domoticz.com | sudo bash "$@"
		exit $?
	else
		echo "::: sudo is needed for the Web interface to run domoticz commands.  Please run this script as root and it will be automatically installed."
		exit 1
	fi
fi

# Compatibility

if [ -x "$(command -v apt-get)" ]; then
	#Debian Family
	#############################################
	PKG_MANAGER="apt-get"
	PKG_CACHE="/var/lib/apt/lists/"
	UPDATE_PKG_CACHE="${PKG_MANAGER} update"
	PKG_UPDATE="${PKG_MANAGER} upgrade"
	PKG_INSTALL="${PKG_MANAGER} --yes --fix-missing install"
	# grep -c will return 1 retVal on 0 matches, block this throwing the set -e with an OR TRUE
	PKG_COUNT="${PKG_MANAGER} -s -o Debug::NoLocking=true upgrade | grep -c ^Inst || true"
	INSTALLER_DEPS=( apt-utils whiptail git)
	domoticz_DEPS=( curl unzip wget sudo cron libudev-dev)

        DEBIAN_ID=$(grep -oP '(?<=^ID=).+' /etc/*-release | tr -d '"')
        DEBIAN_VERSION=$(grep -oP '(?<=^VERSION_ID=).+' /etc/*-release | tr -d '"')

	if test ${DEBIAN_VERSION} -lt 10
	then
		domoticz_DEPS=( ${domoticz_DEPS[@]} libcurl3 )
	else
		domoticz_DEPS=( ${domoticz_DEPS[@]} libcurl4 libusb-0.1)
	fi;

	package_check_install() {
		dpkg-query -W -f='${Status}' "${1}" 2>/dev/null | grep -c "ok installed" || ${PKG_INSTALL} "${1}"
	}
elif [ -x "$(command -v rpm)" ]; then
	# Fedora Family
	if [ -x "$(command -v dnf)" ]; then
		PKG_MANAGER="dnf"
	else
		PKG_MANAGER="yum"
	fi
	PKG_CACHE="/var/cache/${PKG_MANAGER}"
	UPDATE_PKG_CACHE="${PKG_MANAGER} check-update"
	PKG_UPDATE="${PKG_MANAGER} update -y"
	PKG_INSTALL="${PKG_MANAGER} install -y"
	PKG_COUNT="${PKG_MANAGER} check-update | egrep '(.i686|.x86|.noarch|.arm|.src)' | wc -l"
	INSTALLER_DEPS=( procps-ng newt git )
	domoticz_DEPS=( curl libcurl4 unzip wget findutils cronie sudo domoticz_DEP)
	if grep -q 'Fedora' /etc/redhat-release; then
		remove_deps=(epel-release);
		domoticz_DEPS=( ${domoticz_DEPS[@]/$remove_deps} );
	fi
	package_check_install() {
		rpm -qa | grep ^"${1}"- > /dev/null || ${PKG_INSTALL} "${1}"
	}
else
	echo "OS distribution not supported"
	exit
fi

####### FUNCTIONS ##########
spinner() {
	local pid=$1
	local delay=0.50
	local spinstr='/-\|'
	while [ "$(ps a | awk '{print $1}' | grep "${pid}")" ]; do
		local temp=${spinstr#?}
		printf " [%c]  " "${spinstr}"
		local spinstr=${temp}${spinstr%"$temp"}
		sleep ${delay}
		printf "\b\b\b\b\b\b"
	done
	printf "    \b\b\b\b"
}

find_current_user() {
	# Find current user
	Current_user=${SUDO_USER:-$USER}
	echo "::: Current User: ${Current_user}"
}

find_IPv4_information() {
	# Find IP used to route to outside world
	IPv4dev=$(ip route get 8.8.8.8 | awk '{for(i=1;i<=NF;i++)if($i~/dev/)print $(i+1)}')
	IPv4_address=$(ip -o -f inet addr show dev "$IPv4dev" | awk '{print $4}' | awk 'END {print}')
	IPv4gw=$(ip route get 8.8.8.8 | awk '{print $3}')
}

welcomeDialogs() {
	# Display the welcome dialog
	whiptail --msgbox --backtitle "Welcome" --title "Domoticz automated installer" "\n\nThis installer will transform your device into a Home Automation System!\n\n
Domoticz is free, but powered by your donations at:  http://www.domoticz.com\n\n
Domoticz is a SERVER so it needs a STATIC IP ADDRESS to function properly.
	" ${r} ${c}
}

displayFinalMessage() {
	# Final completion message to user
	whiptail --msgbox --backtitle "Ready..." --title "Installation Complete!" "Point your browser to either:

HTTP:	${IPv4_address%/*}:${HTTP_port%/*}
HTTPS:	${IPv4_address%/*}:${HTTPS_port}
User/Password:   admin/domoticz 
Modify password asap in menu Setup - MyProfile

Wiki:  https://www.domoticz.com/wiki
Forum: https://www.domoticz.com/forum

The install log is in /etc/domoticz." ${r} ${c}
}

verifyFreeDiskSpace() {

	# 50MB is the minimum space needed
	# - Fourdee: Local ensures the variable is only created, and accessible within this function/void. Generally considered a "good" coding practice for non-global variables.
	echo "::: Verifying free disk space..."
	local required_free_kilobytes=51200
	local existing_free_kilobytes=$(df -Pk | grep -m1 '\/$' | awk '{print $4}')

	# - Unknown free disk space , not a integer
	if ! [[ "${existing_free_kilobytes}" =~ ^([0-9])+$ ]]; then
		echo "::: Unknown free disk space!"
		echo "::: We were unable to determine available free disk space on this system."
		echo "::: You may override this check and force the installation, however, it is not recommended"
		echo "::: To do so, pass the argument '--i_do_not_follow_recommendations' to the install script"
		echo "::: eg. curl -L https://install.domoticz.com | bash /dev/stdin --i_do_not_follow_recommendations"
		exit 1
	# - Insufficient free disk space
	elif [[ ${existing_free_kilobytes} -lt ${required_free_kilobytes} ]]; then
		echo "::: Insufficient Disk Space!"
		echo "::: Your system appears to be low on disk space. Domoticz recommends a minimum of $required_free_kilobytes KiloBytes."
		echo "::: You only have ${existing_free_kilobytes} KiloBytes free."
		echo "::: If this is a new install you may need to expand your disk."
		echo "::: Try running 'sudo raspi-config', and choose the 'expand file system option'"
		echo "::: After rebooting, run this installation again. (curl -L https://install.domoticz.com | bash)"

		echo "Insufficient free space, exiting..."
		exit 1

	fi

}

chooseServices() {
	Enable_http=false;
	Enable_https=false;
	# Let use enable HTTP and/or HTTPS
	cmd=(whiptail --separate-output --checklist "Select Services (press space to select)" ${r} ${c} 2)
	options=(HTTP "Enables HTTP access" on
	HTTPS "Enabled HTTPS access" on)
	choices=$("${cmd[@]}" "${options[@]}" 2>&1 >/dev/tty)
	if [[ $? = 0 ]];then
		for choice in ${choices}
		do
			case ${choice} in
			HTTP  )   Enable_http=true;;
			HTTPS  )   Enable_https=true;;
			esac
		done
		if [ ! ${Enable_http} ] && [ ! ${Enable_https} ]; then
			echo "::: Cannot continue, neither HTTP or HTTPS selected"
			echo "::: Exiting"
			exit 1
		fi
	else
		echo "::: Cancel selected. Exiting..."
		exit 1
	fi
	# Configure the port(s)
	if [ "$Enable_http" = true ] ; then
		HTTP_port=$(whiptail --inputbox "HTTP Port number:" ${r} ${c} ${HTTP_port} --title "Configure HTTP" 3>&1 1>&2 2>&3)
		exitstatus=$?
		if [ $exitstatus = 0 ]; then
			echo "HTTP Port: " $HTTP_port
		else
			echo "::: Cancel selected. Exiting..."
			exit 1
		fi	
	fi    
	if [ "$Enable_https" = true ] ; then
		HTTPS_port=$(whiptail --inputbox "HTTPS Port number:" ${r} ${c} ${HTTPS_port} --title "Configure HTTPS" 3>&1 1>&2 2>&3)
		exitstatus=$?
		if [ $exitstatus = 0 ]; then
			echo "HTTPS Port: " $HTTPS_port
		else
			echo "::: Cancel selected. Exiting..."
			exit 1
		fi	
	fi
}

chooseDestinationFolder() {
	Dest_folder=$(whiptail --inputbox "Installation Folder:" ${r} ${c} ${Dest_folder} --title "Destination" 3>&1 1>&2 2>&3)
	exitstatus=$?
	if [ $exitstatus = 0 ]; then
		echo ":::"
	else
		echo "::: Cancel selected. Exiting..."
		exit 1
	fi	
}

stop_service() {
	# Stop service passed in as argument.
	echo ":::"
	echo -n "::: Stopping ${1} service..."
	if [ -x "$(command -v service)" ]; then
		service "${1}" stop &> /dev/null & spinner $! || true
	fi
	echo " done."
}

start_service() {
	# Start/Restart service passed in as argument
	# This should not fail, it's an error if it does
	echo ":::"
	echo -n "::: Starting ${1} service..."
	if [ -x "$(command -v service)" ]; then
		service "${1}" restart &> /dev/null  & spinner $!
	fi
	echo " done."
}

enable_service() {
	# Enable service so that it will start with next reboot
	echo ":::"
	echo -n "::: Enabling ${1} service to start on reboot..."
	if [ -x "$(command -v service)" ]; then
		update-rc.d "${1}" defaults &> /dev/null  & spinner $!
	fi
	echo " done."
}

update_package_cache() {
	#Running apt-get update/upgrade with minimal output can cause some issues with
	#requiring user input (e.g password for phpmyadmin see #218)

	#Check to see if apt-get update has already been run today
	#it needs to have been run at least once on new installs!
	timestamp=$(stat -c %Y ${PKG_CACHE})
	timestampAsDate=$(date -d @"${timestamp}" "+%b %e")
	today=$(date "+%b %e")

	if [ ! "${today}" == "${timestampAsDate}" ]; then
		#update package lists
		echo ":::"
		echo -n "::: ${PKG_MANAGER} update has not been run today. Running now..."
		${UPDATE_PKG_CACHE} &> /dev/null & spinner $!
		echo " done!"
	fi
}

notify_package_updates_available() {
  # Let user know if they have outdated packages on their system and
  # advise them to run a package update at soonest possible.
	echo ":::"
	echo -n "::: Checking ${PKG_MANAGER} for upgraded packages...."
	updatesToInstall=$(eval "${PKG_COUNT}")
	echo " done!"
	echo ":::"
	if [[ ${updatesToInstall} -eq "0" ]]; then
		echo "::: Your system is up to date! Continuing with Domoticz installation..."
	else
		echo "::: There are ${updatesToInstall} updates available for your system!"
		echo "::: We recommend you run '${PKG_UPDATE}' after installing Domoticz! "
		echo ":::"
	fi
}

install_dependent_packages() {
	# Install packages passed in via argument array
	# No spinner - conflicts with set -e
	declare -a argArray1=("${!1}")

	for i in "${argArray1[@]}"; do
		echo -n ":::    Checking for $i..."
		package_check_install "${i}" &> /dev/null
		echo " installed!"
	done
}

finalExports() {
	#If it already exists, lets overwrite it with the new values.
	if [[ -f ${setupVars} ]]; then
		rm ${setupVars}
	fi
    {
	echo "Dest_folder=${Dest_folder}"
	echo "Enable_http=${Enable_http}"
	echo "HTTP_port=${HTTP_port}"
	echo "Enable_https=${Enable_https}"
	echo "HTTPS_port=${HTTPS_port}"
    }>> "${setupVars}"
}

downloadDomoticzWeb() {
	echo "::: Destination folder=${Dest_folder}"
	if [[ ! -e $Dest_folder ]]; then
		echo "::: Creating ${Dest_folder}"
		mkdir $Dest_folder
		chown "${Current_user}":"${Current_user}" $Dest_folder
	fi
	cd $Dest_folder
	wget -O domoticz_release.tgz "http://www.domoticz.com/download.php?channel=release&type=release&system=${OS}&machine=${MACH}"
	echo "::: Unpacking Domoticz..."
	tar xvfz domoticz_release.tgz
	rm domoticz_release.tgz
	Database_file="${Dest_folder}/domoticz.db"
	if [ ! -f $Database_file ]; then
		echo "Creating database..."
		touch $Database_file
		chmod 644 $Database_file
		chown "${Current_user}":"${Current_user}" $Database_file
	fi
}

makeStartupScript() {
	cp "${Dest_folder}/domoticz.sh" /tmp/domoticz_tmp_ss1

    #configure the script
    cat /tmp/domoticz_tmp_ss1 | sed -e "s/USERNAME=pi/USERNAME=${Current_user}/" > /tmp/domoticz_tmp_ss2
    rm /tmp/domoticz_tmp_ss1
    
    local http_port="${HTTP_port}"
    local https_port="${HTTPS_port}"
	if [ "$Enable_http" = false ] ; then
		http_port="0"
	fi    
	if [ "$Enable_https" = false ] ; then
		https_port="0"
	fi    
    
    cat /tmp/domoticz_tmp_ss2 | sed -e "s/-www 8080/-www ${http_port}/" > /tmp/domoticz_tmp_ss1
    rm /tmp/domoticz_tmp_ss2
    cat /tmp/domoticz_tmp_ss1 | sed -e "s/-sslwww 443/-sslwww ${https_port}/" > /tmp/domoticz_tmp_ss2
    rm /tmp/domoticz_tmp_ss1
    cat /tmp/domoticz_tmp_ss2 | sed -e "s%/home/\$USERNAME/domoticz%${Dest_folder}%" > /tmp/domoticz_tmp_ss1
    rm /tmp/domoticz_tmp_ss2
    
    mv /tmp/domoticz_tmp_ss1 /etc/init.d/domoticz.sh
	chmod +x /etc/init.d/domoticz.sh
	update-rc.d domoticz.sh defaults
}

installdomoticz() {
	# Install base files
	downloadDomoticzWeb
	makeStartupScript
	finalExports
}

updatedomoticz() {
	# Source ${setupVars} for use in the rest of the functions.
	. ${setupVars}
	# Install base files
	downloadDomoticzWeb
}

update_dialogs() {
	# reconfigure
	if [ "${reconfigure}" = true ]; then
		opt1a="Repair"
		opt1b="This will retain existing settings"
		strAdd="You will remain on the same version"
	else
		opt1a="Update"
		opt1b="This will retain existing settings."
		strAdd="You will be updated to the latest version."
	fi
	opt2a="Reconfigure"
	opt2b="This will allow you to enter new settings"

	UpdateCmd=$(whiptail --title "Existing Install Detected!" --menu "\n\nWe have detected an existing install.\n\nPlease choose from the following options: \n($strAdd)" ${r} ${c} 2 \
	"${opt1a}"  "${opt1b}" \
	"${opt2a}"  "${opt2b}" 3>&2 2>&1 1>&3)

	if [[ $? = 0 ]];then
		case ${UpdateCmd} in
			${opt1a})
				echo "::: ${opt1a} option selected."
				useUpdateVars=true
				;;
			${opt2a})
				echo "::: ${opt2a} option selected"
				useUpdateVars=false
				;;
		esac
	else
		echo "::: Cancel selected. Exiting..."
		exit 1
	fi

}

install_packages() {
	# Update package cache
	update_package_cache

	# Notify user of package availability
	notify_package_updates_available

	# Install packages used by this installation script
	install_dependent_packages INSTALLER_DEPS[@]

	# Install packages used by the Domoticz
	install_dependent_packages domoticz_DEPS[@]
}

main() {
# Check arguments for the undocumented flags
	for var in "$@"; do
		case "$var" in
			"--reconfigure"  ) reconfigure=true;;
			"--i_do_not_follow_recommendations"   ) skipSpaceCheck=false;;
			"--unattended"     ) runUnattended=true;;
		esac
	done

	if [[ -f ${setupVars} ]]; then
		if [[ "${runUnattended}" == true ]]; then
			echo "::: --unattended passed to install script, no whiptail dialogs will be displayed"
			useUpdateVars=true
		else
			update_dialogs
		fi
	fi

	# Start the installer
	# Verify there is enough disk space for the install
	if [[ "${skipSpaceCheck}" == true ]]; then
		echo "::: --i_do_not_follow_recommendations passed to script, skipping free disk space verification!"
	else
		verifyFreeDiskSpace
	fi

	install_packages

	if [[ "${reconfigure}" == true ]]; then
		echo "::: --reconfigure passed to install script. Not downloading/updating local installation"
	else
		echo "::: Downloading Domoticz"
	fi
	
	find_current_user
	
	Dest_folder="/home/${Current_user}/domoticz"
	
	find_IPv4_information

	if [[ ${useUpdateVars} == false ]]; then
		# Display welcome dialogs
		welcomeDialogs
		# Create directory for Domoticz storage
		mkdir -p /etc/domoticz/
		# Install and log everything to a file
		chooseServices
		chooseDestinationFolder
		installdomoticz
	else
		updatedomoticz
	fi

	if [[ "${useUpdateVars}" == false ]]; then
	    displayFinalMessage
	fi

	echo "::: Restarting services..."
	# Start services
	enable_service domoticz.sh
	start_service domoticz.sh
	echo "::: done."

	echo ":::"
	if [[ "${useUpdateVars}" == false ]]; then
		echo "::: Installation Complete! Configure your browser to use the Domoticz using:"
		echo ":::     ${IPv4_address%/*}:${HTTP_port}"
		echo ":::     ${IPv4_address%/*}:${HTTPS_port}"
	else
		echo "::: Update complete!"
	fi
}

main "$@"
