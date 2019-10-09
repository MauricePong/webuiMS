#!/bin/sh
mount -t nfs -o nolock -o tcp  192.168.1.237:/home/pzw/armwork /mnt 
webfile=/www/configs/web.conf
web_port=`cat $webfile`
httpd -h /www -c /etc/httpd/conf/httpd.conf -p $web_port  
 
