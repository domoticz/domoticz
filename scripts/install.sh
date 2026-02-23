#!/usr/bin/env bash
# Domoticz: Open Source Home Automation System
# (c) 2012-2026 by GizMoCuz
# Big thanks to Jacob Salmela! (This is based on the excelent Pi-Hole install script ;)
# https://www.domoticz.com
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

######## COLORS & FORMATTING #########
if [[ -t 1 ]]; then
    COL_NC='\e[0m'
    COL_GREEN='\e[0;32m'
    COL_RED='\e[0;31m'
    COL_YELLOW='\e[0;33m'
    COL_BLUE='\e[0;34m'
    COL_BOLD='\e[1m'
else
    COL_NC=''
    COL_GREEN=''
    COL_RED=''
    COL_YELLOW=''
    COL_BLUE=''
    COL_BOLD=''
fi

TICK="[${COL_GREEN}\xE2\x9C\x93${COL_NC}]"
CROSS="[${COL_RED}\xE2\x9C\x97${COL_NC}]"
INFO="[${COL_BLUE}i${COL_NC}]"
WARN="[${COL_YELLOW}\xE2\x9A\xA0${COL_NC}]"

msg_info() {
    printf "  ${INFO} %s\n" "$@"
}

msg_ok() {
    printf "  ${TICK} %s\n" "$@"
}

msg_error() {
    printf "  ${CROSS} %s\n" "$@"
}

msg_warn() {
    printf "  ${WARN} %s\n" "$@"
}

msg_header() {
    local term_width=60
    local line
    line=$(printf '%*s' "$term_width" '' | tr ' ' '─')
    printf "\n  ${COL_BOLD}%s${COL_NC}\n" "$@"
    printf "  %s\n\n" "$line"
}

show_banner() {
    printf "${COL_BLUE}"
    cat << 'BANNER'

    ____                        __  _
   / __ \____  ____ ___  ____  / /_(_)_______
  / / / / __ \/ __ `__ \/ __ \/ __/ / ___/_  /
 / /_/ / /_/ / / / / / / /_/ / /_/ / /__  / /_
/_____/\____/_/ /_/ /_/\____/\__/_/\___/ /___/

   Home Automation System Installer

BANNER
    printf "${COL_NC}"
    printf "  ────────────────────────────────────────────────────────────\n\n"
}

lowercase(){
    echo "$1" | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"
}

OS=`lowercase \`uname -s\``
MACH=`uname -m`
ARCH=""
if [ -x "$(command -v dpkg)" ]; then
    ARCH=$(dpkg --print-architecture)
fi
OPENSSL_VERSION=$(openssl version -v | awk '{print $2}' | sed 's/\..*//')

if [ "${MACH}" = "armv6l" ] || [ "${ARCH}" = "armhf" ]
then
 MACH="armv7l"
fi

if [  ${OPENSSL_VERSION} -ne 3 ]; then
 msg_error "OpenSSL version 3 required!"
 exit 1
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
show_banner
if [[ ${EUID} -eq 0 ]]; then
	msg_ok "Running as root"
else
	msg_error "Script called with non-root privileges."
	msg_info "The Domoticz installer needs elevated rights to install packages"
	msg_info "and configure system networking."
	msg_info "Detecting the presence of the sudo utility..."

	if [ -x "$(command -v sudo)" ]; then
		msg_ok "Utility sudo located"
		exec curl -sSL https://install.domoticz.com | sudo bash -s -- "$@"
		exit $?
	else
		msg_error "sudo is needed for the Web interface to run domoticz commands."
		msg_error "Please run this script as root and it will be automatically installed."
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
	domoticz_DEPS=( curl unzip wget sudo cron libudev-dev libmosquitto1)

        DEBIAN_ID=$(grep -oP '(?<=^ID=).+' /etc/*-release | tr -d '"')
        DEBIAN_VERSION=$(grep -oP '(?<=^VERSION_ID=).+' /etc/*-release | tr -d '"')

	if test ${DEBIAN_VERSION} -lt 12; then
		msg_error "Debian Bookworm (12) or later is required!"
		exit 1
	fi
	domoticz_DEPS=( ${domoticz_DEPS[@]} libcurl4 libusb-0.1)

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
	domoticz_DEPS=( curl libcurl unzip wget findutils cronie sudo mosquitto)
	if grep -q 'Fedora' /etc/redhat-release; then
		remove_deps=(epel-release);
		domoticz_DEPS=( ${domoticz_DEPS[@]/$remove_deps} );
	fi
	package_check_install() {
		rpm -qa | grep ^"${1}"- > /dev/null || ${PKG_INSTALL} "${1}"
	}
else
	msg_error "OS distribution not supported"
	exit 1
fi

####### FUNCTIONS ##########
spinner() {
	local pid=$1
	local msg="${2:-}"
	local delay=0.1
	local spinchars='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
	local i=0
	local len=${#spinchars}
	while kill -0 "$pid" 2>/dev/null; do
		local char="${spinchars:$i:1}"
		printf "\r  [${COL_BLUE}%s${COL_NC}] %s" "$char" "$msg"
		i=$(( (i + 1) % len ))
		sleep ${delay}
	done
	wait "$pid" 2>/dev/null || true
	printf "\r  ${TICK} %s\n" "$msg"
}

find_current_user() {
	# Find current user
	Current_user=${SUDO_USER:-$USER}
	msg_ok "Current user: ${Current_user}"
}

find_IPv4_information() {
	# Find IP used to route to outside world
	IPv4dev=$(ip route get 8.8.8.8 | awk '{for(i=1;i<=NF;i++)if($i~/dev/)print $(i+1)}')
	IPv4_address=$(ip -o -f inet addr show dev "$IPv4dev" | awk '{print $4}' | awk 'END {print}')
	IPv4gw=$(ip route get 8.8.8.8 | awk '{print $3}')
}

welcomeDialogs() {
	# Display the welcome dialog
	whiptail --msgbox --backtitle "Domoticz Installer" --title "Welcome to Domoticz" "\
This installer will transform your device into a powerful Home Automation System!

Domoticz is free and open source, powered by your donations at:
  https://www.domoticz.com

NOTE: Domoticz is a server application and requires a STATIC IP ADDRESS to function properly.\
" ${r} ${c}
}

displayFinalMessage() {
	local http_line=""
	local https_line=""
	if [ "$Enable_http" = true ]; then
		http_line="  HTTP  :  ${IPv4_address%/*}:${HTTP_port%/*}"
	fi
	if [ "$Enable_https" = true ]; then
		https_line="  HTTPS :  ${IPv4_address%/*}:${HTTPS_port}"
	fi

	whiptail --msgbox --backtitle "Domoticz Installer" --title "Installation Complete!" "\
Point your browser to:
${http_line}
${https_line}

Default credentials:
  User     :  admin
  Password :  domoticz
  (Change your password in Setup > My Profile)

Resources:
  Wiki  :  https://www.domoticz.com/wiki
  Forum :  https://www.domoticz.com/forum

The install log is saved in /etc/domoticz.\
" ${r} ${c}
}

verifyFreeDiskSpace() {

	# 50MB is the minimum space needed
	msg_info "Verifying free disk space..."
	local required_free_kilobytes=51200
	local existing_free_kilobytes=$(df -Pk | grep -m1 '\/$' | awk '{print $4}')

	# - Unknown free disk space , not a integer
	if ! [[ "${existing_free_kilobytes}" =~ ^([0-9])+$ ]]; then
		msg_error "Unknown free disk space!"
		msg_error "We were unable to determine available free disk space on this system."
		msg_warn "You may override this check with the '--i_do_not_follow_recommendations' flag"
		msg_info "eg. curl -L https://install.domoticz.com | bash /dev/stdin --i_do_not_follow_recommendations"
		exit 1
	# - Insufficient free disk space
	elif [[ ${existing_free_kilobytes} -lt ${required_free_kilobytes} ]]; then
		msg_error "Insufficient disk space!"
		msg_error "Domoticz requires a minimum of $required_free_kilobytes KiloBytes."
		msg_error "You only have ${existing_free_kilobytes} KiloBytes free."
		msg_warn "If this is a new install you may need to expand your disk."
		msg_info "Try running 'sudo raspi-config' and choose 'expand file system'"
		msg_info "After rebooting, run this installation again."
		exit 1
	fi
	msg_ok "Sufficient disk space available"
}

chooseServices() {
	Enable_http=false;
	Enable_https=false;
	# Let user enable HTTP and/or HTTPS
	cmd=(whiptail --separate-output --checklist "Select the protocols to enable (use Space to toggle):" ${r} ${c} 2)
	options=(HTTP "Enable HTTP access" on
	HTTPS "Enable HTTPS access" on)
	choices=$("${cmd[@]}" "${options[@]}" 2>&1 >/dev/tty)
	if [[ $? = 0 ]];then
		for choice in ${choices}
		do
			case ${choice} in
			HTTP  )   Enable_http=true;;
			HTTPS  )   Enable_https=true;;
			esac
		done
		if [ "${Enable_http}" = false ] && [ "${Enable_https}" = false ]; then
			msg_error "Cannot continue: neither HTTP nor HTTPS selected"
			exit 1
		fi
	else
		msg_warn "Cancel selected. Exiting..."
		exit 1
	fi
	# Configure the port(s)
	if [ "$Enable_http" = true ] ; then
		HTTP_port=$(whiptail --inputbox "Enter the HTTP port number:" ${r} ${c} ${HTTP_port} --title "HTTP Port Configuration" 3>&1 1>&2 2>&3)
		exitstatus=$?
		if [ $exitstatus = 0 ]; then
			msg_ok "HTTP port: ${HTTP_port}"
		else
			msg_warn "Cancel selected. Exiting..."
			exit 1
		fi
	fi
	if [ "$Enable_https" = true ] ; then
		HTTPS_port=$(whiptail --inputbox "Enter the HTTPS port number:" ${r} ${c} ${HTTPS_port} --title "HTTPS Port Configuration" 3>&1 1>&2 2>&3)
		exitstatus=$?
		if [ $exitstatus = 0 ]; then
			msg_ok "HTTPS port: ${HTTPS_port}"
		else
			msg_warn "Cancel selected. Exiting..."
			exit 1
		fi
	fi
}

chooseDestinationFolder() {
	Dest_folder=$(whiptail --inputbox "Enter the installation folder:" ${r} ${c} ${Dest_folder} --title "Installation Destination" 3>&1 1>&2 2>&3)
	exitstatus=$?
	if [ $exitstatus = 0 ]; then
		msg_ok "Destination folder: ${Dest_folder}"
	else
		msg_warn "Cancel selected. Exiting..."
		exit 1
	fi
}

displaySummary() {
	local http_status="Disabled"
	local https_status="Disabled"
	local http_detail=""
	local https_detail=""

	if [ "$Enable_http" = true ]; then
		http_status="Enabled (port ${HTTP_port})"
	fi
	if [ "$Enable_https" = true ]; then
		https_status="Enabled (port ${HTTPS_port})"
	fi

	whiptail --yesno --backtitle "Domoticz Installer" --title "Installation Summary" "\
Please review your settings before installation begins:

  Install folder :  ${Dest_folder}
  HTTP           :  ${http_status}
  HTTPS          :  ${https_status}
  User           :  ${Current_user}

Proceed with installation?" ${r} ${c}

	if [[ $? -ne 0 ]]; then
		msg_warn "Installation cancelled by user."
		exit 1
	fi
}

stop_service() {
	# Stop service passed in as argument
	if systemctl is-active --quiet "${1}" 2>/dev/null; then
		systemctl stop "${1}" &> /dev/null & spinner $! "Stopping ${1} service..."
	else
		msg_ok "Service ${1} is not running"
	fi
}

start_service() {
	# Start/Restart service passed in as argument
	systemctl restart "${1}" &> /dev/null & spinner $! "Starting ${1} service..."
}

enable_service() {
	# Enable service so that it will start with next reboot
	systemctl enable "${1}" &> /dev/null & spinner $! "Enabling ${1} to start on boot..."
}

migrate_from_sysv() {
	# Migrate from legacy SysV init script to systemd service
	local sysv_script="/etc/init.d/domoticz.sh"

	if [[ ! -f "${sysv_script}" ]]; then
		return 0
	fi

	msg_info "Legacy SysV init script detected, migrating to systemd..."

	# Stop the old SysV service if running
	if service domoticz.sh status &> /dev/null; then
		service domoticz.sh stop &> /dev/null & spinner $! "Stopping legacy SysV service..."
	fi

	# Remove SysV boot links (Debian/Ubuntu)
	if [ -x "$(command -v update-rc.d)" ]; then
		update-rc.d -f domoticz.sh remove &> /dev/null || true
	fi

	# Remove SysV boot links (RPM-based: CentOS, Fedora, RHEL)
	if [ -x "$(command -v chkconfig)" ]; then
		chkconfig domoticz.sh off &> /dev/null || true
		chkconfig --del domoticz.sh &> /dev/null || true
	fi

	# Remove the old init script
	rm -f "${sysv_script}"
	msg_ok "Legacy SysV init script removed"

	# Reload systemd so it picks up the removal
	systemctl daemon-reload &> /dev/null || true
	msg_ok "Migrated to systemd service"
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
		${UPDATE_PKG_CACHE} &> /dev/null & spinner $! "Updating package cache (${PKG_MANAGER})..."
	else
		msg_ok "Package cache is up to date"
	fi
}

notify_package_updates_available() {
  # Let user know if they have outdated packages on their system and
  # advise them to run a package update at soonest possible.
	msg_info "Checking ${PKG_MANAGER} for available upgrades..."
	updatesToInstall=$(eval "${PKG_COUNT}")
	if [[ ${updatesToInstall} -eq "0" ]]; then
		msg_ok "System is up to date"
	else
		msg_warn "${updatesToInstall} updates available for your system"
		msg_info "We recommend running '${PKG_UPDATE}' after installing Domoticz"
	fi
}

install_dependent_packages() {
	# Install packages passed in via argument array
	declare -a argArray1=("${!1}")
	local total=${#argArray1[@]}
	local current=0

	{
	for i in "${argArray1[@]}"; do
		current=$((current + 1))
		local pct=$(( current * 100 / total ))
		echo "XXX"
		echo "${pct}"
		echo "Checking / installing: ${i}"
		echo "XXX"
		package_check_install "${i}" &> /dev/null
	done
	} | whiptail --title "Installing Packages" --gauge "Preparing..." ${r} ${c} 0

	msg_ok "All required packages installed"
}

finalExports() {
	#If it already exists, lets overwrite it with the new values.
	if [[ -f ${setupVars} ]]; then
		rm "${setupVars}"
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
	msg_info "Destination folder: ${Dest_folder}"
	if [[ ! -e "$Dest_folder" ]]; then
		msg_info "Creating ${Dest_folder}"
		mkdir "$Dest_folder"
		chown "${Current_user}":"${Current_user}" "$Dest_folder"
		msg_ok "Created ${Dest_folder}"
	fi
	cd "$Dest_folder"
	msg_info "Downloading Domoticz release..."
	wget -q -O domoticz_release.tgz "https://www.domoticz.com/download.php?channel=release&type=release&system=${OS}&machine=${MACH}"
	msg_ok "Download complete"
	msg_info "Unpacking Domoticz..."
	tar xfz domoticz_release.tgz
	rm domoticz_release.tgz
	msg_ok "Domoticz unpacked"
	Database_file="${Dest_folder}/domoticz.db"
	if [ ! -f "$Database_file" ]; then
		msg_info "Creating database..."
		touch "$Database_file"
		chmod 644 "$Database_file"
		chown "${Current_user}":"${Current_user}" "$Database_file"
		msg_ok "Database created"
	fi
}

install_systemd_service() {
	local http_port="${HTTP_port}"
	local https_port="${HTTPS_port}"
	if [ "$Enable_http" = false ] ; then
		http_port="0"
	fi
	if [ "$Enable_https" = false ] ; then
		https_port="0"
	fi

	local service_file="/etc/systemd/system/domoticz.service"

	cat > "${service_file}" << EOF
[Unit]
Description=Domoticz Home Automation System
Documentation=https://www.domoticz.com/wiki
After=network-online.target time-sync.target
Wants=network-online.target

[Service]
Type=simple
User=${Current_user}
Group=${Current_user}
WorkingDirectory=${Dest_folder}
ExecStart=${Dest_folder}/domoticz -www ${http_port} -sslwww ${https_port}
ExecReload=/bin/kill -HUP \$MAINPID
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

	chmod 644 "${service_file}"
	systemctl daemon-reload
}

installdomoticz() {
	msg_header "Installing Domoticz"
	# Install base files
	downloadDomoticzWeb
	# Migrate from SysV init if upgrading from an older installation
	migrate_from_sysv
	install_systemd_service
	msg_ok "Systemd service installed"
	finalExports
	msg_ok "Configuration saved"
}

updatedomoticz() {
	# Source ${setupVars} for use in the rest of the functions.
	. ${setupVars}
	msg_header "Updating Domoticz"
	# Stop service before updating files
	stop_service domoticz.service
	# Migrate from SysV init if present from a previous installation
	migrate_from_sysv
	# Install base files
	downloadDomoticzWeb
	# Ensure systemd service is installed and up to date
	install_systemd_service
	msg_ok "Systemd service updated"
}

uninstall_domoticz() {
	if ! whiptail --yesno --backtitle "Domoticz Uninstaller" --title "Confirm Uninstall" "\
Are you sure you want to uninstall Domoticz?

This will:
  - Stop the Domoticz service
  - Remove the startup script
  - Remove the installation folder
  - Remove /etc/domoticz configuration

This action cannot be undone!" ${r} ${c}; then
		msg_info "Uninstall cancelled"
		exit 0
	fi

	msg_header "Uninstalling Domoticz"

	# Source setupVars to find install folder
	if [[ -f ${setupVars} ]]; then
		. ${setupVars}
	fi

	# Stop and remove systemd service
	if [[ -f /etc/systemd/system/domoticz.service ]]; then
		msg_info "Stopping domoticz service..."
		systemctl stop domoticz.service &> /dev/null || true
		systemctl disable domoticz.service &> /dev/null || true
		rm -f /etc/systemd/system/domoticz.service
		systemctl daemon-reload &> /dev/null || true
		msg_ok "Systemd service removed"
	fi

	# Remove legacy SysV init script if present
	if [[ -f /etc/init.d/domoticz.sh ]]; then
		service domoticz.sh stop &> /dev/null || true
		if [ -x "$(command -v update-rc.d)" ]; then
			update-rc.d -f domoticz.sh remove &> /dev/null || true
		fi
		if [ -x "$(command -v chkconfig)" ]; then
			chkconfig domoticz.sh off &> /dev/null || true
			chkconfig --del domoticz.sh &> /dev/null || true
		fi
		rm -f /etc/init.d/domoticz.sh
		msg_ok "Legacy init script removed"
	fi

	# Remove install folder
	if [[ -n "${Dest_folder}" && -d "${Dest_folder}" ]]; then
		rm -rf "${Dest_folder}"
		msg_ok "Installation folder removed (${Dest_folder})"
	else
		msg_warn "Installation folder not found or not set"
	fi

	# Remove config
	if [[ -d /etc/domoticz ]]; then
		rm -rf /etc/domoticz
		msg_ok "Configuration removed (/etc/domoticz)"
	fi

	msg_ok "Domoticz has been uninstalled"
}

update_dialogs() {
	# reconfigure
	if [ "${reconfigure}" = true ]; then
		opt1a="Repair"
		opt1b="Retain existing settings, same version"
		strAdd="You will remain on the same version."
	else
		opt1a="Update"
		opt1b="Retain existing settings, update to latest"
		strAdd="You will be updated to the latest version."
	fi
	opt2a="Reconfigure"
	opt2b="Enter new settings for your installation"
	opt3a="Uninstall"
	opt3b="Remove Domoticz from this system"

	UpdateCmd=$(whiptail --title "Existing Install Detected" --menu "\
An existing Domoticz installation was found.
${strAdd}

Choose an option:" ${r} ${c} 3 \
	"${opt1a}"  "${opt1b}" \
	"${opt2a}"  "${opt2b}" \
	"${opt3a}"  "${opt3b}" 3>&2 2>&1 1>&3)

	if [[ $? = 0 ]];then
		case ${UpdateCmd} in
			${opt1a})
				msg_ok "${opt1a} option selected"
				useUpdateVars=true
				;;
			${opt2a})
				msg_ok "${opt2a} option selected"
				useUpdateVars=false
				;;
			${opt3a})
				uninstall_domoticz
				exit 0
				;;
		esac
	else
		msg_warn "Cancel selected. Exiting..."
		exit 1
	fi

}

install_packages() {
	msg_header "Package Installation"

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
			"--i_do_not_follow_recommendations"   ) skipSpaceCheck=true;;
			"--unattended"     ) runUnattended=true;;
			"--uninstall"      ) uninstall_domoticz; exit 0;;
		esac
	done

	if [[ -f ${setupVars} ]]; then
		if [[ "${runUnattended}" == true ]]; then
			msg_info "Unattended mode: skipping dialogs"
			useUpdateVars=true
		else
			update_dialogs
		fi
	fi

	# Start the installer
	# Verify there is enough disk space for the install
	if [[ "${skipSpaceCheck}" == true ]]; then
		msg_warn "Skipping free disk space verification (--i_do_not_follow_recommendations)"
	else
		verifyFreeDiskSpace
	fi

	install_packages

	if [[ "${reconfigure}" == true ]]; then
		msg_info "Reconfigure mode: skipping download/update"
	else
		msg_info "Preparing Domoticz installation..."
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
		# Show summary before installing
		displaySummary
		installdomoticz
	else
		updatedomoticz
	fi

	if [[ "${useUpdateVars}" == false ]]; then
	    displayFinalMessage
	fi

	msg_header "Finalizing"
	# Start services
	enable_service domoticz.service
	start_service domoticz.service

	if [[ "${useUpdateVars}" == false ]]; then
		msg_ok "Installation complete!"
		msg_info "Access your Domoticz server at:"
		if [ "$Enable_http" = true ]; then
			msg_info "  HTTP  : ${IPv4_address%/*}:${HTTP_port}"
		fi
		if [ "$Enable_https" = true ]; then
			msg_info "  HTTPS : ${IPv4_address%/*}:${HTTPS_port}"
		fi
	else
		msg_ok "Update complete!"
	fi
	echo ""
}

main "$@"
