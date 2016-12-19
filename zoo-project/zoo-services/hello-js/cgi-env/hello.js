/**
 * Author : Gérald FENOY
 *
 * Copyright 2009-2013 GeoLabs SARL. All rights reserved.
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

function hellojs(conf,inputs,outputs){
	outputs["result"]["value"]="Hello "+inputs["S"]["value"]+" from the JS World !";
	//SERVICE_SUCEEDED
	return Array(3,outputs);
}

function hellojs2(conf,inputs,outputs){
    outputs["result"]["value"]="Hello "+inputs["S"]["child"]["nom"]["value"]+" "+inputs["S"]["child"]["prenom"]["value"]+" from the JS World !";
    outputs["result1"]["child"]["tata"]["value"]="a"
    //SERVICE_SUCEEDED
    return {"result": 3,"outputs":outputs};
}

function hellojs1(conf,inputs,outputs){
	outputs["result"]["value"]="Hello "+inputs["S"]["value"]+" from the JS World !";
	//SERVICE_SUCEEDED
	return {"result":3,"outputs": outputs};
}

