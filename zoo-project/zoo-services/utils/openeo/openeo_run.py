# -*- coding: utf-8 -*-
###############################################################################
#  Author:   Gérald Fenoy, gerald.fenoy@geolabs.fr
#  Copyright (c) 2023, GeoLabs SARL. 
############################################################################### 
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
################################################################################
from openeo_pg_parser_networkx import OpenEOProcessGraph
import sys
import os
original=os.dup(1)
os.close(1)

#print(inputs,file=sys.stderr)
#parsed_graph = OpenEOProcessGraph.from_file(inputs["OpenEOGraph"]["cache_file"])
print(sys.argv,file=sys.stderr)
parsed_graph = OpenEOProcessGraph.from_file(sys.argv[2])
import importlib
import inspect
from openeo_pg_parser_networkx import ProcessRegistry
from openeo_processes_dask.process_implementations.core import process
from openeo_pg_parser_networkx.process_registry import Process

process_registry = ProcessRegistry(wrap_funcs=[process])

# Import these pre-defined processes from openeo_processes_dask and register them into registry
processes_from_module = [
    func
    for _, func in inspect.getmembers(
        importlib.import_module("openeo_processes_dask.process_implementations"),
        inspect.isfunction,
        )
]

specs_module = importlib.import_module("openeo_processes_dask.specs")
specs = {
    func.__name__: getattr(specs_module, func.__name__)
    for func in processes_from_module
}

for func in processes_from_module:
    process_registry[func.__name__] = Process(
        spec=specs[func.__name__], implementation=func
        )
    
pg_callable = parsed_graph.to_callable(process_registry=process_registry)

#print(pg_callable(named_parameters={"f":90.0}))
os.dup2(original,1)
os.close(original)
#print(pg_callable(named_parameters=eval(sys.argv[2])))
f=open(sys.argv[1],"w")
f.write(str(pg_callable(named_parameters=eval(sys.argv[3]))))
f.close()
