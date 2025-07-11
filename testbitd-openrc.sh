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
	if [[ ! -d "/var/lib/bitbit/nnue" ]]; then
		eerror "Please create the nnue directory /var/lib/bitbit/nnue"
		return 1
	fi
	if [[ ! -f "/etc/letsencrypt/live/jalagaoi.se/privkey.pem" ]]; then
		eerror "Please add a private key at /etc/letsencrypt/live/jalagaoi.se/privkey.pem"
		return 2
	fi
	if [[ ! -f "/etc/letsencrypt/live/jalagaoi.se/fullchain.pem" ]]; then
		eerror "Please add a certificate at /etc/letsencrypt/live/jalagaoi.se/fullchain.pem"
		return 3
	fi
	return 0
}
