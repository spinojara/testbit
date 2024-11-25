#!/sbin/openrc-run
command_background="yes"
pidfile="/var/run/testbitd.pid"
name="testbitd"
description="testbitd - A test server for bitbit."
command="testbitd"
command_args="--daemon"
output_log="/var/log/testbitd.log"
error_log="/var/log/testbitd.err"
supervisor="supervise-daemon"
retry="SIGINT/5"

depend() {
	need net
}

start_pre() {
	if [[ ! -d "/var/lib/bitbit/patch" ]]; then
		eerror "Please create the patch directory /var/lib/bitbit/patch" 
		return 1
	fi
	if [[ ! -f "/var/lib/bitbit/private/testbit.key.pem" ]]; then
		eerror "Please add a private key at /var/lib/bitbit/private/testbit.key.pem"
		return 2
	fi
	if [[ ! -f "/var/lib/bitbit/certs/testbit.crt" ]]; then
		eerror "Please add a certificate at /var/lib/bitbit/private/testbit.crt"
		return 3
	fi
	return 0
}
