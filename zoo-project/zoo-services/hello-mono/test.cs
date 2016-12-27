/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2016 GeoLabs SARL
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
using System;
using ZooGenerics;
using System.Threading;

namespace Default
{
    public class Service{
	public static int HelloMono(ZMaps conf,ZMaps inputs,ZMaps outputs){
	    _ZMaps test;
	    if(inputs.TryGetValue("a", out test)){
		ZMap content=test.getContent();
		String test1;
		if(content.TryGetValue("value", out test1)){
		    outputs.setMapsInMaps("Result","value",ZOO_API.Translate("Hello ")+test1+" from the Mono .NET framework World!");
		}
		return ZOO_API.SERVICE_SUCCEEDED;
	    }else{
		conf.setMapsInMaps("lenv","message","Unable to run the service");
		return ZOO_API.SERVICE_FAILED;
	    }
	}
	public static int longProcessMono(ZMaps conf,ZMaps inputs,ZMaps outputs){
	    _ZMaps test;
	    int i=1;
	    while(i<10){
		ZOO_API.UpdateStatus(conf,"Step "+i,(i*10));
		Thread.Sleep(1000);
		i+=1;
	    }
	    if(inputs.TryGetValue("a", out test)){
		ZMap content=test.getContent();
		String test1;
		if(content.TryGetValue("value", out test1)){
		    outputs.setMapsInMaps("Result","value",ZOO_API.Translate("Hello ")+test1+" from the Mono .NET framework World!");
		}
		return ZOO_API.SERVICE_SUCCEEDED;
	    }else{
		conf.setMapsInMaps("lenv","message","Unable to run the service");
		return ZOO_API.SERVICE_FAILED;
	    }
	}
    };
}
