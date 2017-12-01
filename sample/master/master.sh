#!/bin/sh

ulimit -n 102400

___INFO()
{
	echo $@
}
INFO=___INFO

subcmd=$1
work_path="./"
master_cmd="./master"
config_dir="./etc/service/"
pid_file="./master.pid"

cd $work_path || {
	$INFO no such work_path \"$work_path\" !
	exit 1
}
umask 002
touch $pid_file

case $subcmd in
	start)
		$master_cmd -pid-file $pid_file --try-lock 2>/dev/null || {
			$INFO the system is already running
			exit 1
		}
		$INFO starting the system
		$master_cmd -C $config_dir -pid-file $pid_file -log-service log.socket,./log_dir/,hour -server-log masterlog,master,master &
		;;

	stop)
		$master_cmd -pid-file $pid_file --try-lock 2>/dev/null && {
			$INFO  the system is not running
			exit 1
		}
		$INFO stop the system
		kill `head -n 1 $pid_file`
		for i in 5 4 3 2 1
		do
			$master_cmd -pid-file $pid_file --try-lock && exit 0
			$INFO waiting "for" the system to terminate
			sleep 1
		done
		;;

	reload)
		$master_cmd -pid-file $pid_file --try-lock 2>/dev/null && {
			$INFO  the system is not running
			exit 1
		}
		$INFO refreshing the system
		kill -HUP `head -n 1 $pid_file`
		;;

	restart)
        sh $0 "stop"
        sh $0 start
        ;;
	*)
		$INFO unknown command: $subcmd
		$INFO USAGE: $0 start "("or stop, reload, restart")"
		;;

esac
