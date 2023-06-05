/*
 * Author : Gérald Fenoy
 *
 *  Copyright 2011-2012 GeoLabs SARL. All rights reserved.
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
#include "service.h"

extern "C" {

#ifdef WIN32
__declspec(dllexport)
#endif
  int Multiply(maps*& conf,maps*& inputs,maps*& outputs){
  	fprintf(stderr,"\nService internal print\n");
  	maps* cursor=inputs;
	int A,B,res;
	A=0;B=0;
	if(cursor!=NULL){
		fprintf(stderr,"\nService internal print\n");
		dumpMaps(cursor);
		maps* tmp=getMaps(inputs,"A");
		if(tmp==NULL)
			return SERVICE_FAILED;
		fprintf(stderr,"\nService internal print\n");
		dumpMap(tmp->content);
		map* tmpv=getMap(tmp->content,"value");
		fprintf(stderr,"\nService internal print\n");
		A=atoi(tmpv->value);
		fprintf(stderr,"\nService internal print (A value: %i)\n",A);
		cursor=cursor->next;
	}
	if(cursor!=NULL){
		maps* tmp=getMaps(cursor,"B");
		map* tmpv=getMap(tmp->content,"value");
		if(tmpv==NULL)
			return SERVICE_FAILED;
		B=atoi(tmpv->value);
		fprintf(stderr,"\nService internal print (B value: %i)\n",B);
	}
	res=A*B;
	char tmp[256];
	sprintf(tmp,"%i",res);
	setMapInMaps(outputs,"Result","value",tmp);
	return SERVICE_SUCCEEDED;
  }

}
