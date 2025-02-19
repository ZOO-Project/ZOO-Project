#!/bin/sh
# spawn-fcgi -n -s /var/run/fcgiwrap.socket -M 0660 -u www-data -g www-data /usr/sbin/fcgiwrap &
spawn-fcgi -n -s /var/run/fcgiwrap.socket -M 0660 -u www-data -g www-data -P /var/run/fcgiwrap.pid -F 5 /usr/sbin/fcgiwrap 2>> /var/log/fcgiwrap.log &
nginx -g 'daemon off;'