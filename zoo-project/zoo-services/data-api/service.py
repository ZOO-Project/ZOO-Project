#!/usr/bin/python

import zoo

import logging
import logging.config
import sys
import errno
import os
import json
import geojson
import csv
import argparse

from publicamundi.data.api import *

ERROR_OK = 0
ERROR_UNKNOWN = 1

def configure_logging(filename):
    if filename is None:
        print 'Logging is not configured.'
    else:
        logging.config.fileConfig(filename)

def parse_query(filename, text):
    if not filename is None and os.path.isfile(filename):
        with open(filename) as query_file:    
            return json.load(query_file, cls=ShapelyJsonDecoder, encoding='utf-8')
    if not text is None:
        return json.loads(text, cls=ShapelyJsonDecoder, encoding='utf-8')
        
    return {}

def query(conf,inputs,outputs):    
    config = {
        CONFIG_SQL_CATALOG : conf["data-api"]["catalog"],
        CONFIG_SQL_DATA : conf["data-api"]["vectorstore"],
        CONFIG_SQL_TIMEOUT : int(conf["data-api"]["timeout"]) * 1000
    }

    if inputs.keys().count("timeout")>0:
        config["CONFIG_SQL_TIMEOUT"]=int(inputs["timeout"]["value"]) * 1000
    
    metadata = {}

    configure_logging(conf["lenv"]["cwd"]+"/"+conf["lenv"]["metapath"]+"/service.ini")
    
    query_executor = QueryExecutor()
    result = query_executor.execute(config, parse_query(None,inputs["query"]["value"]) )
    
    outputs["Result"]["value"]=geojson.dumps(result['data'][0], cls=ShapelyGeoJsonEncoder, encoding='utf-8')
    return zoo.SERVICE_SUCCEEDED
        
