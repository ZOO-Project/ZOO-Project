#!/bin/bash

rm -f log log1

./zoo_loader.cgi "request=Execute&service=WPS&version=1.0.0&Identifier=longProcess&DataInputs=&storeExecuteResponse=true&status=true" > log

if [ -z "$(grep "ows:ExceptionReport" log)" ]; then
    while [ -z "$(grep "wps:ProcessSucceeded" log1)" ]; 
    do 
	./zoo_loader.cgi $(grep statusLocation= ./log | cut -d'?' -f2 | cut -d'"' -f1 | sed "s:amp;::g") > log1 ;
	cat log1 ; 
    done
    cat log1
else
    echo "Service failed, please make sure that your main.cfg file contains"
    echo "in the [main] section valid values for both tmpPath and dataPath."
    echo 
    cat log
fi
