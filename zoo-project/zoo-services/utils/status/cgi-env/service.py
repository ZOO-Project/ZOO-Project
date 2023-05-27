#
# Author : Gérald FENOY
#
# Copyright 2013-2015 GeoLabs SARL. All rights reserved.
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

def demo(conf,inputs,outputs):
    import zoo,time
    i=0
    while i < 100:
        conf["lenv"]["message"]="Step "+str(i)
        zoo.update_status(conf,i)
        time.sleep(0.5)
        i+=1
    conf["lenv"]["message"]=zoo._("Error executing the service")
    return zoo.SERVICE_FAILED

def demo1(conf,inputs,outputs):
    conf["lenv"]["message"]=zoo._("Error executing the service")
    return zoo.SERVICE_FAILED
