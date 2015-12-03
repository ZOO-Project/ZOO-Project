# -*- coding: utf-8 -*-
#
# Author : Vlad Merticariu <merticariu@rasdaman.com>
#
# Copyright 2015 rasdaman GmbH. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including with
# out limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
import zoo
import requests

WCPS_REQUEST_TEMPLATE = "%endPoint%?service=WCS&version=2.0.1&request=ProcessCoverages&query=%query%"

def WCPS(conf,inputs,outputs):
	end_point = validate_end_point(inputs["endPoint"]["value"])
        query = get_correct_query(inputs["query"]["value"],outputs["Response"]["mimeType"])
        response_data_type = get_data_type(query)
        status, response = execute_wcps_request(end_point, query)
	import sys    
	print >> sys.stderr,query
	#handle errors
	if status:
		conf["lenv"]["message"] = zoo._("The end point at " + end_point + " could not be reached.")
		return zoo.SERVICE_FAILED
 	if "ows:ExceptionReport" in response:
		conf["lenv"]["message"] = parse_error_message(response)
		return zoo.SERVICE_FAILED
		
	#no errors, everything ok 	
        outputs["Response"]["value"] = remove_trailing_characters(response, response_data_type)
        #outputs["Response"]["mimeType"] = response_data_type
        #outputs["Response"]["encoding"] = get_encoding(outputs["Response"]["mimeType"])
	return zoo.SERVICE_SUCCEEDED

def parse_error_message(response):
	return response.split("<ows:ExceptionText>")[1].split("</ows:ExceptionText>")[0]

def validate_end_point(end_point):
	ret = end_point
	if not end_point.startswith("http"):
        	ret = "http://" + end_point
	return ret

def get_data_type(query):
	ret = "text/plain"
	if "\"png\"" in query:
		ret = "image/png"
	if "\"csv\"" in query:
		ret = "text/csv"
	if "\"tiff\"" in query:
		ret = "image/tiff"
	if "\"jpeg\"" in query or "\"jpg\"" in query:
		ret = "image/jpeg"
	return ret

def get_correct_query(query,mimeType):
    return query.replace("[format]",get_data_ext(mimeType))

def get_data_ext(mimeType):
	ret = "csv"
	for i in ["png","csv","tiff","jpeg"]:
		if i in mimeType:
			ret = i
			break
	return ret

def get_encoding(data_type):
	ret = "utf-8"
	if data_type.startswith("image"):
		ret = "base64"
	return ret

def remove_trailing_characters(response, data_type):
        response_start = 27
        if data_type.startswith("image"):
		response_start += len(data_type)
	else:
		response_start += 10
        ret = response[response_start:-17]
	return ret

def execute_wcps_request(end_point, query):
        composed_url = WCPS_REQUEST_TEMPLATE.replace("%endPoint%", end_point).replace("%query%", query) 
	try:
		response = requests.get(composed_url)
	except:
		#can not open url
		return 1, ""
	#all ok
	return 0, response.content

    
