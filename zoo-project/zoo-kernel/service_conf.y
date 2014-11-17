%{
//======================================================
/**
 * Thx to Jean-Marie CODOL and Naitan GROLLEMUND
 * copyright 2009 GeoLabs SARL 
 * Author : Gérald FENOY
 *
 */
//======================================================

#include <string>
#include <stdio.h>
#include <ctype.h>
#include <service.h>

static int tmp_count=1;
static int defaultsc=0;
static bool wait_maincontent=true;
static bool wait_mainmetadata=false;
static bool wait_metadata=false;
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
// namespace
using namespace std;
//======================================================

// srerror
void srerror(const char *s);
//======================================================

// usage ()
void usage(void) ;
//======================================================

// srdebug
extern int srdebug;
//======================================================

extern char srtext[];

// srlineno
extern int srlineno;
//======================================================

// srin
extern FILE* srin;
//======================================================

// srlex
extern int srlex(void);
extern int srlex_destroy(void);

//vector<char*> lattribute;

%}



%union 
{char * s;char* chaine;char* key;char* val;}

// jetons //
%token <s> ID
%token <s> CHAINE
/* STARTXMLDECL et ENDXMLDECL qui sont <?xml et ?>*/
%token STARTXMLDECL ENDXMLDECL
//======================================================
/* version="xxx" et encoding="xxx" */
%token VERSIONDECL ENCODINGDECL SDDECL
//======================================================
/* < et > */
%token INFCAR SUPCAR 
//======================================================
/* / = a1  texte "texte" */
%token SLASH Eq CHARDATA ATTVALUE PAIR SPAIR EPAIR ANID
%type <chaine> PAIR
%type <chaine> EPAIR
%type <chaine> SPAIR
//======================================================
/* <!-- xxx -> <? xxx yyy ?> */
%token PI PIERROR /** COMMENT **/
//======================================================
/* <!-- xxx -> <? xxx yyy ?> */
%token ERREURGENERALE CDATA WHITESPACE NEWLINE
%type <s> STag
%type <s> ETag
%type <s> ANID
//======================================================
// %start
//======================================================

%%
// document <//===
//======================================================
// regle 1
// on est a la racine du fichier xml
//======================================================
document
 : miscetoile element miscetoile {}
 | contentetoile processid contentetoile document {}
 ;

miscetoile
 : miscetoile PIERROR {  srerror("processing instruction begining with <?xml ?> impossible\n");}
 | miscetoile PI {}
 | {}
 ;
// element
//======================================================
// regle 39
// OUVRANTE CONTENU FERMANTE obligatoirement
// ou neutre
// on ne peut pas avoir Epsilon
// un fichier xml ne peut pas etre vide ou seulement avec un prolog
//======================================================
element
 : STag contentetoile ETag	
{
}

// pour neutre
// on a rien a faire, meme pas renvoyer l identificateur de balise
// vu qu'il n y a pas de comparaison d'identificateurs avec un balise jumelle .
 | EmptyElemTag          {}
 ;

//======================================================
// STag
//======================================================
// regle 40
// BALISE OUVRANTE
// on est obligé de faire appel a infcar et supcar
// pour acceder aux start conditions DANSBALISE et INITIAL
//======================================================
STag
: INFCAR ID Attributeetoile SUPCAR
{
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"(%s %d) %s\n",__FILE__,__LINE__,$2);
  fflush(stderr);
#endif
  if(my_service->content==NULL){
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"NO CONTENT\n");
#endif
    addMapToMap(&my_service->content,current_content);
    freeMap(&current_content);
    free(current_content);
    current_content=NULL;
    my_service->metadata=NULL;
    wait_maincontent=false;
  }
  if(strncasecmp($2,"DataInputs",10)==0){
    if(wait_mainmetadata==true){
      addMapToMap(&my_service->metadata,current_content);
      freeMap(&current_content);
      free(current_content);
      current_content=NULL;
      wait_mainmetadata=false;
    }
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
      current_element=(elements*)malloc(ELEMENTS_SIZE);
      current_element->name=NULL;
      current_element->content=NULL;
      current_element->metadata=NULL;
      current_element->format=NULL;
      current_element->defaults=NULL;
      current_element->supported=NULL;
      current_element->next=NULL;
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
	current_element=(elements*)malloc(ELEMENTS_SIZE);
	current_element->name=NULL;
	current_element->content=NULL;
	current_element->metadata=NULL;
	current_element->format=NULL;
	current_element->defaults=NULL;
	current_element->supported=NULL;
	current_element->next=NULL;
      }
      wait_outputs=true;
      current_data=2;
      previous_data=2;
    }
    else
      if(strncasecmp($2,"MetaData",8)==0){
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
	current_content=NULL;
      }
      else
	if(strncasecmp($2,"ComplexData",11)==0 || strncasecmp($2,"LiteralData",10)==0
	   || strncasecmp($2,"ComplexOutput",13)==0 || strncasecmp($2,"LiteralOutput",12)==0
	   || strncasecmp($2,"BoundingBoxOutput",13)==0 || strncasecmp($2,"BoundingBoxData",12)==0){
	  current_data=4;
	  if(wait_metadata==true){
	    if(current_content!=NULL){
#ifdef DEBUG_SERVICE_CONF
	      fprintf(stderr,"add current_content to current_element->content\n");
	      fprintf(stderr,"LINE %d",__LINE__);
#endif
	      addMapToMap(&current_element->metadata,current_content);
	      current_element->next=NULL;
	      if($2!=NULL)
		current_element->format=zStrdup($2);
	      
	      current_element->defaults=NULL;
	      current_element->supported=NULL;
	      freeMap(&current_content);
	      free(current_content);
	    }
	  }else{ 
	    // No MainMetaData
	    addMapToMap(&current_element->content,current_content);
	    freeMap(&current_content);
	    free(current_content);
	    current_element->metadata=NULL;
	    current_element->next=NULL;
	    if($2!=NULL)
	      current_element->format=zStrdup($2);
	    current_element->defaults=NULL;
	    current_element->supported=NULL;
	  }
	  current_content=NULL;
	  wait_metadata=false;
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
  printf("* Identifiant : %s\n",$2);
  fflush(stdout);
#endif
}
 ;

//======================================================
// Attributeetoile
//======================================================
// regle 41
// une liste qui peut etre vide d'attributs
// utiliser la récursivité a gauche
//======================================================
Attributeetoile
 : Attributeetoile attribute  {}
 | 	                          {/* Epsilon */}
 ;

//======================================================
// attribute
//======================================================
// regle 41
// un attribut est compose d'un identifiant
// d'un "="
// et d'une définition de chaine de caractere
// ( "xxx" ou 'xxx' )
//======================================================
attribute
 : ID Eq ATTVALUE		
{
#ifdef DEBUG_SERVICE_CONF
  printf ("attribute : %s\n",$1) ;
#endif
}
 ;

//======================================================
// EmptyElemTag
//======================================================
// regle 44
// ICI ON DEFINIT NEUTRE
// on ne renvoie pas de char*
// parce qu'il n'y a pas de comparaisons a faire
// avec un identifiant d'une balise jumelle
//======================================================
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
     current_element->next=NULL;
   }
 }
 ;

//======================================================
// ETag
//======================================================
// regle 42
// BALISE FERMANTE
// les separateurs après ID sont filtrés
//======================================================
ETag
 : INFCAR SLASH ID SUPCAR
{
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"(%s %d)\n",__FILE__,__LINE__);
#endif
  if(strcmp($3,"DataInputs")==0){
    current_data=1;
  }
  if(strcmp($3,"DataOutputs")==0){
    current_data=2;
  }
  if(strcmp($3,"MetaData")==0){
    current_data=previous_data;
  }
  if(strcmp($3,"ComplexData")==0 || strcmp($3,"LiteralData")==0 
     || strcmp($3,"ComplexOutput")==0 || strcmp($3,"LiteralOutput")==0){
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

//======================================================
// texteinterbalise
//======================================================
// regle 14
// DU TEXTE quelconque
// c'est du CHARDATA
// il y a eut un probleme avec ID,
// on a mis des starts conditions,
// maintenant on croise les ID dans les dbalises
// et des CHARDATA hors des balises
//======================================================
texteinterbalise
 : CHARDATA		{}
 ;
//======================================================

pair: PAIR { if(debug) fprintf(stderr,"PAIR FOUND !!\n");if(curr_key!=NULL){free(curr_key);curr_key=NULL;} }
| EPAIR {
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"(%s %d)\n",__FILE__,__LINE__);
  fprintf(stderr,"EPAIR FOUND !! \n"); 
  fprintf(stderr,"[%s=>%s]\n",curr_key,$1);
  fprintf(stderr,"[ZOO: service_conf.y line %d free(%s)]\n",__LINE__,curr_key);
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
  fprintf(stderr,"(%s %d)\n",__FILE__,__LINE__);
#endif
  if(data==-1){
    data=1;
    if($1!=NULL){
      char *cen=zStrdup($1);
      my_service->name=(char*)malloc((strlen(cen)-1)*sizeof(char*));
      cen[strlen(cen)-1]=0;
      cen+=1;
      sprintf(my_service->name,"%s",cen);
      cen-=1;
      free(cen);
      my_service->content=NULL;
      my_service->metadata=NULL;
      my_service->inputs=NULL;
      my_service->outputs=NULL;
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
	  addToElements(&my_service->inputs,current_element);
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
	current_element=(elements*)malloc(ELEMENTS_SIZE);
	current_element->name=NULL;
	current_element->content=NULL;
	current_element->metadata=NULL;
	current_element->format=NULL;
	current_element->defaults=NULL;
	current_element->supported=NULL;
	current_element->next=NULL;
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
	  current_element->name=(char*)malloc((strlen(cen)-1)*sizeof(char*));
	  cen[strlen(cen)-1]=0;
	  cen+=1;
	  sprintf(current_element->name,"%s",cen);
	  cen-=1;
	  free(cen);
#ifdef DEBUG_SERVICE_CONF
	  fprintf(stderr,"NAME IN %s (current - %s)\n",$1,current_element->name);
#endif
	  current_element->content=NULL;
	  current_element->metadata=NULL;
	  current_element->format=NULL;
	  current_element->defaults=NULL;
	  current_element->supported=NULL;
	  current_element->next=NULL;
#ifdef DEBUG_SERVICE_CONF
	  fprintf(stderr,"NAME IN %s (current - %s)\n",$1,current_element->name);
#endif
	}
      }
    }
    else
      if(current_data==2){ 
	wait_outputs=true;
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
	    current_element=(elements*)malloc(ELEMENTS_SIZE);
	    current_element->name=NULL;
	    current_element->content=NULL;
	    current_element->metadata=NULL;
	    current_element->format=NULL;
	    current_element->defaults=NULL;
	    current_element->supported=NULL;
	    current_element->next=NULL;
	  }
	  if(current_element->name==NULL){
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"NAME OUT %s\n",$1);
	    fprintf(stderr,"(DATAOUTPUTS - %d) SET NAME OF current_element\n",__LINE__);
#endif
	    if($1!=NULL){ 
	      char *cen=zStrdup($1);
	      current_element->name=(char*)malloc((strlen(cen)-1)*sizeof(char));
	      cen[strlen(cen)-1]=0;
	      cen+=1;
	      sprintf(current_element->name,"%s",cen);
	      cen-=1;
	      free(cen);
	      current_element->content=NULL;
	      current_element->metadata=NULL;
	      current_element->format=NULL;
	      current_element->defaults=NULL;
	      current_element->supported=NULL;
	      current_element->next=NULL;
	    }
	  }

	  current_content=NULL;
	}
	else
	  if(current_element!=NULL && current_element->name!=NULL){
	    if(my_service->outputs==NULL)
	      my_service->outputs=dupElements(current_element);
	    else
	      addToElements(&my_service->outputs,current_element);
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"ADD TO OUTPUTS Elements\n");
	    dupElements(current_element);
#endif
	    freeElements(&current_element);
	    free(current_element);
	    current_element=NULL;
	    current_element=(elements*)malloc(ELEMENTS_SIZE);
	    char *cen=zStrdup($1);
	    current_element->name=(char*)malloc((strlen(cen)-1)*sizeof(char));
	    cen[strlen(cen)-1]=0;
	    cen+=1;
	    sprintf(current_element->name,"%s",cen);
	    cen-=1;
	    free(cen);
	    current_element->content=NULL;
	    current_element->metadata=NULL;
	    current_element->format=NULL;
	    current_element->defaults=NULL;
	    current_element->supported=NULL;
	    current_element->next=NULL;
	  }
	  else{
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"NAME OUT %s\n",$1);
	    fprintf(stderr,"(DATAOUTPUTS - %d) SET NAME OF current_element %s\n",__LINE__,$1);
#endif
	    if($1!=NULL){ 
	      char *cen=zStrdup($1);
	      current_element->name=(char*)malloc((strlen(cen))*sizeof(char*));
	      cen[strlen(cen)-1]=0;
#ifdef DEBUG
	      fprintf(stderr,"tmp %s\n",cen);
#endif
	      cen+=1;
	      sprintf(current_element->name,"%s",cen);
	      cen-=1;
	      free(cen);
	      current_element->content=NULL;
	      current_element->metadata=NULL;
	      current_element->format=NULL;
	      current_element->defaults=NULL;
	      current_element->supported=NULL;
	      current_element->next=NULL;
	    }
	  }
	wait_inputs=false;
	wait_outputs=true;
	//wait_outputs=true;
      }
  }
 }
 ;

%%

// srerror
//======================================================
/* fonction qui affiche l erreur si il y en a une */
//======================================================
void srerror(const char *s)
{
  if(debug)
    fprintf(stderr,"\nligne %d : %s\n",srlineno,s);
}

/**
 * getServiceFromFile :
 * set service given as second parameter with informations extracted from the
 * definition file.
 */
int getServiceFromFile(maps* conf,const char* file,service** service){
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

  wait_maincontent=true;
  wait_mainmetadata=false;
  wait_metadata=false;
  wait_inputs=false;
  wait_defaults=false;
  wait_supporteds=false;
  wait_outputs=false;
  wait_data=false;
  data=-1;
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
#endif
  if(wait_outputs && current_element!=NULL && current_element->name!=NULL){
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
  if(current_content!=NULL){
    freeMap(&current_content);
    free(current_content);
    current_content=NULL;
  }
  fclose(srin);
#ifdef DEBUG_SERVICE_CONF
  dumpService(my_service);
#endif
  if(wait_outputs<0 || my_service==NULL || my_service->name==NULL || my_service->content==NULL || my_service->outputs==NULL){
    fprintf(stderr,"%s %d\n",__FILE__,__LINE__);
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
