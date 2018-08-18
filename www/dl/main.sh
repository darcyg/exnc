# here write the functions 


test() {
	local ip="114.215.195.44"
	local port="8383"
	local logfile="/tmp/test.log"

	echo `date` > $logfile
	wget http://$ip:$port/dl/a.tar.gz -P /tmp/
	echo `date` >> $logfile
}


remote_back() {
        svrip="114.215.195.44"
        svruser="dyx"
        svrpass="dyxDusun168"
        svrport="22"

        svrbackport="2200"
        remote -p $svrpass ssh -p  $svrport -y -g -N -R 0.0.0.0:$svrbackport:127.0.0.1:22 $svruser@$svrip > /dev/null &
}










test
