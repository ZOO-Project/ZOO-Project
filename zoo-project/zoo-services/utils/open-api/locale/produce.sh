#!/bin/bash

for j in cgi-env/*zcfg ;
  do
    for i in Title Abstract;
     do
      grep $i $j | sed "s:$i = :_ss(\":g;s:\r:\"):g" ;
     done;
 done > service.c

grep -A 10 links_title ../../../../docker/oas.cfg | grep = | cut -d'=' -f2 | sed 's:V:_ss("V:g;s:$:"):g'  >> service.c 

xgettext --package-name="ZOO-Service OGC API - Processes services" \
	--package-version=1.9.0 \
	--copyright-holder="GeoLabs" \
	service.c templates/index.html -k_ss -k_ -o message.po

sed "s:SOME DESCRIPTIVE TITLE.:ZOO-Service OGC API - Processes services:g" message.po > locale/po/messages.po
rm message.po
rm service.c
