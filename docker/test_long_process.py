#
# Author : Samuel Souk-Aloun
#
# Copyright 2020 GeoLabs SARL. All rights reserved.
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

import argparse
import requests
import time

from threading import Thread
from xml.etree import ElementTree

parser = argparse.ArgumentParser()
parser.add_argument("threads", help="number of threads", type=int)
args = parser.parse_args()


class LongProcess(Thread):
    def __init__(self, name):
        Thread.__init__(self)
        self.name = name
        self.progress = None
        self.location_url = self.launch_long_process()
        print("INIT %s %s" % (self.name, self.location_url))

    def launch_long_process(self):
        endpoint_url = "http://localhost/cgi-bin/zoo_loader.cgi"
        params = dict(
            request="Execute",
            service="WPS",
            version="1.0.0",
            Identifier="longProcess",
            DataInputs="a=toto",
            ResponseDocument="Result",
            storeExecuteResponse="true",
            status="true",
        )
        r = requests.get(endpoint_url, params=params)
        root = ElementTree.fromstring(r.content)

        location_url = root.attrib["statusLocation"]
        return location_url

    def get_progress(self):
        progress = None
        r = requests.get(self.location_url)
        root = ElementTree.fromstring(r.content)
        ns = dict(wps="http://www.opengis.net/wps/1.0.0")
        status = root.find("wps:Status", ns)
        if status:
            ctime = status.attrib["creationTime"]
            started = root.find("wps:Status/wps:ProcessStarted", ns)
            #started = root.find("wps:ProcessStarted", ns)
            succeeded = root.find("wps:Status/wps:ProcessSucceeded", ns)
            #succeeded = root.find("wps:ProcessSucceeded", ns)
            if started is not None and started.attrib["percentCompleted"]:
                percent = started.attrib["percentCompleted"]
                progress = int(percent)
            elif succeeded is not None:
                progress = 100
            else:
                # error?
                print(r.content.decode())
                progress = None
        else:
            print(r.content.decode())
            progress = -1

        self.progress = progress
        return self.progress

    def run(self):
        print("Thread %s: starting" % self.name)
        while True:
            progress = self.get_progress()
            if progress == 100:
                print("Thread %s: succeeded" % self.name)
                break
            elif progress == -1:
                print("Thread %s: failed" % self.name)
                break
            elif progress is None:
                print("Thread %s: error" % self.name)
                break
            else:
                print("Thread %s :progress %s" % (self.name, progress))
            time.sleep(1.0)
        print("Thread %s: finishing" % self.name)


threads = list()
for i in range(0, args.threads):
    t = LongProcess(i)
    t.start()
    threads.append(t)

for t in threads:
    t.join()

