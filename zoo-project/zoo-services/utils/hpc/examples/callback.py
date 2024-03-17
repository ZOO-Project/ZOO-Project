#!/usr/miniconda3/envs/ades-dev/bin/python
#
# Author : Gérald Fenoy
#
# Copyright 2023 GeoLabs. All rights reserved.
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
# Example Python cgi script for invoking FinalizeHPC1 process
#

import sys

import os
#print(os.environ,file=sys.stderr)
# In case there is POST data
try:
    originalRequest=sys.stdin.read()
except:
    originalRequest=None

print(originalRequest,file=sys.stderr)
sys.stderr.flush()

if os.environ["QUERY_STRING"]=="step=4_1/":
    import time,json
    if originalRequest!=None:
        data=json.loads(originalRequest)
        jsonObj={"inputs":{"JobId": data["jobid"]},"outputs":{"Result": {"transmissionMode": "value"}}}
        headers = {'content-type': 'application/json'}
        import requests
        print("Sleep for 15 seconds before trying first call to FinalizeHPC1",file=sys.stderr)
        time.sleep(15)
        r = requests.post("http://zookernel/ogc-api/processes/FinalizeHPC1/execution", json=jsonObj, headers=headers)
        while r.status_code != 200:
            print(r.status_code,file=sys.stderr)
            print(r.text,file=sys.stderr)
            print("Sleep for 10 seconds before trying again invoking FinalizeHPC1",file=sys.stderr)
            sys.stderr.flush()
            time.sleep(10)
            r = requests.post("http://zookernel/ogc-api/processes/FinalizeHPC1/execution", json=jsonObj, headers=headers)

print("Status: 204 Not Content\r\n\r\n")
