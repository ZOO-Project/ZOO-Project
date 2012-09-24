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
