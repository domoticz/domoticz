#! /bin/sh
### BEGIN INIT INFO
# Provides:          domoticz
# Required-Start:    $network $remote_fs $syslog $time
# Required-Stop:     $network $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Home Automation System
# Description:       This daemon will start the Domoticz Home Automation System
### END INIT INFO

# Do NOT "set -e"

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
DESC="Domoticz Home Automation System"
NAME=domoticz
USERNAME=pi
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME

DAEMON=/home/$USERNAME/domoticz/$NAME
DAEMON_ARGS="-daemon"
#DAEMON_ARGS="$DAEMON_ARGS -daemonname $NAME -pidfile $PIDFILE"
DAEMON_ARGS="$DAEMON_ARGS -www 8080"
DAEMON_ARGS="$DAEMON_ARGS -sslwww 443"
#DAEMON_ARGS="$DAEMON_ARGS -log /tmp/domoticz.txt"
#DAEMON_ARGS="$DAEMON_ARGS -syslog"

# Exit if the package is not installed
[ -x "$DAEMON" ] || exit 0

# Load the VERBOSE setting and other rcS variables
. /lib/init/vars.sh

# Define LSB log_* functions.
# Depend on lsb-base (>= 3.2-14) to ensure that this file is present
# and status_of_proc is working.
. /lib/lsb/init-functions

pidof_domoticz() {
    # if there is actually a domoticz process whose pid is in PIDFILE,
    # print it and return 0.
    if [ -e "$PIDFILE" ]; then
        if pidof domoticz | tr ' ' '\n' | grep -w $(cat $PIDFILE); then
            return 0
        fi
    fi
    return 1
}

#
# Function that starts the daemon/service
#
do_start()
{
	# Script that waits for time synchronisation before starting Domoticz to prevent a crash
	# on a cold boot when no real time clock is available.

	# Check if systemd-timesyncd is enabled and running, otherwise ignore script
	if systemctl --quiet is-enabled systemd-timesyncd && systemctl --quiet is-active systemd-timesyncd; then
	    # Check if time is already synced, if so start Domoticz immediately
	    if [ -f "/run/systemd/timesync/synchronized" ]; then
		printf "Time synchronized, starting Domoticz...\n"
	    else
		# Check if custom time server(s) are defined
		if grep -q "^#NTP=" /etc/systemd/timesyncd.conf; then
		    printf "INFO: No time server(s) defined in /etc/systemd/timesyncd.conf, using default fallback server(s)\n"
		fi
		# Wait a maximum of 30 seconds until sync file is generated indicating successful time sync
		printf "Waiting for time synchronization before starting Domoticz"
		count=30
		while [ ! -f "/run/systemd/timesync/synchronized" ]
		do
		    count=$((count-1))
		    if [ $((count)) -lt 1 ]
		    then
			# If failed, print error message, exit loop and start Domoticz anyway
			printf "\nWARNING: Time synchronization failed, check network and /etc/systemd/timesyncd.conf\n"
			printf "Starting Domoticz without successful time synchronization...\n"
			break
		    fi
		    printf "."
		    sleep 1
		done
		#If the file was found in time, sync was successful and Domoticz will start immediately
		if [ -f "/run/systemd/timesync/synchronized" ]
		then
		    printf "\nTime synchronization successful, starting Domoticz...\n"
		fi
	    fi
	fi

	# Return
	#   0 if daemon has been started
	#   1 if daemon was already running
	#   2 if daemon could not be started
	start-stop-daemon --chuid $USERNAME --start --quiet --pidfile $PIDFILE --exec $DAEMON --test > /dev/null \
		|| return 1
	start-stop-daemon --start --quiet --pidfile $PIDFILE --exec $DAEMON -- \
		$DAEMON_ARGS \
		|| return 2
}

#
# Function that stops the daemon/service
#
do_stop()
{
        # Return
        #   0 if daemon has been stopped
        #   1 if daemon was already stopped
        #   2 if daemon could not be stopped
        #   other if a failure occurred
        start-stop-daemon --stop --quiet --retry=TERM/30/KILL/5 --pidfile $PIDFILE --name $NAME
        RETVAL="$?"
        [ "$RETVAL" = 2 ] && return 2
        # Wait for children to finish too if this is a daemon that forks
        # and if the daemon is only ever run from this initscript.
        # If the above conditions are not satisfied then add some other code
        # that waits for the process to drop all resources that could be
        # needed by services started subsequently.  A last resort is to
        # sleep for some time.
        start-stop-daemon --stop --quiet --oknodo --retry=0/30/KILL/5 --exec $DAEMON
        [ "$?" = 2 ] && return 2
        # Many daemons don't delete their pidfiles when they exit.
        rm -f $PIDFILE
        return "$RETVAL"
}

case "$1" in
  start)
        [ "$VERBOSE" != no ] && log_daemon_msg "Starting $DESC" "$NAME"
        do_start
        case "$?" in
                0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
                2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
        esac
        ;;
  stop)
        [ "$VERBOSE" != no ] && log_daemon_msg "Stopping $DESC" "$NAME"
        do_stop
        case "$?" in
                0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
                2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
        esac
        ;;
  status)
        status_of_proc "$DAEMON" "$NAME" && exit 0 || exit $?
        ;;
  reload)
        log_daemon_msg "Reloading $DESC" "$NAME"
        PID=$(pidof_domoticz) || true
        if [ "${PID}" ]; then
                kill -HUP $PID
                log_end_msg 0
        else
                log_end_msg 1
        fi
        ;;
  restart)
        log_daemon_msg "Restarting $DESC" "$NAME"
        do_stop
        case "$?" in
          0|1)
                do_start
                case "$?" in
                        0) log_end_msg 0 ;;
                        1) log_end_msg 1 ;; # Old process is still running
                        *) log_end_msg 1 ;; # Failed to start
                esac
                ;;
          *)
                # Failed to stop
                log_end_msg 1
                ;;
        esac
        ;;
  *)
        echo "Usage: $SCRIPTNAME {start|stop|status|restart|reload}" >&2
        exit 3
        ;;
esac

:
