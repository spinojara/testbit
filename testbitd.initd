#!/sbin/openrc-run
command_background="yes"
pidfile="/var/run/testbitd.pid"
name="testbitd"
description="testbitd - A test server for bitbit."
command="testbitd"
command_args="--db /var/lib/bitbit/bitbit.sqlite3 \
	--db-backup /var/lib/bitbit/backup"
output_log="/var/log/testbitd.log"
error_log="/var/log/testbitd.err"
supervisor="supervise-daemon"
retry="SIGINT/5"

depend() {
	need net docker
}

start_pre() {
	if [[ ! -d "/var/lib/bitbit" ]]; then
		eerror "Please create the directory /var/lib/bitbit"
		return 1
	fi
	if [[ ! -d "/var/lib/bitbit/backup" ]]; then
		eerror "Please create the directory /var/lib/bitbit/backup"
		return 1
	fi
	return 0
}
