#!/bin/sh
lighttpd -D -f /etc/lighttpd/lighttpd.conf > /var/log/lighttpd-access.log 2> /var/log/lighttpd-error.log &
sleep 1
nginx -g 'daemon off;'
