#! /bin/sh
# xerxes init script
#		Written by Miquel van Smoorenburg <miquels@cistron.nl>.
#		Modified for Debian 
#		by Ian Murdock <imurdock@gnu.ai.mit.edu>.
#		adopted by Maximilian Marx <mmarx@wh2.tu-dresden.de>.
#
# Version:	@(#)xerxes  1.0  27-Aug-2008  mmarx@wh2.tu-dresden.de
#

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/bin/xerxes_runner.rb
NAME=xerxes
DESC=xerxes

test -x $DAEMON || exit 0

# Include xerxes defaults if available
if [ -f /etc/default/xerxes ] ; then
	. /etc/default/xerxes
fi

set -e

case "$1" in
  start)
	echo -n "Starting $DESC: "
	start-stop-daemon --start --quiet --pidfile /var/run/$NAME.pid --chuid xerxes -b --exec $DAEMON -- $DAEMON_OPTS
	echo "$NAME."
	;;
  stop)
	echo -n "Stopping $DESC: "
	start-stop-daemon --stop --quiet --pidfile /var/run/$NAME.pid --exec $DAEMON
	echo "$NAME."
	;;
  force-reload)
	# check wether $DAEMON is running. If so, restart
	start-stop-daemon --stop --test --quiet --pidfile /var/run/$NAME.pid --exec $DAEMON && $0 restart \
	|| exit 0
	;;
  restart)
    echo -n "Restarting $DESC: "
	start-stop-daemon --stop --quiet --pidfile /var/run/$NAME.pid --exec $DAEMON
	sleep 1
	start-stop-daemon --start --quiet --pidfile /var/run/$NAME.pid --chuid xerxes -b --exec $DAEMON -- $DAEMON_OPTS
	echo "$NAME."
	;;
  *)
	N=/etc/init.d/$NAME
	echo "Usage: $N {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
