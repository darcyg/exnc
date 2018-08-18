# here write the functions 


test() {
	local logfile="/tmp/test.log"

	echo `date` > $logfile
	wget http://192.168.10.6:18080/dl/a.tar.gz -P /tmp/
	echo `date` >> $logfile
}









test
