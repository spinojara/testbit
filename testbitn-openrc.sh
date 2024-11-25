#!/sbin/openrc-run
command_background="yes"
pidfile="/var/run/testbitn.pid"
name="testbitn"
description="testbitn - A test server for bitbit."
command="testbitn"
command_args="--daemon --stdin /var/lib/bitbit/private/passphrase"
output_log="/var/log/testbitn.log"
error_log="/var/log/testbitn.err"
supervisor="supervise-daemon"
retry="SIGINT/5"

depend() {
	need net
}

start_pre() {
	if [[ ! -f "/var/lib/bitbit/private/passphrase" ]]; then
		eerror "No passphrase found at /var/lib/bitbit/private/passphrase"
		return 1
	fi

	if [[ ! -f "/etc/testbit.conf" ]]; then
		eerror "Please create the configuration file /etc/testbit.conf"
		return 2
	fi
	return 0
}
