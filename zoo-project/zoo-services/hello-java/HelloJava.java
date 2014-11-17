/**
 * Author : Gérald FENOY
 *
 *  Copyright 2008-2009 GeoLabs SARL. All rights reserved.
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

import java.lang.*;
import java.util.*;

public class HelloJava {
    public static int HelloWorldJava(HashMap conf,HashMap inputs, HashMap outputs) {
        HashMap tmp=(HashMap)(inputs.get("S"));
        String v=tmp.get("value").toString();
        HashMap hm1 = (HashMap)(outputs.get("Result"));
        hm1.put("value",ZOO._("Hello "+v+" from JAVA World !!"));
        return ZOO.SERVICE_SUCCEEDED;
    }
    public static int JavaLongProcess(HashMap conf,HashMap inputs, HashMap outputs) {
        HashMap tmp=(HashMap)(inputs.get("S"));
        String v=tmp.get("value").toString();
	Integer i;
	for(i=0;i<100;i+=5){
	    ZOO.updateStatus(conf,String.valueOf(i),"Currently executing tasks "+String.valueOf(i/5)+"..");
	    try{
		Thread.sleep(2000);
	    }catch(java.lang.InterruptedException e){
	    }finally{
	    }
	}
        HashMap hm1 = (HashMap)(outputs.get("Result"));
        hm1.put("value",ZOO._("Hello "+v+" from JAVA World !!"));
        return ZOO.SERVICE_SUCCEEDED;
    }
}

