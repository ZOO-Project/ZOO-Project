/**
 * Author : Gérald FENOY
 *
 *  Copyright 2014 GeoLabs SARL. All rights reserved.
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

#include "server_internal.h"
#include "response_print.h"
#include "service.h"

extern   int getServiceFromFile(maps*,const char*,service**);

int main(int argc, char** argv){
  service* s1=NULL;
  maps *m=NULL;
  char conf_file[1024];
  snprintf(conf_file,1024,"%s",argv[1]);
  s1=(service*)malloc(SERVICE_SIZE);
  if(s1 == NULL){ 
    return errorException(m, _("Unable to allocate memory."),"InternalError",NULL);
  }
  int t=getServiceFromFile(m,conf_file,&s1);
  if(t>=0)
    dumpServiceAsYAML(s1);
  return 0;
}
