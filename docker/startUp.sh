#!/bin/bash
# Author: Gérald Fenoy
# Copyright GeoLabs 2021

CMD="curl -o toto.out http://rabbitmq:15672"
$CMD
cat toto.out
if [ -e toto.out ]; then echo "Should start" ; else echo wait; sleep 1; $CMD ; fi 

while [ ! -e toto.out ]; do echo wait; sleep 1; $CMD ;  done


echo "START FPM in 5 seconds"

sleep 5

cd /usr/lib/cgi-bin
./zoo_loader_fpm ./main.cfg 2> /var/log/zoofpm.log >> /var/log/zoofpm.log
