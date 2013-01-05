#! /bin/bash
#
# domoticz.sh	init script for domoticz
#
#		Modified for Debian GNU/Linux

### BEGIN INIT INFO
# Provides:          domoticz 
# Required-Start:    $network $remote_fs $syslog
# Required-Stop:     $network $remote_fs $syslog
# Should-Start:
# Should-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Home Automation System
# Description:       This daemon will start the Domoticz Home Automation System
### END INIT INFO

set -e

if [ -r "/lib/lsb/init-functions" ]; then
  . /lib/lsb/init-functions
else
  echo "E: /lib/lsb/init-functions not found, lsb-base (>= 3.0-6) needed"
  exit 1
fi

LANG=C
export LANG

PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/home/pi/domoticz/domoticz
NAME=domoticz
DESC="Domoticz Home Automation System"
PIDFILE=/var/run/$NAME.pid
[ "$LOGFILE" = "" ] && LOGFILE="/tmp/domoticz.log"

test -f $DAEMON || exit 0


# Defaults
OPTIONS="-www 8080"

# Read config file (will override defaults above)
[ -r /etc/default/ser2net ] && . /etc/default/ser2net

# this is from madduck on IRC, 2006-07-06
# There should be a better possibility to give daemon error messages
# and/or to log things
log()
{
  case "$1" in
    [[:digit:]]*) success=$1; shift;;
    *) :;;
  esac
  log_action_begin_msg "$1"; shift
  log_action_end_msg ${success:-0} "$*"
}

start () {
  if ! pidofproc -p "$PIDFILE" >/dev/null; then
      # since we dont have a proper daemon, lets pretend...
      $DAEMON $OPTIONS &> $LOGFILE &
      pid=$!
      ret=$?
      echo $pid > $PIDFILE
      [ $ret = 0 ] && log_daemon_msg "Running - PID: $pid" "Domoticz"
      [ $ret != 0 ] && log_daemon_msg "Error - exit status: $ret" "Domoticz"
      log_end_msg $ret
  else
    log_failure_msg "already running!"
    log_end_msg 1
    exit 1
  fi
  return $ret
}

stop () {
  # this is a workaround as Domoticz does not delete its pidfile
  SIG="${1:--TERM}"
  killproc -p "$PIDFILE" "$DAEMON" "$SIG"
  # this is a workaround for killproc -TERM not zapping the pidfile
  if ! pidofproc -p "$PIDFILE" >/dev/null; then
    rm -f $PIDFILE
  fi
}

case "$1" in
  start)
	log_daemon_msg "Starting $DESC"
	start
	log_end_msg 0
	;;
  stop)
	log_daemon_msg "Stopping $DESC"
	stop
	log_end_msg 0
	;;
  reload|force-reload)
	log_daemon_msg "Reloading $DESC"
	stop "-HUP"
	log_end_msg 0
	;;
  restart)
	log_daemon_msg "Restarting $DESC"
	stop
	sleep 1
	start
	log_end_msg 0
	;;
  status)
  	status_of_proc $DAEMON $NAME 
	;;
  *)
	N=/etc/init.d/$NAME
	echo "Usage: $N {start|stop|restart|reload|force-reload|status}" >&2
	exit 1
	;;
esac

exit 0
