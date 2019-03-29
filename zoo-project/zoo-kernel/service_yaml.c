/*
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

#include <stdio.h>
#include <ctype.h>
#include <service.h>
#include <yaml.h>

static service* my_service=NULL;
static map* current_content=NULL;
static elements* current_element=NULL;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read and parse a ZCFG file in YAML format 
 *
 * @param conf the conf maps containing the main.cfg settings
 * @param file the file name to read
 * @param service the service structure to fill
 * @param name the service name
 * @return 1 on success, -1 if error occurred
 */
int getServiceFromYAML(maps* conf, char* file,service** service,char *name){
  FILE *fh;
  if(current_content!=NULL){
    freeMap(&current_content);
    free(current_content);
    current_content=NULL;
  }
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"(STARTING)FREE current_element\n");
#endif
  if(current_element!=NULL){
    freeElements(&current_element);
    free(current_element);
    current_element=NULL;
  }
  my_service=NULL;
  
  my_service=*service;
  my_service->name=strdup(name);
  my_service->content=NULL;
  my_service->metadata=NULL;
  my_service->inputs=NULL;
  my_service->outputs=NULL;
  fh = fopen(file,"r");
  if (fh==NULL){
    fprintf(stderr,"error : file not found\n") ;
    return -1;
  }
  yaml_parser_t parser;
  yaml_token_t  token;   /* new variable */

  /* Initialize parser */
  if(!yaml_parser_initialize(&parser))
    fputs("Failed to initialize parser!\n", stderr);
  if(fh == NULL)
    fputs("Failed to open file!\n", stderr);
  /* Set input file */
  yaml_parser_set_input_file(&parser, fh);
  /* BEGIN new code */
  int nleveld=-1;
  int level=0;
  int plevel=level;
  int nlevel=0;
  int pnlevel=0;
  int ilevel=-1;
  int blevel=-1;
  int ttype=0;
  int outputs=-1;
  int noutputs=-1;
  int wait_metadata=-1;
  char *cur_key;
  do {
    yaml_parser_scan(&parser, &token);
    switch(token.type)
    {
    /* Stream start/end */
    case YAML_STREAM_START_TOKEN: 
#ifdef DEBUG_YAML
      puts("STREAM START"); 
#endif
      break;
    case YAML_STREAM_END_TOKEN:   
#ifdef DEBUG_YAML
      puts("STREAM END");   
#endif
      break;
    /* Token types (read before actual token) */
    case YAML_KEY_TOKEN:   
#ifdef DEBUG_YAML
      printf("(Key token)   "); 
#endif
      ttype=0;
      break;
    case YAML_VALUE_TOKEN: 
#ifdef DEBUG_YAML
      printf("(Value token) "); 
#endif
      ttype=1;
      break;
    /* Block delimeters */
    case YAML_BLOCK_SEQUENCE_START_TOKEN: 
#ifdef DEBUG_YAML
      puts("<b>Start Block (Sequence)</b>"); 
#endif
      break;
    case YAML_BLOCK_ENTRY_TOKEN:          
#ifdef DEBUG_YAML
      puts("<b>Start Block (Entry)</b>");    
#endif
      break;
    case YAML_BLOCK_END_TOKEN:      
      blevel--;
      if(ilevel>=0)
	ilevel--;
#ifdef DEBUG_YAML
      printf("<b>End block</b> (%d,%d,%d,%d)\n", blevel,level,ilevel,ttype); 
#endif
      break;
    /* Data */
    case YAML_BLOCK_MAPPING_START_TOKEN:  
#ifdef DEBUG_YAML
      puts("[Block mapping]");            
#endif
      blevel++;
      break;
    case YAML_SCALAR_TOKEN:
      if((blevel-1)/2<nlevel){
	pnlevel=nlevel;
	nlevel=(blevel-1)/2;
	nleveld=1;
      }else
	nleveld=-1;
      if(ttype==0){
	cur_key=zStrdup((char *)token.data.scalar.value);
      }
      if(ttype==1){
	if(current_content==NULL){
	  current_content=createMap(cur_key,(char *)token.data.scalar.value);
	}else{
	  addToMap(current_content,cur_key,(char *)token.data.scalar.value);
	}
	free(cur_key);
	cur_key=NULL;
      }

      if(ttype==0 && blevel==0 && level==0 && strcasecmp((char *)token.data.scalar.value,"MetaData")==0 && blevel==0){
	addMapToMap(&my_service->content,current_content);
#ifdef DEBUG_YAML
	fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
	freeMap(&current_content);
	free(current_content);
	current_content=NULL;
	wait_metadata=1;
      }
      if(ttype==0 && blevel>0 && level>0 && strcasecmp((char *)token.data.scalar.value,"MetaData")==0){
	if(current_element->content==NULL && current_content!=NULL)
	  addMapToMap(&current_element->content,current_content);
#ifdef DEBUG_YAML
	dumpMap(current_content);
	fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
	freeMap(&current_content);
	free(current_content);
	current_content=NULL;
	wait_metadata=1;
      }
      if(strcasecmp((char *)token.data.scalar.value,"Child")==0){
	elements* cursor=my_service->inputs;
	if(outputs==1)
	  cursor=my_service->outputs;
	for(int i=0;(blevel/2)>1 && i<(blevel/2)-1 && cursor!=NULL;i++){
	  while(cursor->next!=NULL)
	    cursor=cursor->next;
	  if(cursor->child!=NULL){
	    cursor=cursor->child;
	  }
	}
	if(current_content!=NULL){
	  addMapToMap(&current_element->content,current_content);
	  freeMap(&current_content);
	  free(current_content);
	  current_content=NULL;
	}
	if(current_element!=NULL){
	  if(blevel/2>1 && cursor!=NULL){
	    if(cursor->child==NULL)
	      cursor->child=dupElements(current_element);
	    else
	      addToElements(&cursor->child,current_element);
	  }
	  else{
	    if(outputs<0)
	      if(my_service->inputs==NULL)
		my_service->inputs=dupElements(current_element);
	      else
		addToElements(&my_service->inputs,current_element);
	    else
	      if(my_service->inputs==NULL)
		my_service->outputs=dupElements(current_element);
	      else
		addToElements(&my_service->outputs,current_element);
	  }
	  freeElements(&current_element);
	  free(current_element);
	  current_element=NULL;
	}
	nlevel+=1;
      }
      if(ttype==0 && strcasecmp((char *)token.data.scalar.value,"inputs")==0 && blevel==0){
	if(wait_metadata>0){
	  addMapToMap(&my_service->metadata,current_content);
	  wait_metadata=-1;
	}else{
	  if(current_content!=NULL && my_service->content==NULL)
	    addMapToMap(&my_service->content,current_content);
	}
#ifdef DEBUG_YAML
	dumpMap(current_content);
	fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
	freeMap(&current_content);
	free(current_content);
	current_content=NULL;
	wait_metadata=false;
	level++;
      }
      if(ttype==0 && strcasecmp((char *)token.data.scalar.value,"outputs")==0 && blevel==1){
	outputs=1;
	level++;
#ifdef DEBUG_YAML
	dumpMap(current_content);
	printf("\n***\n%d (%d,%d,%d,%d)\n+++\n", current_element->defaults==NULL,blevel,level,ilevel,ttype); 
#endif
	if(current_element->defaults==NULL && current_content!=NULL && ilevel<0){
	  current_element->defaults=(iotype*)malloc(IOTYPE_SIZE);
	  current_element->defaults->content=NULL;
	  current_element->defaults->next=NULL;
	  addMapToMap(&current_element->defaults->content,current_content);
#ifdef DEBUG_YAML
	  dumpElements(current_element);
	  dumpMap(current_content);
	  fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
	  freeMap(&current_content);
	  free(current_content);
	  current_content=NULL;
	}else{
	  if(current_content!=NULL && ilevel<=0){
	    addMapToIoType(&current_element->supported,current_content);
#ifdef DEBUG_YAML
	    dumpElements(current_element);
	    dumpMap(current_content);
	    fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
	    freeMap(&current_content);
	    free(current_content);
	    current_content=NULL;
	  }
	}
      }
      if(level==1 && strcasecmp((char *)token.data.scalar.value,"default")==0){
	ilevel=0;
      }
      if(level==1 && strcasecmp((char *)token.data.scalar.value,"supported")==0){
#ifdef DEBUG_YAML
	dumpMap(current_content);
	printf("\n***\n%d (%d,%d,%d,%d)\n+++\n", current_element->defaults==NULL,blevel,level,ilevel,ttype); 
#endif
	if(current_element->defaults==NULL && current_content!=NULL && ilevel<0){
	  current_element->defaults=(iotype*)malloc(IOTYPE_SIZE);
	  current_element->defaults->content=NULL;
	  current_element->defaults->next=NULL;
	  addMapToMap(&current_element->defaults->content,current_content);
#ifdef DEBUG_YAML
	  dumpElements(current_element);
	  dumpMap(current_content);
	  fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
	  freeMap(&current_content);
	  free(current_content);
	  current_content=NULL;
	}else{
	  if(current_content!=NULL && ilevel<=0){
	    if(current_element->supported==NULL){
	      current_element->supported=(iotype*)malloc(IOTYPE_SIZE);
	      current_element->supported->content=NULL;
	      current_element->supported->next=NULL;
	    }
	    addMapToMap(&current_element->supported->content,current_content);
#ifdef DEBUG_YAML
	    dumpElements(current_element);
	    fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
	    freeMap(&current_content);
	    free(current_content);
	    current_content=NULL;
	  }
	}
	ilevel=1;
      }


      if(strncasecmp((char *)token.data.scalar.value,"ComplexData",11)==0 || 
	 strncasecmp((char *)token.data.scalar.value,"LiteralData",10)==0 || 
	 strncasecmp((char *)token.data.scalar.value,"ComplexOutput",13)==0 || 
	 strncasecmp((char *)token.data.scalar.value,"LiteralOutput",12)==0 || 
	 strncasecmp((char *)token.data.scalar.value,"BoundingBoxOutput",13)==0 || 
	 strncasecmp((char *)token.data.scalar.value,"BoundingBoxData",12)==0){
	current_element->format=zStrdup((char *)token.data.scalar.value);
	free(cur_key);
	cur_key=NULL;
	if(wait_metadata>0 && current_content!=NULL){
	  addMapToMap(&current_element->metadata,current_content);
	  wait_metadata=-1;
	}else{
	  if(current_content!=NULL){
	    addMapToMap(&current_element->content,current_content);
	  }
	}
#ifdef DEBUG_YAML
	dumpMap(current_content);
	fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
	freeMap(&current_content);
	free(current_content);
	current_content=NULL;
#ifdef DEBUG_YAML
	dumpElements(current_element);
#endif
      }

      if(strcasecmp(token.data.scalar.value,"default")!=0 && strcasecmp(token.data.scalar.value,"supported")!=0 && level==1 && (blevel-1)%2==0 && blevel!=1 && (blevel-1)/2==nlevel){
	if(current_element==NULL)
	  current_element=createElements((char *)token.data.scalar.value);
	else{
	  if(current_content!=NULL){
	    if(current_element->defaults==NULL){
	      current_element->defaults=(iotype*)malloc(IOTYPE_SIZE);
	      current_element->defaults->content=NULL;
	      current_element->defaults->next=NULL;
	      addMapToMap(&current_element->defaults->content,current_content);
	    }else{
	      if(current_element->supported==NULL){
		current_element->supported=(iotype*)malloc(IOTYPE_SIZE);
		current_element->supported->content=NULL;
		current_element->supported->next=NULL;
		addMapToMap(&current_element->supported->content,current_content);
	      }else
		addMapToIoType(&current_element->supported,current_content);
	    }
	    freeMap(&current_content);
	    free(current_content);
	    current_content=NULL;
	  }
	  
	  elements* cursor=my_service->inputs;
	  if(outputs==1)
	    cursor=my_service->outputs;
	  int llevel=((blevel-1)/2);
	  if(nleveld>0)
	    llevel=((blevel-1)/2)+1;
	  for(int i=0;llevel>1 && i<llevel-1 && cursor!=NULL;i++){
	    while(cursor->next!=NULL)
	      cursor=cursor->next;
	    if(cursor->child!=NULL){
	      cursor=cursor->child;
	    }
	  }
	  if(llevel>1)
	    if(cursor->child==NULL)
	      cursor->child=dupElements(current_element);
	    else
	      addToElements(&cursor->child,current_element);
	  else
	    if(cursor==NULL)
	      cursor=dupElements(current_element);
	    else
	      addToElements(&cursor,current_element);
	  freeElements(&current_element);
	  free(current_element);
	  current_element=NULL;
	  current_element=createElements((char *)token.data.scalar.value);
	}
      }
      if(blevel==1 && level==1){
	if(current_element!=NULL && current_content!=NULL){
	  if(current_element->defaults==NULL){
	    current_element->defaults=(iotype*)malloc(IOTYPE_SIZE);
	    current_element->defaults->content=NULL;
	    current_element->defaults->next=NULL;
	    addMapToMap(&current_element->defaults->content,current_content);
	  }else{
	    if(current_element->supported==NULL){
	      current_element->supported=(iotype*)malloc(IOTYPE_SIZE);
	      current_element->supported->content=NULL;
	      current_element->supported->next=NULL;
	      addMapToMap(&current_element->supported->content,current_content);
	    }else
	      addMapToIoType(&current_element->supported,current_content);
	  }
	  freeMap(&current_content);
	  free(current_content);
	  current_content=NULL;
	}
	if(nleveld<0){
	  if(current_element!=NULL){
	    if(outputs<0)
	      if(my_service->inputs==NULL)
		my_service->inputs=dupElements(current_element);
	      else
		addToElements(&my_service->inputs,current_element);
	    else
	      if(my_service->outputs==NULL)
		my_service->outputs=dupElements(current_element);
	      else
		addToElements(&my_service->outputs,current_element);
	    freeElements(&current_element);
	    free(current_element);
	  }
	}
	else{
	  if(current_element!=NULL){
	    elements* cursor=my_service->inputs;
	    if(outputs==1)
	      cursor=my_service->outputs;
	    while(cursor->next!=NULL)
	      cursor=cursor->next;
	    for(int i=0;pnlevel>1 && i<pnlevel-1 && cursor!=NULL;i++){
	      while(cursor->next!=NULL)
		cursor=cursor->next;
	      if(cursor->child!=NULL){
		cursor=cursor->child;
	      }
	    }
	    if(cursor->child==NULL)
	      cursor->child=dupElements(current_element);
	    else
	      addToElements(&cursor->child,current_element);
	    freeElements(&current_element);
	    free(current_element);
	  }
	}   
	plevel=level;
	current_element=createElements((char *)token.data.scalar.value);	
      }
      if(blevel==1 && level==2){
	if(current_element!=NULL && current_content!=NULL){
	  if(current_element->defaults==NULL){
	    current_element->defaults=(iotype*)malloc(IOTYPE_SIZE);
	    current_element->defaults->content=NULL;
	    current_element->defaults->next=NULL;
	    addMapToMap(&current_element->defaults->content,current_content);
	  }else{
	    if(current_element->supported==NULL){
	      current_element->supported=(iotype*)malloc(IOTYPE_SIZE);
	      current_element->supported->content=NULL;
	      current_element->supported->next=NULL;
	      addMapToMap(&current_element->supported->content,current_content);
	    }else
	      addMapToIoType(&current_element->supported,current_content);
	  }
	  freeMap(&current_content);
	  free(current_content);
	  current_content=NULL;
	}
	if(current_element!=NULL){
	  if(plevel==level){
	    if(my_service->outputs==NULL)
	      my_service->outputs=dupElements(current_element);
	    else
	      addToElements(&my_service->outputs,current_element);
	  }else{
	    if(my_service->inputs==NULL)
	      my_service->inputs=dupElements(current_element);
	    else
	      addToElements(&my_service->inputs,current_element);
	  }
	  freeElements(&current_element);
	  free(current_element);
	}
	plevel=level;
	current_element=createElements((char *)token.data.scalar.value);	
	
      }

      if(noutputs>0)
	outputs=1;
      if(strcasecmp((char *)token.data.scalar.value,"outputs")==0 && ttype==0 && blevel==0){
	noutputs=1;
      }
      


#ifdef DEBUG_YAML
      printf("scalar %s (%d,%d,%d,%d,%d)\n", token.data.scalar.value,blevel,level,plevel,ilevel,ttype); 
#endif
      break;
    /* Others */
    default:
      if(token.type==0){
	char tmp[1024];
	sprintf(tmp,"Wrong charater found in %s: \\t",name);
	setMapInMaps(conf,"lenv","message",tmp);
	return -1;
      }
#ifdef DEBUG_YAML
      printf("Got token of type %d\n", token.type);
#endif
      break;
    }
    if(token.type != YAML_STREAM_END_TOKEN )
      yaml_token_delete(&token);
  } while(token.type != YAML_STREAM_END_TOKEN);
  yaml_token_delete(&token);


#ifdef DEBUG_YAML
  fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
  if(current_element!=NULL && current_content!=NULL){
    if(current_element->defaults==NULL){
      current_element->defaults=(iotype*)malloc(IOTYPE_SIZE);
      current_element->defaults->content=NULL;
      current_element->defaults->next=NULL;
      addMapToMap(&current_element->defaults->content,current_content);
    }else{
      if(current_element->supported==NULL){
	current_element->supported=(iotype*)malloc(IOTYPE_SIZE);
	current_element->supported->content=NULL;
	current_element->supported->next=NULL;
	addMapToMap(&current_element->supported->content,current_content);
      }else
	addMapToIoType(&current_element->supported,current_content);
    }
#ifdef DEBUG_YAML
    dumpMap(current_content);
    fprintf(stderr,"MSG: %s %d \n",__FILE__,__LINE__);
#endif
    freeMap(&current_content);
    free(current_content);
    current_content=NULL;
  }

  if(current_element!=NULL){
    if(nlevel>0){
      elements* cursor=my_service->inputs;
      if(outputs==1)
	cursor=my_service->outputs;
      for(int i=0;nlevel>1 && i<nlevel-1 && cursor!=NULL;i++){
	while(cursor->next!=NULL)
	  cursor=cursor->next;
	if(cursor->child!=NULL){
	  cursor=cursor->child;
	}
      }
      if(cursor->child==NULL)
	cursor->child=dupElements(current_element);
      else
	addToElements(&cursor->child,current_element);
    }else{
      if(my_service->outputs==NULL)
	my_service->outputs=dupElements(current_element);
      else
	addToElements(&my_service->outputs,current_element);
    }
    freeElements(&current_element);
    free(current_element);
    current_element=NULL;
  }
  /* END new code */

  /* Cleanup */
  yaml_parser_delete(&parser);
  fclose(fh);

#ifdef DEBUG_YAML
  dumpService(my_service);
#endif
  *service=my_service;

  return 1;
}
#ifdef __cplusplus
}
#endif
