# here write the functions 


test() {
	local ip="114.215.195.44"
	local port="8383"
	local logfile="/tmp/test.log"

	echo `date` > $logfile
	wget http://$ip:$port/dl/a.tar.gz -P /tmp/
	echo `date` >> $logfile
}









test
