#!/bin/bash

Usage=$(cat <<EOF
Please use the following syntaxe:

  ./run.sh <WPSInstance> <ServiceName> <Request>

where <WPSInstance> should be the url to a WPS Server, <ServiceName> should 
be the service name you want to run tests with (you can use multiple service 
names, use quote and seperate them using space), <Request> should contain the 
requests you want to use (you can use more than one at a time).

For instance to test the Buffer service on a localhost WPS server, use the 
following command:

  ./run.sh http://localhost/cgi-bin/zoo_loader.cgi Buffer "GetCapabilities DescribeProcess Execute"

EOF
)

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "$Usage"
    exit
fi

WPSInstance=$1
ServiceName=$2
NBRequests=1000
NBConcurrents=50
pstat=0

iter=0

function testPostRequests {
#
# Tests for Execute using POST requests
#
    for i in $1; 
    do
	cat requests/${i}.xml | sed "s:ServiceName:${ServiceName}:g;s:InputName:$(cat tmp/inputName.txt):g" > tmp/${i}1.xml
	if [ -z "$(echo $i | grep async)" ]; then
	    postRequest "${WPSInstance}" "tmp/outputE${i}.xml" "Execute" "tmp/${i}1.xml"
	else
	    postRequest "${WPSInstance}" "tmp/outputE${i}.xml" "Execute" "tmp/${i}1.xml" "async"
	fi
	echo ""
    done

}

function plotStat {
    echo " **"
    echo " * Plot statistics ..."
    echo " **"
    cp run.tsv tmp/run$1.tsv
    sed "s:\[image\]:$2:g;s:\[file\]:$3:g" -i tmp/run$1.tsv
    gnuplot tmp/run$1.tsv
    cp run1.tsv tmp/run1$1.tsv
    sed "s:\[image\]:$(echo $2 | sed "s:.jpg:1.jpg:g"):g;s:\[file\]:$3:g" -i tmp/run1$1.tsv
    gnuplot tmp/run1$1.tsv
}


function kvpRequest {
    echo " **"
    echo " <h1> Simple KVP request start on $(date) </h1>"
    echo " <a href='$1'>$1</a>"
    
    RESP=$(curl -v -o "$2" "$1" 2> tmp/temp.log; grep "< HTTP" tmp/temp.log | cut -d' ' -f3)
    if [ "${3}" == "owsExceptionReport" ]; then
	echo " *********************************"
	if [ "$RESP" ==	"200" ]; then
	    echo " ! Invalid response code ($RESP)"
	else
	    echo " * Valid response code ($RESP)"
	fi
	echo " *********************************"
	echo " * Checking for ${3} response XML validity..."
    	echo " * Schema: [http://schemas.opengis.net/ows/1.1.0/${3}.xsd]"
	echo " *********************************"
	xmllint --noout --schema http://schemas.opengis.net/ows/1.1.0/${3}.xsd "$2" 2> tmp/res${iter}.txt
	echo " *********************************"
	echo "Verifying that the missing / wrong argument was referenced in locator and the exceptionCode take the corresponding value ..."
	echo -n " *********************************"
	xsltproc ./extractExceptionInfo.xsl "$2"
	echo " *********************************"
    else
	if [ "$RESP" ==	"200" ]; then
	    echo " * Valid response code ($RESP)"
	else
	    echo " ! Invalid response code ($RESP)"
	fi
	echo " * Checking for ${3} response XML validity..."
	echo " * Schema: [http://schemas.opengis.net/wps/1.0.0/wps${3}_response.xsd]"
	xmllint --noout --schema http://schemas.opengis.net/wps/1.0.0/wps${3}_response.xsd "$2" 2> tmp/res${iter}.txt
	v="$(cat tmp/res${iter}.txt | grep validates)"
	echo ""
    fi
    echo " **"
    echo " * Sending $NBRequests ${3} requests starting on $(date) ..."
    echo " **"
    ab -g tmp/stat${3}${iter}.plot -e tmp/stat${3}${iter}.txt -n "$NBRequests" -c "$NBConcurrents" "$1"
    if [ "$pstat" -eq 1 ]; then
	plotStat ${iter} tmp/stat${3}${iter}.jpg tmp/stat${3}${iter}.plot
    fi
    iter=$(expr $iter + 1)
    echo " ** Ending on $(date)"
}

function postRequest {
    echo " **"
    echo " * Simple POST request started on $(date)"
    echo " **"
    echo " * Checking for ${3} request XML validity..."
    echo " * Schema: http://schemas.opengis.net/wps/1.0.0/wps${3}_request.xsd"
    echo " *********************************"
    xmllint --noout --schema http://schemas.opengis.net/wps/1.0.0/wps${3}_request.xsd "$4" 
    echo " *********************************"
    curl -H "Content-type: text/xml" -d@"$4" -o "$2" "$1"
    echo " * Checking for ${3} response XML validity on $(date) ..."
    echo " * Schema: http://schemas.opengis.net/wps/1.0.0/wps${3}_response.xsd"
    echo " *********************************"
    xmllint --noout --schema http://schemas.opengis.net/wps/1.0.0/wps${3}_response.xsd "$2"
    echo " *********************************"
    if [ -z "$5" ]; then
	echo ""
    else
	echo " * Schema: http://schemas.opengis.net/wps/1.0.0/wps${3}_response.xsd"
	echo " *********************************"
	xmllint --noout --schema http://schemas.opengis.net/wps/1.0.0/wps${3}_response.xsd "$(xsltproc ./extractStatusLocation.xsl $2)"
	echo " *********************************"
    fi
    echo " * Sending $NBRequests ${3} XML requests on $(date) ..."
    ab -g tmp/stat${3}${iter}.plot -e tmp/stat${3}${iter}.txt -T "text/xml" -p "$4" -n "$NBRequests" -c "$NBConcurrents" "$1"
    if [ "$pstat" -eq 1 ]; then
	plotStat ${iter} tmp/stat${3}${iter}.jpg tmp/stat${3}${iter}.plot
    fi
    iter=$(expr $iter + 1)
    echo " ** Ending on $(date)"
}

function kvpRequestWrite {
    suffix=""
    cnt=0
    cnt0=0
    for i in $2; do
	if [ ! $1 -eq $cnt0 ]; then
	    if [ $cnt -gt 0 ]; then
		suffix="$suffix&$(echo $i | sed 's:\"::g')"
	    else
		suffix="$(echo $i | sed 's:\"::g')"
	    fi
	    cnt=$(expr $cnt + 1)
	fi
	cnt0=$(expr $cnt0 + 1)
    done
    echo $suffix
}

function kvpWrongRequestWrite {
    suffix=""
    cnt=0
    cnt0=0
    for i in $2; do
	if [ ! $1 -eq $cnt0 ]; then
	    if [ $cnt -gt 0 ]; then
		suffix="$suffix&$(echo $i | sed 's:\"::g')"
	    else
		suffix="$(echo $i | sed 's:\"::g')"
	    fi
	    cnt=$(expr $cnt + 1)
	else
	    cnt1=0
	    for j in $3; do 
		if [ $cnt1 -eq $1 ]; then
		    suffix="$suffix&$(echo $j | sed 's:\"::g')"
		fi
		cnt1=$(expr $cnt1 + 1)
	    done
	fi
	cnt0=$(expr $cnt0 + 1)
    done
    echo $suffix
}

for i in $3; do
    if [ "$i" == "GetCapabilities" ]; then
#
# Tests for GetCapabilities using KVP (including wrong requests) and POST requests
#
	kvpRequest "${WPSInstance}?REQUEST=GetCapabilities&SERVICE=WPS" "tmp/outputGC1.xml" "GetCapabilities"

	params='"request=GetCapabilities" "service=WPS"'

	suffix=$(kvpRequestWrite -1 "$params")
	kvpRequest "${WPSInstance}?$suffix" "tmp/outputGC2.xml" "GetCapabilities"

	for j in 0 1; do 
	    suffix=$(kvpRequestWrite $j "$params")
	    kvpRequest "${WPSInstance}?$suffix" "tmp/outputGC$(expr $j + 3).xml" "owsExceptionReport"
	done

	paramsw='"request=GetCapabilitie" "service=WXS"'
	for j in 0 1; do 
	    suffix=$(kvpWrongRequestWrite $j "$params" "$paramsw")
	    kvpRequest "${WPSInstance}?$suffix" "tmp/outputGC$(expr $j + 5).xml" "owsExceptionReport"
	done

	echo "Check if differences between upper case and lower case parameter names"
	diff -ru tmp/outputGC1.xml tmp/outputGC2.xml 

	echo ""

	curl -o tmp/10_wpsGetCapabilities_request.xml http://schemas.opengis.net/wps/1.0.0/examples/10_wpsGetCapabilities_request.xml
	postRequest "${WPSInstance}" "tmp/outputGCp.xml" "GetCapabilities" "tmp/10_wpsGetCapabilities_request.xml"
	echo ""
    fi
    if [ "$i" == "DescribeProcess" ]; then
#
# Tests for DescribeProcess using KVP and POST requests
#
	kvpRequest "${WPSInstance}?request=DescribeProcess&service=WPS&version=1.0.0&Identifier=ALL" "tmp/outputDPall.xml" "DescribeProcess"

	params='"request=DescribeProcess" "service=WPS" "version=1.0.0" "Identifier='${ServiceName}'"'

	suffix=$(kvpRequestWrite -1 "$params")
	kvpRequest "${WPSInstance}?$suffix" "tmp/outputDPb1.xml" "DescribeProcess"
	
	for j in 0 1 2 3; do 
	    suffix=$(kvpRequestWrite $j "$params")
	    kvpRequest "${WPSInstance}?$suffix" "tmp/outputDPb$(expr $j + 2).xml" "owsExceptionReport"
	done

	paramsw='"request=DescribeProces" "service=WXS" "version=1.2.0" "Identifier=Undefined"'
	
	for j in 0 1 2 3; do 
	    suffix=$(kvpWrongRequestWrite $j "$params")
	    kvpRequest "${WPSInstance}?$suffix" "tmp/outputDPb$(expr $j + 6).xml" "owsExceptionReport"
	done


	cat requests/dp.xml | sed "s:ServiceName:${ServiceName}:g" > tmp/dp1.xml
	postRequest "${WPSInstance}" "tmp/outputDPp.xml" "DescribeProcess" "tmp/dp1.xml"
	xsltproc extractInputs.xsl tmp/outputDPp.xml > tmp/inputName.txt
	echo "" 
    fi
    if [ "$i" == "ExecuteSync" ]; then
	testPostRequests "ijson_o igml_o ir_o ir_or irb_o irb_or"
	echo "" 
    fi
    if [ "$i" == "ExecuteAsync" ]; then	
	testPostRequests "ir_o_async ir_or_async irb_o_async irb_or_async"
	echo "" 
    fi
done
