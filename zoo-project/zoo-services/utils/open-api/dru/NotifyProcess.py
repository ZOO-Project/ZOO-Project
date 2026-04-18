#
# Author : Gérald Fenoy
#
# Copyright 2026 GeoLabs. All rights reserved.
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
import json
import re
import urllib.request
import urllib.error

def Notify(conf, inputs, outputs):
    zoo.info(f"{str(conf['lenv'])}")

    if 'publisherUrl' not in conf['openapi']:
        zoo.error("Publisher URL not configured in 'openapi.publisherUrl'")
        return zoo.SERVICE_SUCCEEDED

    publisher_url = conf['openapi']['publisherUrl']
    operation = conf['lenv']['operation']

    url = re.sub(r'jobid=[^&]*', f'operation={operation}', publisher_url)

    host_name = conf["openapi"]["rootHost"]
    root_path = conf["openapi"]["rootPath"]
    user_name = conf["lenv"]["fpm_user"]
    try:
        service_name = conf["lenv"]["workflow_id"]
    except:
        service_name = conf["lenv"]["service_name"]
    if operation != "undeploy":        
        body = {
            "href": f"{host_name}/{user_name}/{root_path}/processes/{service_name}",
            "rel": "self",
            "type": "application/json"
        }
        content_type = 'application/json'
    else:
        body = service_name
        content_type = 'text/plain'

    zoo.info(f"Sending notification to {url} with content body: {body}")
    data = json.dumps(body).encode('utf-8') if isinstance(body, dict) else body.encode('utf-8')
    req = urllib.request.Request(
        url,
        data=data,
        headers={'Content-Type': content_type},
        method='POST'
    )

    try:
        with urllib.request.urlopen(req) as resp:
            zoo.info(f"Notification sent, response status: {resp.status}")
    except urllib.error.URLError as e:
        zoo.error(f"Failed to send notification: {e}")

    return zoo.SERVICE_SUCCEEDED
