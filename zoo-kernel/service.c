#include "service.h"

map* createMap(char* name,char* value){
  map* tmp=(map *)malloc(sizeof(map*));
  tmp->name=strdup(name);
  tmp->value=strdup(value);
  tmp->next=NULL;
  return tmp;
}

bool hasKey(map* m,char *key){
  map* tmp=m;
  while(tmp!=NULL){
    if(strcmp(tmp->name,key)==0)
      return true;
    tmp=tmp->next;
  }
#ifdef DEBUG_MAP
  printf("NOT FOUND \n");
#endif
  return false;
}

map* getMap(map* m,char *key){
  map* tmp=m;
  while(tmp!=NULL){
    if(strcmp(tmp->name,key)==0)
      return tmp;
    tmp=tmp->next;
  }
  return NULL;
}

void* addToMap(map* m,char* n,char* v){
  if(hasKey(m,n)==false){
    map* _cursor=m;
    while(_cursor->next)
      _cursor=_cursor->next;
    _cursor->next=createMap(n,v);
  }
  else{
    map *tmp=getMap(m,n);
    tmp->value=strdup(v);
  }
}

void* _dumpMap(map* t){
  fprintf(stderr,"[%s] => [%s] \n",t->name,t->value);
  fflush(stderr);
}

void* dumpMap(map* t){
  map* tmp=t;
  while(tmp!=NULL){
    _dumpMap(tmp);
    tmp=tmp->next;
  }
}
