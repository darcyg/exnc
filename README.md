# 基本操作   
        # 执行 a.sh  

        # 进入云平台执行回链  


        # 执行list:  
                2018-08-18T09:55:30.752 [INF] ./exnc::do_cmd_list()  fd:8, ip:122.234.180.177, mac:  

        # 执行 back 或者 back 2323 后面更个端口  

        # 执行 sel 8, 可以切换到指定的设备上，执行shell命令  

        # 执行 exit 退出指定设备或者退出程序  

# 其他操作在/www/main.sh 中完成, main.sh会在设备连接上的瞬间执行，连接上的时候马上push到设备上，然后执行main.sh  


        [dyx@AY140424204831449491Z exnc-master]$ cat ./www/dl/main.sh   
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

