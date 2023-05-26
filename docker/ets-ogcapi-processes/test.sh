#!/bin/bash
# run the following sed "s/tb17.geolabs.fr:8120/$(hostname)/g" -i /root/test-run-props.xml
java -jar $(find /root -name "ets-ogcapi-processes10-*aio.jar") -o /tmp /root/test-run-props.xml
cat $(find /tmp -name "*results.xml")
echo Success: $(grep PASS $(find /tmp -name "*results.xml") | wc -l)
echo Failed: $(grep FAIL $(find /tmp -name "*results.xml") | grep -v FAILURE | wc -l)

