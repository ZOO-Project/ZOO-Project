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
	outputs=(maps*)malloc(sizeof(maps*));
	outputs->name="Result";
	char tmp[256];
	sprintf(tmp,"%i",res);
	outputs->content=createMap("value",tmp);
	addMapToMap(&outputs->content,createMap("datatype","float"));
	addMapToMap(&outputs->content,createMap("uom","meter"));
	outputs->next=NULL;
	dumpMaps(outputs);
  	fprintf(stderr,"\nService internal print\n===\n");
	return SERVICE_SUCCEEDED;
  }

  int helloworld1(maps*& conf,maps*& inputs,maps*& outputs){
    outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="Result";
    outputs->content=createMap("value","Hello World");
    addMapToMap(&outputs->content,createMap("datatype","string"));
    return SERVICE_SUCCEEDED; 
  }

  int helloworld(map*& inputs,map*& outputs){
    outputs=createMap("output_0","Hello World\n");
    return SERVICE_SUCCEEDED; 
  }

  int printArguments(map*& inputs,map*& outputs){
    char *res=(char *)malloc(sizeof(char));
    map* tmp=inputs;
    while(tmp!=NULL){
      res=(char *)realloc(res,strlen(res)+strlen(tmp->value)+strlen(tmp->name)+6);
      sprintf(res,"%s,\"%s\"=\"%s\"",res,tmp->name,tmp->value);
      //sprintf(res,"%s,\"%s\"=\"%s\"",res,tmp->name,tmp->value);
      tmp=tmp->next;
    }
    char *tmpVal=strdup(res+1);
    outputs=createMap("output_0",tmpVal);
    addToMap(outputs,"output_1",tmpVal);

    /*dumpMap(outputs);
      dumpMap(inputs);*/
    return SERVICE_SUCCEEDED; 
  }

  int buildJsonArrayOfArgs(map*& inputs,map*& outputs){
    char *res=(char *)malloc(sizeof(char));
    map* tmp=inputs;
    while(tmp!=NULL){
      res=(char *)realloc(res,strlen(res)+strlen(tmp->value)+3);
      sprintf(res,"%s,\"%s\"",res,tmp->value);
      tmp=tmp->next;
    }
    char *tmpVal;
    if(strncmp(res,",",1)!=0){
      free(tmpVal);
      tmpVal=strdup(res+1);
      //dumpMap(inputs);      
    }else
      tmpVal=strdup(res);
    tmpVal=(char*)realloc(tmpVal,strlen(tmpVal)+2);
    sprintf(tmpVal,"[%s]",tmpVal);
    outputs=createMap("output_0",tmpVal+1);
    //dumpMap(outputs);
    //dumpMap(inputs);
    return SERVICE_SUCCEEDED; 
  }

}
