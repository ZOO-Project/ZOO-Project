/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 209-2015 GeoLabs SARL
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
%{

#include <string>
#include <stdio.h>
#include <ctype.h>
#include <service.h>

static int tmp_count=1;
static int defaultsc=0;
static bool wait_maincontent=true;
static bool wait_mainmetadata=false;
static bool wait_mainap=false;
static bool wait_metadata=false;
static bool wait_ap=false;
static bool wait_nested=false;
static bool wait_inputs=false;
static bool wait_defaults=false;
static bool wait_supporteds=false;
static bool wait_outputs=false;
static bool wait_data=false;
static service* my_service=NULL;
static map* current_content=NULL;
static elements* current_element=NULL;
static char* curr_key;
static int debug=0;
static int data=-1;
static int previous_data=0;
static int current_data=0;
static int nested_level=0;
// namespace
using namespace std;

// srerror
void srerror(const char *s);

// usage ()
void usage(void) ;

// srdebug
extern int srdebug;

extern char srtext[];

// srlineno
extern int srlineno;

// srin
extern FILE* srin;

// srlex
extern int srlex(void);
extern int srlex_destroy(void);

%}



%union 
{char * s;char* chaine;char* key;char* val;}

// jetons //
%token <s> ID
%token <s> CHAINE

%token INFCAR SUPCAR 

/* / = a1  texte "texte" */
%token SLASH Eq CHARDATA ATTVALUE PAIR SPAIR EPAIR ANID
%type <chaine> PAIR
%type <chaine> EPAIR
%type <chaine> SPAIR

/* <!-- xxx -> <? xxx yyy ?> */
%token PI PIERROR /** COMMENT **/

/* <!-- xxx -> <? xxx yyy ?> */
%token ERREURGENERALE CDATA WHITESPACE NEWLINE
%type <s> STag
%type <s> ETag
%type <s> ANID

// % start

%%
// document 
// regle 1
// on est a la racine du fichier xml
document
 : miscetoile element miscetoile {}
 | contentetoile processid contentetoile document {}
 ;

miscetoile
 : miscetoile PIERROR {  srerror("processing instruction beginning with <?xml ?> impossible\n");}
 | miscetoile PI {}
 | {}
 ;
// element
// regle 39
// OUVRANTE CONTENU FERMANTE obligatoirement
// ou neutre
// on ne peut pas avoir Epsilon
// un fichier xml ne peut pas etre vide ou seulement avec un prolog
element
 : STag contentetoile ETag	
{
}

// pour neutre
// on a rien a faire, meme pas renvoyer l identificateur de balise
// vu qu'il n y a pas de comparaison d'identificateurs avec un balise jumelle .
 | EmptyElemTag          {}
 ;

// STag
// regle 40
// BALISE OUVRANTE
// on est obligé de faire appel a infcar et supcar
// pour acceder aux start conditions DANSBALISE et INITIAL
STag
: INFCAR ID Attributeetoile SUPCAR
{
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"STag (%s %d) %s %d %d \n",__FILE__,__LINE__,$2,current_data,previous_data);
  fflush(stderr);
  dumpMap(current_content);
#endif
  if(my_service->content==NULL && current_content!=NULL){
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"NO CONTENT\n");
#endif
    addMapToMap(&my_service->content,current_content);
    freeMap(&current_content);
    free(current_content);
    current_content=NULL;
    my_service->metadata=NULL;
    my_service->additional_parameters=NULL;
    wait_maincontent=false;
  }
  if(strncasecmp($2,"EndNested",9)==0){
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"(ENDNESTED - %d) \n",__LINE__);
    fflush(stderr);
#endif
    nested_level-=1;
    if(nested_level==0){
      wait_nested=false;
    }
  }

  if(strncasecmp($2,"DataInputs",10)==0){
    if(current_element==NULL){
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"(DATAINPUTS - %d) FREE current_element\n",__LINE__);
#endif
      freeElements(&current_element);
      free(current_element);
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"(DATAINPUTS - %d) ALLOCATE current_element\n",__LINE__);
#endif
      current_element=NULL;
      current_element=createEmptyElements();
    }
    wait_inputs=true;
    current_data=1;
    previous_data=1;
  }
  else
    if(strncasecmp($2,"DataOutputs",11)==0){
      if(wait_inputs==true){
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"(DATAOUTPUTS %d) DUP INPUTS current_element\n",__LINE__);
	fprintf(stderr,"CURRENT_ELEMENT\n");
	dumpElements(current_element);
	fprintf(stderr,"SERVICE INPUTS\n");
	dumpElements(my_service->inputs);
	dumpService(my_service);
#endif	
	if(my_service->inputs==NULL && current_element!=NULL && current_element->name!=NULL){
	  my_service->inputs=dupElements(current_element);
	  my_service->inputs->next=NULL;
	  freeElements(&current_element);
	}
	else if(current_element!=NULL && current_element->name!=NULL){
	  addToElements(&my_service->inputs,current_element);
	  freeElements(&current_element);
	}
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"CURRENT_ELEMENT\n");
	dumpElements(current_element);
	fprintf(stderr,"SERVICE INPUTS\n");
	dumpElements(my_service->inputs);
	fprintf(stderr,"(DATAOUTPUTS) FREE current_element\n");
#endif
	free(current_element);
	current_element=NULL;
	wait_inputs=false;
      }
      if(current_element==NULL){
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"(DATAOUTPUTS - %d) ALLOCATE current_element (%s)\n",__LINE__,$2);
	fflush(stderr);
#endif
	current_element=createEmptyElements();
      }
      wait_outputs=1;
      current_data=2;
      previous_data=2;
    }
    else
      if(strncasecmp($2,"MetaData",8)==0 ||
	 strncasecmp($2,"AdditionalParameters",8)==0){
	if(strncasecmp($2,"AdditionalParameters",8)==0){
	  previous_data=current_data;
	  current_data=4;
	  if(current_element!=NULL){
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"add current_content to current_element->content\n");
	    fprintf(stderr,"LINE %d",__LINE__);
#endif
	    if(wait_mainmetadata)
	      addMapToMap(&my_service->metadata,current_content);
	    else
	      if(wait_metadata)
		addMapToMap(&current_element->metadata,current_content);
	      else
		addMapToMap(&current_element->content,current_content);		
	    freeMap(&current_content);
	    free(current_content);
	    if(previous_data==1 || previous_data==2)
	      wait_ap=true;
	    else
	      wait_mainap=true;
	  }
	  else{
	    if(previous_data==1 || previous_data==2)
	      wait_ap=true;
	    else
	      wait_mainap=true;
	  }
	}else{
	  previous_data=current_data;
	  current_data=3;
	  if(current_element!=NULL){
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"add current_content to current_element->content\n");
	    fprintf(stderr,"LINE %d",__LINE__);
#endif
	    addMapToMap(&current_element->content,current_content);
	    freeMap(&current_content);
	    free(current_content);
	    if(previous_data==1 || previous_data==2)
	      wait_metadata=true;
	    else
	      wait_mainmetadata=true;
	  }
	  else{
	    if(previous_data==1 || previous_data==2)
	      wait_metadata=true;
	    else
	      wait_mainmetadata=true;
	  }
	}
	current_content=NULL;
      }
      else
	if(strncasecmp($2,"ComplexData",11)==0 || strncasecmp($2,"LiteralData",10)==0
	   || strncasecmp($2,"ComplexOutput",13)==0 || strncasecmp($2,"LiteralOutput",12)==0
	   || strncasecmp($2,"BoundingBoxOutput",13)==0 || strncasecmp($2,"BoundingBoxData",12)==0){
	  current_data=4;
	  addMapToMap(&current_element->content,current_content);
	  freeMap(&current_content);
	  free(current_content);
	  current_element->next=NULL;
	  if($2!=NULL)
	    current_element->format=zStrdup($2);
	  current_element->defaults=NULL;
	  current_element->supported=NULL;
	  current_element->child=NULL;
	  current_content=NULL;
	}
	else
	  if(strncasecmp($2,"Default",7)==0){
	    wait_defaults=true;
	    current_data=5;
	  }
	  else
	    if(strncasecmp($2,"Supported",9)==0){
	      wait_supporteds=true;
	      if(wait_defaults==true){
		defaultsc++;
	      }
	      current_data=5;
	    }
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"* Identifiant : %s\n",$2);
  fflush(stderr);
#endif
}
 ;

// Attributeetoile
// regle 41
// une liste qui peut etre vide d'attributs
// utiliser la récursivité a gauche
Attributeetoile
: Attributeetoile attribute  {}
 | 	                          {/* Epsilon */}
 ;

// attribute
// regle 41
// un attribut est compose d'un identifiant
// d'un "="
// et d'une définition de chaine de caractere
// ( "xxx" ou 'xxx' )
attribute
 : ID Eq ATTVALUE		
{
#ifdef DEBUG_SERVICE_CONF
  printf ("attribute : %s\n",$1) ;
#endif
}
 ;

// EmptyElemTag
// regle 44
// ICI ON DEFINIT NEUTRE
// on ne renvoie pas de char*
// parce qu'il n'y a pas de comparaisons a faire
// avec un identifiant d'une balise jumelle
EmptyElemTag
 : INFCAR ID Attributeetoile SLASH SUPCAR	{
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"(%s %d)\n",__FILE__,__LINE__);
#endif
   if(strncasecmp($2,"Default",7)==0){
     wait_defaults=false;
     current_data=previous_data;
     if(current_element->defaults==NULL){
       current_element->defaults=(iotype*)malloc(IOTYPE_SIZE);
       current_element->defaults->content=NULL;
     }
     addMapToMap(&current_element->defaults->content,current_content);
     freeMap(&current_content);
     free(current_content);
     current_element->defaults->next=NULL;
     wait_defaults=false;
     current_content=NULL;
     current_element->supported=NULL;
     current_element->child=NULL;
     current_element->next=NULL;
   }
   if(strncasecmp($2,"EndNested",9)==0){
     if(current_data==1){
       elements* cursor=my_service->inputs;
       while(cursor->next!=NULL)
	 cursor=cursor->next;
       if(nested_level>1){
	 for(int j=0;j<nested_level-1;j++){
	   cursor=cursor->child;
	   while(cursor->next!=NULL)
	     cursor=cursor->next;
	 }
       }
       if(cursor->child==NULL){
	 cursor->child=dupElements(current_element);
       }else{
	 addToElements(&cursor->child,current_element);
       }
     }else{
       if(current_element->name!=NULL){
	 elements* cursor=my_service->outputs;
	 while(cursor->next!=NULL)
	   cursor=cursor->next;
	 if(nested_level>1){
	   for(int j=0;j<nested_level-1;j++){
	     cursor=cursor->child;
	     while(cursor->next!=NULL)
	       cursor=cursor->next;
	   }
	 }
	 if(cursor->child==NULL){
	   cursor->child=dupElements(current_element);
	 }else
	   addToElements(&cursor->child,current_element);
       }
     }
     freeElements(&current_element);
     free(current_element);
     current_element=NULL;
     current_element=createEmptyElements();
     nested_level-=1;
     if(nested_level==0){
       wait_nested=false;       
     }
   }
 }
 ;

// ETag
// BALISE FERMANTE
// les separateurs après ID sont filtrés
ETag
 : INFCAR SLASH ID SUPCAR
{
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"ETag %s (%s %d) %d %d \n",$3,__FILE__,__LINE__,current_data,previous_data);
#endif
  if(strcmp($3,"DataInputs")==0){
    current_data=1;
    if(current_content!=NULL){
      if(current_element->content==NULL){
	addMapToMap(&current_element->content,current_content);
      }
      freeMap(&current_content);
      free(current_content);
      current_content=NULL;
    }
    if(current_element!=NULL){
      if(my_service->content!=NULL && current_element->name!=NULL){
	if(my_service->inputs==NULL){
	  my_service->inputs=dupElements(current_element);
	  my_service->inputs->next=NULL;
	  tmp_count++;
	}
	else{
	  addToElements(&my_service->inputs,current_element);
	}
	freeElements(&current_element);
	free(current_element);
	current_element=NULL;
      }
    }
  }
  if(strcmp($3,"DataOutputs")==0){
    current_data=2;
  }  
  if(strcmp($3,"AdditionalParameters")==0){
    if(current_content!=NULL){
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"add current_content to current_element->content\n");
      fprintf(stderr,"LINE %d",__LINE__);
#endif
      if(wait_ap){
	current_element->additional_parameters=NULL;
	addMapToMap(&current_element->additional_parameters,current_content);
	current_element->next=NULL;
	current_element->defaults=NULL;
	current_element->supported=NULL;
	current_element->child=NULL;
      }else{
	if(wait_mainap){
	  addMapToMap(&my_service->additional_parameters,current_content);
	}
      }

      freeMap(&current_content);
      free(current_content);
      current_content=NULL;
    }
    current_data=previous_data;
    wait_mainap=false;
    wait_ap=false;
  }
  if(strcmp($3,"MetaData")==0){
    if(current_content!=NULL){
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"add current_content to current_element->content\n");
      fprintf(stderr,"LINE %d",__LINE__);
#endif
      if(wait_metadata){
	current_element->metadata=NULL;
	addMapToMap(&current_element->metadata,current_content);
	current_element->next=NULL;
	current_element->defaults=NULL;
	current_element->supported=NULL;
	current_element->child=NULL;
      }else{
	if(wait_mainmetadata){
	  addMapToMap(&my_service->metadata,current_content);
	}
      }

      freeMap(&current_content);
      free(current_content);
      current_content=NULL;
    }
    current_data=previous_data;
    wait_mainmetadata=false;
    wait_metadata=false;
  }
  if(strcmp($3,"ComplexData")==0 || strcmp($3,"LiteralData")==0 
     || strcmp($3,"ComplexOutput")==0 || strcmp($3,"LiteralOutput")==0){
    if(current_element->format==NULL)
      current_element->format=zStrdup($3);
    current_content=NULL;
  }
  if(strcmp($3,"Default")==0){
    current_data=previous_data;
    if(current_element->defaults==NULL){
      current_element->defaults=(iotype*)malloc(IOTYPE_SIZE);
      current_element->defaults->content=NULL;
    }
    addMapToMap(&current_element->defaults->content,current_content);
    freeMap(&current_content);
    free(current_content);
    current_element->defaults->next=NULL;
    wait_defaults=false;
    current_content=NULL;
    current_element->supported=NULL;
    current_element->child=NULL;
    current_element->next=NULL;
  }
  if(strcmp($3,"Supported")==0){
    current_data=previous_data;
    if(current_element->supported==NULL){
      if(current_content!=NULL){
	current_element->supported=(iotype*)malloc(IOTYPE_SIZE);
	current_element->supported->content=NULL;
	addMapToMap(&current_element->supported->content,current_content);
	freeMap(&current_content);
	free(current_content);
	current_element->supported->next=NULL;
	current_content=NULL;
      }else{
	current_element->supported=NULL;
	current_element->child=NULL;
	current_element->next=NULL;
      }
    }
    else{
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"SECOND SUPPORTED FORMAT !!!!\n");
#endif
      addMapToIoType(&current_element->supported,current_content);
      freeMap(&current_content);
      free(current_content);
      current_content=NULL;
#ifdef DEBUG_SERVICE_CONF
      dumpElements(current_element);
      fprintf(stderr,"SECOND SUPPORTED FORMAT !!!!\n");
#endif
    }
    current_content=NULL;
  }
}
 ;

//======================================================
// contentetoile
//======================================================
// regle 43
// ENTRE 2 BALISES
// entre 2 balises, on peut avoir :
// --- OUVRANTE CONTENU FERMANTE (recursivement !)
// --- DU TEXTE quelconque
// --- COMMENTS 
// --- DES PROCESSES INSTRUCTIONS
// --- /!\ il peut y avoir une processing instruction invalide ! <?xml
// --- EPSILON
// ### et/ou tout ca a la suite en nombre indeterminé
// ### donc c'est un operateur etoile (*)
//======================================================
contentetoile
: contentetoile element	          {}
 | contentetoile PIERROR	          {srerror("processing instruction <?xml ?> impossible\n");}
 | contentetoile PI	                  {}
///// on filtre les commentaires | contentetoile comment              {} 
 | contentetoile NEWLINE {/*printf("NEWLINE FOUND !!");*/}
 | contentetoile pair {}
 | contentetoile processid {}
 | contentetoile texteinterbalise	  {}
 | contentetoile CDATA {}  
 | {/* Epsilon */}
 ;

// texteinterbalise
// regle 14
// DU TEXTE quelconque
// c'est du CHARDATA
// il y a eut un probleme avec ID,
// on a mis des starts conditions,
// maintenant on croise les ID dans les dbalises
// et des CHARDATA hors des balises
texteinterbalise
 : CHARDATA		{}
 ;

pair: PAIR { if(debug) fprintf(stderr,"PAIR FOUND !!\n");if(curr_key!=NULL){free(curr_key);curr_key=NULL;} }
| EPAIR {
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"EPAIR (%s %d)\n",__FILE__,__LINE__);
  fprintf(stderr,"[%s=>%s]\n",curr_key,$1);
  dumpMap(current_content);
  fflush(stderr);
#endif
  if($1!=NULL && curr_key!=NULL){
    if(current_content==NULL){
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"[ZOO: service_conf.y line %d free(%s)]\n",__LINE__,curr_key);
#endif
      current_content=createMap(curr_key,$1);
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"[ZOO: service_conf.y line %d free(%s)]\n",__LINE__,curr_key);
#endif
    }
    else{ 
#ifdef DEBUG_SERVICE_CONF
      dumpMap(current_content);
      fprintf(stderr,"addToMap(current_content,%s,%s) !! \n",curr_key,$1); 
#endif
      addToMap(current_content,curr_key,$1);
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"addToMap(current_content,%s,%s) end !! \n",curr_key,$1); 
#endif    
    }
  }
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"[%s=>%s]\n",curr_key,$1);
  fprintf(stderr,"[ZOO: service_conf.y line %d free(%s)]\n",__LINE__,curr_key);
  dumpMap(current_content);
  fflush(stderr);
#endif
  if(curr_key!=NULL){
    free(curr_key);
    curr_key=NULL;
  }
  }
| SPAIR  { if(curr_key!=NULL) {free(curr_key);curr_key=NULL;} if($1!=NULL) curr_key=zStrdup($1);if(debug) fprintf(stderr,"SPAIR FOUND !!\n"); }
 ;


processid
: ANID  {
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"processid (%s %d) %s\n",__FILE__,__LINE__,$1);
#endif
  if(::data==-1){ // knut: add namespace to avoid ambiguous symbol
    ::data=1;	
    if($1!=NULL){
      char *cen=zStrdup($1);
      cen[strlen(cen)-1]=0;
      cen+=1;
      setServiceName(&my_service,cen);
      cen-=1;
      free(cen);
    }
  } else {
    if(current_data==1){
      if(my_service->content!=NULL && current_element->name!=NULL){
	if(my_service->inputs==NULL){
	  my_service->inputs=dupElements(current_element);
	  my_service->inputs->next=NULL;
	  tmp_count++;
	}
	else{
	  if(wait_nested){
	    elements* cursor=my_service->inputs;
	    while(cursor->next!=NULL)
	      cursor=cursor->next;
	    if(nested_level>1){
	      for(int j=0;j<nested_level-1;j++){
		cursor=cursor->child;
		while(cursor->next!=NULL)
		  cursor=cursor->next;
	      }
	    }
	    if(cursor->child==NULL){
	      cursor->child=dupElements(current_element);
	    }else{
	      addToElements(&cursor->child,current_element);
	    }
	  }else
	    addToElements(&my_service->inputs,current_element);
	}
	if(current_element->format==NULL){
	  wait_nested=true;
	  nested_level+=1;
	  if(current_content!=NULL){
	    elements* cursor=my_service->inputs;
	    while(cursor->next!=NULL)
	      cursor=cursor->next;
	    if(nested_level>1){
	      for(int j=0;j<nested_level-1;j++){
		cursor=cursor->child;
		while(cursor->next!=NULL)
		  cursor=cursor->next;
	      }
	    }
	    addMapToMap(&cursor->content,current_content);
	    freeMap(&current_content);
	    free(current_content);
	    current_content=NULL;
	  }
	}
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"(%s %d)FREE current_element (after adding to allread existing inputs)",__FILE__,__LINE__);
	dumpElements(current_element);
	fprintf(stderr,"(%s %d)FREE current_element (after adding to allread existing inputs)",__FILE__,__LINE__);
	dumpElements(my_service->inputs);
#endif
	freeElements(&current_element);
	free(current_element);
	current_element=NULL;
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"(DATAINPUTS - 489) ALLOCATE current_element\n");
#endif
	current_element=createEmptyElements();
      }
      if(current_element->name==NULL){
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"NAME IN %s (current - %s)\n",
		$1,current_element->name);
#endif
	wait_inputs=true;
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"(DATAINPUTS - 501) SET NAME OF current_element\n");
#endif
	if($1!=NULL){ 
	  char *cen=zStrdup($1);
	  cen[strlen(cen)-1]=0;
	  cen+=1;
	  setElementsName(&current_element,cen);
	  cen-=1;
	  free(cen);
#ifdef DEBUG_SERVICE_CONF
	  fprintf(stderr,"NAME IN %s (current - %s)\n",$1,current_element->name);
#endif
#ifdef DEBUG_SERVICE_CONF
	  fprintf(stderr,"NAME IN %s (current - %s)\n",$1,current_element->name);
#endif
	}
      }
    }
    else
      if(current_data==2){ 
	wait_outputs=1;
	if(wait_inputs){
	  if(current_element!=NULL && current_element->name!=NULL){
	    if(my_service->outputs==NULL){
	      my_service->outputs=dupElements(current_element);
	      my_service->outputs->next=NULL;
	    }
	    else{
#ifdef DEBUG_SERVICE_CONF
	      fprintf(stderr,"LAST NAME IN %s (current - %s)\n",$1,current_element->name);
#endif
	      addToElements(&my_service->outputs,current_element);
	    }
#ifdef DEBUG_SERVICE_CONF
	    dumpElements(current_element);
	    fprintf(stderr,"(DATAOUTPUTS) FREE current_element %s %i\n",__FILE__,__LINE__);
#endif
	    freeElements(&current_element);
	    free(current_element);
	    current_element=NULL;
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"(DATAOUTPUTS -%d) ALLOCATE current_element %s \n",__LINE__,__FILE__);
#endif
	    current_element=createEmptyElements();
	  }
	  if(current_element->name==NULL){
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"NAME OUT %s\n",$1);
	    fprintf(stderr,"(DATAOUTPUTS - %d) SET NAME OF current_element\n",__LINE__);
#endif
	    if($1!=NULL){ 
	      char *cen=zStrdup($1);
	      cen[strlen(cen)-1]=0;
	      cen+=1;
	      setElementsName(&current_element,cen);
	      cen-=1;
	      free(cen);
	    }
	  }

	  current_content=NULL;
	}
	else
	  if(current_element!=NULL && current_element->name!=NULL){
	    if(my_service->outputs==NULL)
	      my_service->outputs=dupElements(current_element);
	    else{
	      if(wait_nested){
		elements* cursor=my_service->outputs;
		while(cursor->next!=NULL)
		  cursor=cursor->next;
		if(nested_level>1){
		  for(int j=0;j<nested_level-1;j++){
		    cursor=cursor->child;
		    while(cursor->next!=NULL)
		      cursor=cursor->next;
		  }
		}
		if(cursor->child==NULL){
		  cursor->child=dupElements(current_element);
		}else
		  addToElements(&cursor->child,current_element);
	      }else
		addToElements(&my_service->outputs,current_element);
	    }
	    if(current_element->format==NULL){
	      wait_nested=true;
	      nested_level+=1;
	      if(current_content!=NULL){
		elements* cursor=my_service->outputs;
		while(cursor->next!=NULL)
		  cursor=cursor->next;
		if(nested_level>1){
		  for(int j=0;j<nested_level-1;j++){
		    cursor=cursor->child;
		    while(cursor->next!=NULL)
		      cursor=cursor->next;
		  }
		}
		addMapToMap(&cursor->content,current_content);
		freeMap(&current_content);
		free(current_content);
		current_content=NULL;
	      }
	    }

#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"ADD TO OUTPUTS Elements\n");
	    dupElements(current_element);
#endif
	    freeElements(&current_element);
	    free(current_element);
	    current_element=NULL;
	    
	    char *cen=zStrdup($1);
	    cen[strlen(cen)-1]=0;
	    cen+=1;
	    current_element=createElements(cen);
	    cen-=1;
	    free(cen);
	  }
	  else{
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"NAME OUT %s\n",$1);
	    fprintf(stderr,"(DATAOUTPUTS - %d) SET NAME OF current_element %s\n",__LINE__,$1);
#endif
	    if($1!=NULL){ 
	      char *cen=zStrdup($1);
	      cen[strlen(cen)-1]=0;
#ifdef DEBUG
	      fprintf(stderr,"tmp %s\n",cen);
#endif
	      cen+=1;
	      setElementsName(&current_element,cen);
	      cen-=1;
	      free(cen);
	    }
	  }
	wait_inputs=false;
	wait_outputs=1;
	//wait_outputs=true;
      }
  }
 }
 ;

%%

/**
 * Print on stderr the message and the line number of the error which occurred.
 * 
 * @param s the error message
 */
void srerror(const char *s)
{
  if(debug)
    fprintf(stderr,"\nligne %d : %s\n",srlineno,s);
}

/**
 * Parse a ZCFG file and fill the service structure.
 *
 * @param conf the conf maps containing the main.cfg settings
 * @param file the fullpath to the ZCFG file
 * @param service the service structure to fill
 * @return 0 on success, -1 on failure
 */
int getServiceFromFile(maps* conf,const char* file,service** service){
  if(current_content!=NULL){
    freeMap(&current_content);
    free(current_content);
    current_content=NULL;
  }
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"(STARTING)FREE current_element %s\n",file);
#endif
  fflush(stderr);
  fflush(stdout);
  if(current_element!=NULL){
    freeElements(&current_element);
    free(current_element);
    current_element=NULL;
  }
  my_service=NULL;

  wait_maincontent=true;
  wait_mainmetadata=false;
  wait_metadata=false;
  wait_mainap=false;
  wait_ap=false;
  wait_inputs=false;
  wait_defaults=false;
  wait_supporteds=false;
  wait_outputs=-1;
  wait_data=false;
  wait_nested=false;
//data=-1;
  ::data=-1; // knut: add namespace to avoid ambiguous symbol
  previous_data=1;
  current_data=0;
  
  my_service=*service;

  srin = fopen(file,"r");
  if (srin==NULL){
    setMapInMaps(conf,"lenv","message","file not found");
    return -1;
  }

  int resultatYYParse = srparse() ;

#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"RESULT: %d %d\n",resultatYYParse,wait_outputs);
  dumpElements(current_element);
#endif
  if(wait_outputs && current_element!=NULL && current_element->name!=NULL){
    if(current_content!=NULL){
      addMapToMap(&current_element->content,current_content);
    }
    if(my_service->outputs==NULL){  
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"(DATAOUTPUTS - %d) DUP current_element\n",__LINE__);
#endif
      my_service->outputs=dupElements(current_element);
      my_service->outputs->next=NULL;
    }
    else{
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"(DATAOUTPUTS - %d) COPY current_element\n",__LINE__);
#endif
      if(wait_nested){
	elements* cursor=my_service->outputs;
	while(cursor->next!=NULL)
	  cursor=cursor->next;
	if(nested_level>1){
	  for(int j=0;j<nested_level-1;j++){
	    cursor=cursor->child;
	    while(cursor->next!=NULL)
	      cursor=cursor->next;
	  }
	}
	if(cursor->child==NULL){
	  cursor->child=dupElements(current_element);
	}else
	  addToElements(&cursor->child,current_element);
      }else
	addToElements(&my_service->outputs,current_element);
    }
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"(DATAOUTPUTS - %d) FREE current_element\n",__LINE__);
#endif
    freeElements(&current_element);
    free(current_element);
    current_element=NULL;
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"(DATAOUTPUTS - %d) FREE current_element\n",__LINE__);
#endif
  }
  if(current_element!=NULL){
    freeElements(&current_element);
    free(current_element);
    current_element=NULL;
  }
  int contentOnly=false;
  if(current_content!=NULL){
    if(my_service->content==NULL){
      addMapToMap(&my_service->content,current_content);
      contentOnly=true;
      wait_maincontent=false;
    }
    freeMap(&current_content);
    free(current_content);
    current_content=NULL;
  }
  fclose(srin);
#ifdef DEBUG_SERVICE_CONF
  dumpService(my_service);
#endif
  if(wait_maincontent==true || (contentOnly==false && ((!wait_outputs && current_data==2 && my_service->outputs==NULL) || my_service==NULL || my_service->name==NULL || my_service->content==NULL))){
    setMapInMaps(conf,"lenv","message",srlval.chaine);
#ifndef WIN32
    srlex_destroy();
#endif
    return -1;
  }
  else
    *service=my_service;

#ifndef WIN32
  srlex_destroy();
#endif
  return resultatYYParse;
}
