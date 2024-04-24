/*
 * Author : Gérald FENOY
 *
 *  Copyright 2024 GeoLabs SARL
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

#include "cpl_string.h"
#include "service_internal_gdal.h"
#include "service.h"

/**
 * Convert input and output maps to arguments for 
 * the GDAL Application to execute as a service
 * 
 * @param scServiceName The name of the service
 * @param argc The number of arguments
 * @param argv The arguments
 * @param pmsConf The configuration maps
 * @param pmsInputs The input maps
 * @param pmsOutputs The output maps
 * @return void
 */
void GdalZOOServiceInit(const char* scServiceName,int* iArgc,char*** pccArgv,maps* pmsConf,maps* pmsInputs,maps* pmsOutputs){
    char** argv=*pccArgv;
    argv = CSLAddString(argv, scServiceName);
    int argc=*iArgc;
    argc++;
    maps* pmsCursor=pmsInputs;
    while(pmsCursor!=NULL){
        map* pmInRequest=getMap(pmsCursor->content,"inRequest");
        map* pmLength=getMap(pmsCursor->content,"length");
        int iLen=1;
        if(pmLength!=NULL){
            iLen=atoi(pmLength->value);
        }
        for(int iCnt=0;iCnt<iLen;iCnt++){
            map* pmCursor=getMapArray(pmsCursor->content,"value",iCnt);
            dumpMap(pmsCursor->content);
            if(pmCursor!=NULL && pmInRequest!=NULL && strncasecmp(pmInRequest->value,"true",4)==0){
                if(strncasecmp(pmsCursor->name,"Input",5)==0 ||
                   strncasecmp(pmsCursor->name,"Output",6)==0){
                    char* pcaTmp=(char*) malloc((strlen(pmCursor->value)+3)*sizeof(char));
                    sprintf(pcaTmp,"%s",pmCursor->value);
                    argv = CSLAddString(argv, pcaTmp);
                    argc++;
                    free(pcaTmp);
                }
                else if(strncasecmp(pmCursor->value,"NULL",4)!=0){
                    if(strncasecmp(pmCursor->value,"true",4)==0 ||
                       strncasecmp(pmCursor->value,"false",5)==0){
                        if(strncasecmp(pmCursor->value,"true",4)==0){
                            char* pcaTmp=(char*) malloc((strlen(pmCursor->name)+2)*sizeof(char));
                            sprintf(pcaTmp,"-%s",pmsCursor->name);
                            argv = CSLAddString(argv, pcaTmp);
                            argc++;
                            free(pcaTmp);
                        }
                    }else{
                        char* pcaTmp=(char*) malloc((strlen(pmsCursor->name)+3)*sizeof(char));
                        sprintf(pcaTmp,"-%s",pmsCursor->name);
                        argv = CSLAddString(argv, pcaTmp);
                        argc++;
                        argv = CSLAddString(argv, pmCursor->value);
                        argc++;
                        free(pcaTmp);
                    }
                }
            }
        }
        pmsCursor=pmsCursor->next;
    }
    *iArgc=argc;
    *pccArgv=argv;
}

