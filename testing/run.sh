#!/bin/bash

Usage=$(cat <<EOF
Please use the following syntaxe:

  ./run.sh <WPSInstance> <ServiceName>

where <WPSInstance> should be the url to a WPS Server and <ServiceName> should 
be the service name you want to run tests with.

For instance to test the Buffer service on a localhost WPS server, use the 
following command:

  ./run.sh http://localhost/cgi-bin/zoo_loader.cgi Buffer

EOF
)

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "$Usage"
    exit
fi

WPSInstance=$1
ServiceName=$2

function kvpRequest {
    echo "Simple KVP request "
    echo "$1"
    curl -o "$2" "$1"
    echo "Checking for ${3} response XML validity..."
    xmllint --noout --schema http://schemas.opengis.net/wps/1.0.0/wps${3}_response.xsd "$2"
}

function postRequest {
    echo "Simple POST request "
    echo "Checking for ${3} request XML validity..."
    xmllint --noout --schema http://schemas.opengis.net/wps/1.0.0/wps${3}_request.xsd "$4" 
    curl -H "Content-type: text/xml" -d@"$4" -o "$2" "$1"
    echo "Checking for ${3} response XML validity..."
    xmllint --noout --schema http://schemas.opengis.net/wps/1.0.0/wps${3}_response.xsd "$2"
    if [ -z "$5" ]; then
	echo ""
    else
	xmllint --noout --schema http://schemas.opengis.net/wps/1.0.0/wps${3}_response.xsd "$(xsltproc ./extractStatusLocation.xsl $2)"
    fi
}


#
# Tests for GetCapabilities using KVP and POST requests
#
kvpRequest "${WPSInstance}?request=GetCapabilities&service=WPS" "tmp/outputGC1.xml" "GetCapabilities"

kvpRequest "${WPSInstance}?REQUEST=GetCapabilities&SERVICE=WPS" "tmp/outputGC2.xml" "GetCapabilities"

echo "Check if differences between upper case and lower case parameter names"
diff -ru tmp/outputGC1.xml tmp/outputGC2.xml 

echo ""

curl -o tmp/10_wpsGetCapabilities_request.xml http://schemas.opengis.net/wps/1.0.0/examples/10_wpsGetCapabilities_request.xml
postRequest "${WPSInstance}" "tmp/outputGCp.xml" "GetCapabilities" "tmp/10_wpsGetCapabilities_request.xml"
echo ""

#
# Tests for DescribeProcess using KVP and POST requests
#
kvpRequest "${WPSInstance}?request=DescribeProcess&service=WPS&version=1.0.0&Identifier=ALL" "tmp/outputDPall.xml" "DescribeProcess"

kvpRequest "${WPSInstance}?request=DescribeProcess&service=WPS&version=1.0.0&Identifier=${ServiceName}" "tmp/outputDPb1.xml" "DescribeProcess"

echo ""

cat requests/dp.xml | sed "s:ServiceName:${ServiceName}:g" > tmp/dp1.xml
postRequest "${WPSInstance}" "tmp/outputDPp.xml" "DescribeProcess" "tmp/dp1.xml"
echo "" 

#
# Tests for Execute using KVP and POST requests
#
for i in ijson_o igml_o ir_o ir_o_async ir_or ir_or_async irb_o irb_o_async irb_or irb_or_async; 
do
    cat requests/${i}.xml | sed "s:ServiceName:${ServiceName}:g" > tmp/${i}1.xml
    if [ -z "$(echo $i | grep async)" ]; then
	postRequest "${WPSInstance}" "tmp/outputE${i}.xml" "Execute" "tmp/${i}1.xml"
    else
	postRequest "${WPSInstance}" "tmp/outputE${i}.xml" "Execute" "tmp/${i}1.xml" "async"
    fi
    echo ""
done

echo ""