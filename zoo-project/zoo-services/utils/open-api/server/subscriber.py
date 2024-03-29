#!/usr/bin/python3
#
# cf. https://github.com/joewalnes/websocketd/wiki/Simple-Python-Duplex-Example
#
# Author: Gérald Fenoy
#
# Copyright (c) 2020-2023 GeoLabs SARL
#
# Use of this source code is governed by a BSD-style
# license that can be found in the https://github.com/joewalnes/websocketd/blob/master/LICENSE file.
#
# example usage:
# websocketd --port=4430 --ssl --sslcert /ssl/fullchain.pem --sslkey /ssl/privkey.pem subscriber.py --devconsole
#

from sys import stdout, stdin
import sys
import threading
import redis
import json
import os

mThreads=[]
r=None

if "ZOO_REDIS_HOST" in os.environ:
    r = redis.Redis(host=os.environ["ZOO_REDIS_HOST"], port=6379, db=0)
else:
    r = redis.Redis(host='redis', port=6379, db=0)

def send(t):
    # send string to web page
    stdout.write(t+'\n')
    stdout.flush()

def listenMessages(jobID=None):
    global r
    p = r.pubsub()
    p.subscribe(jobID)
    hasSend=False
    for raw_message in p.listen():
        try:
            send(str(raw_message["data"],'utf-8'))
            hasSend=True
            try:
                tmp=json.loads(str(raw_message["data"],'utf-8'))
                if tmp is not None and "outputs" in tmp:
                    sys.exit()
            except Exception as e:
                print(str(e))
                return
        except:
            if not(hasSend):
                send(str(raw_message["data"]))


def receive():
    global n
    global mThreads
    while True:
        t = stdin.readline().strip()
        if not t:
            break
        t1 = t.split(" ")
        if t1[0]=="SUB":
            mThreads += [threading.Thread(target=listenMessages,kwargs={"jobID":t1[1]})]
            mThreads[len(mThreads)-1].start()
        else:
            send(t)

t0 = threading.Thread(target=receive)
t0.start()

t0.join()

