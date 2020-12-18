#!/usr/bin/python3
import os
import sys
import redis
data = sys.stdin.read();

print('Content-Type: text/html')
print('')
print('Environment variables')
for param in os.environ.keys():
        print ("<b>%20s</b>: %s<br/>" % (param, os.environ[param]))

print(data)

from urllib import parse

try:
    params=parse.parse_qs(os.environ["QUERY_STRING"])
    r=None
    if "ZOO_REDIS_HOST" in os.environ:
        r = redis.Redis(host=os.environ["ZOO_REDIS_HOST"], port=6379, db=0)
    else:
        r = redis.Redis(host='redis', port=6379, db=0)
    print(params)
    r.publish(params["jobid"][0],data)
except Exception as e:
	print(e)

