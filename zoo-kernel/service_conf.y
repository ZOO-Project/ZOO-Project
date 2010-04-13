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
#include <vector>

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
static int services_c=0;
static service* my_service=NULL;
static map* previous_content=NULL;
static map* current_content=NULL;
static elements* current_element=NULL;
static map* scontent=NULL;
static char* curr_key;
static int debug=0;
static int data=-1;
static int previous_data=0;
static int current_data=0;
static char* myFinalObjectAsJSON="{";
// namespace
using namespace std;
//======================================================

// srerror
void srerror(char *s);
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

vector<char*> lattribute;

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
  if (strcmp($1,$3) != 0)
    {
      //srerror("Opening and ending tag mismatch");
      fprintf(stderr,"Opening and ending tag mismatch\n  ::details : tag '%s' et '%s' \n",$1,$3);
      lattribute.clear();
      //return 1;
    }
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
  /* l'astuce consiste a vider le contenu du vector ici !! */
  /* parce que cet element est reconnu AVANT la balise fermante */
  /* et APRES l'analyse des eventuelles balises internes ou successeur */
  lattribute.clear();
	
  if(my_service->content==NULL){
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"NO CONTENT\n");
#endif
    //addMapToMap(&my_service->content,current_content);
    //freeMap(&current_content);
    my_service->content=current_content;
    current_content=NULL;
    my_service->metadata=NULL;
    wait_maincontent=false;
  }

  if(strcmp($2,"DataInputs")==0){
    if(wait_mainmetadata==true){
      if(my_service->metadata==NULL)
	my_service->metadata=current_content;
      else{
	addMapToMap(&my_service->metadata,current_content);
	freeMap(&current_content);
      }
      current_content=NULL;
    }
    if(current_element==NULL){
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"(DATAINPUTS - 184) FREE current_element\n");
#endif
      freeElements(&current_element);
      free(current_element);
      current_element=NULL;
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"(DATAINPUTS - 186) ALLOCATE current_element\n");
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
    if(strcmp($2,"DataOutputs")==0){
      if(wait_inputs==true){
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"(DATAOUTPUTS) DUP INPUTS current_element\n");
#endif
	if(my_service->inputs==NULL)
	  my_service->inputs=dupElements(current_element);
	else
	  addToElements(my_service->inputs,current_element);
#ifdef DEBUG_SERVICE_CONF
	dumpElements(current_element);
	dumpElements(my_service->inputs);
	fprintf(stderr,"(DATAOUTPUTS) FREE current_element\n");
#endif
	freeElements(&current_element);
	free(current_element);
	current_element=NULL;
	wait_inputs=false;
      }
      if(current_element==NULL){
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"(DATAOUTPUTS - 206) ALLOCATE current_element (%s)\n",$2);
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
      previous_data=1;
    }
    else
      if(strcmp($2,"MetaData")==0){
	current_data=3;
	if(current_element!=NULL){
	  wait_metadata=true;
#ifdef DEBUG_SERVICE_CONF
	  fprintf(stderr,"add current_content to current_element->content\n");
	  fprintf(stderr,"LINE 247");
#endif
	  addMapToMap(&current_element->content,current_content);
	  freeMap(&current_content);
	  free(current_content);
	}
	else{
	  wait_mainmetadata=true;
	}
	current_content=NULL;
      }
      else
	if(strcmp($2,"ComplexData")==0 || strcmp($2,"LiteralData")==0
	|| strcmp($2,"ComplexOutput")==0 || strcmp($2,"LiteralOutput")==0){
	  current_data=4;
	  if(wait_metadata==true){
	    if(current_content!=NULL){
	      current_element->metadata=current_content;
	      current_element->next=NULL;
	      current_element->format=$2;
	      current_element->defaults=NULL;
	    }
	  }else{ // No MainMetaData
	    //addMapToMap(&current_element->content,current_content);
	    //freeMap(&current_content);
	    //free(current_content);
	    current_element->content=current_content;
	    current_element->metadata=NULL;
	    current_element->next=NULL;
	    current_element->format=$2;
	    current_element->defaults=NULL;
	  }
	  current_content=NULL;
	  wait_metadata=false;
	}
	else
	  if(strcmp($2,"Default")==0){
	    wait_defaults=true;
	    current_data=5;
	  }
	  else
	    if(strcmp($2,"Supported")==0){
	      wait_supporteds=true;
	      if(wait_defaults==true){
		defaultsc++;
		freeMap(&current_content);
		current_content=NULL;
	      }
	      current_data=5;
	    }
#ifdef DEBUG_SERVICE_CONF
  printf("* Identifiant : %s\n",$2);
#endif
		
  /* et on renvoie l'identifiant de balise afin de pouvoir le comparer */
  /* avec la balise jumelle fermante ! */
  $$ = $2 ;
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
	// on verifie que les attributst ne sont pas en double
	// sinon on ajoute au vector
#ifdef DEBUG_SERVICE_CONF
  printf ("attribute : %s\n",$1) ;
#endif
  for(int i=0;i < lattribute.size(); i++)
    {
      if (strcmp($1,lattribute.at(i)) == 0)
	{
	  fprintf (stderr,"attributs identiques : %d -- %s , %s",i,lattribute.at(i),$1) ;
	  //srerror("attribut redondant !:");
	}
    }
  lattribute.push_back($1);
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
 : INFCAR ID Attributeetoile SLASH SUPCAR	{lattribute.clear();/* voir Stag */}
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
      current_element->defaults=(iotype*)malloc(MAP_SIZE);
    }
    current_element->defaults->content=current_content;
    current_element->defaults->next=NULL;
    wait_defaults=false;
    current_content=NULL;
    current_element->supported=NULL;
  }
  if(strcmp($3,"Supported")==0){
    current_data=previous_data;
    if(current_element->supported==NULL){
      current_element->supported=(iotype*)malloc(MAP_SIZE);
      current_element->supported->content=current_content;
      current_element->supported->next=NULL;
      /**
       * Need to free this ressource (HERE ?)
      free(current_content);
      */
    }
    else{ 
      /*current_element->supported->next=(iotype*)malloc(sizeof(iotype*));
      current_element->supported->next->content=NULL;
      current_element->supported->next->next=NULL;
      iotype* tmp1=current_element->supported;
      while(tmp1!=NULL){
	addMapToMap(&current_element->supported->next->content,current_content);
	freeMap(&current_content);
#ifdef DEBUG_SERVICE_CONF
	fprintf(stderr,"LINE 409");
#endif
	free(current_content);
	current_content=NULL;
	tmp1=tmp1->next;
	}*/
    }
    current_content=NULL;
  }
  /* on renvoie l'identifiant de la balise pour pouvoir comparer les 2 */
  /* /!\ une balise fermante n'a pas d'attributs (c.f. : W3C) */
  $$ = $3;
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

pair: PAIR {  if(debug) fprintf(stderr,"PAIR FOUND !!\n"); }
| EPAIR {
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"EPAIR FOUND !! \n"); 
    fprintf(stderr,"[%s=>%s]\n",curr_key,$1);
    fprintf(stderr,"[ZOO: service_conf.y line 482 free(%s)]\n",curr_key);
    dumpMap(current_content);
    fflush(stderr);
#endif
  if(current_content==NULL){
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"[ZOO: service_conf.y line 482 free(%s)]\n",curr_key);
#endif
    current_content=createMap(curr_key,$1);
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"[ZOO: service_conf.y line 482 free(%s)]\n",curr_key);
#endif
    //current_content->next=NULL;
  }
  else{ 
#ifdef DEBUG_SERVICE_CONF
    dumpMap(current_content);
    fprintf(stderr,"addToMap(current_content,%s,%s) !! \n",curr_key,$1); 
    
#endif
    //map* tmp1=createMap(curr_key,$1);
    addToMap(current_content,curr_key,$1);
    //freeMap(&tmp1);
    //free(tmp1);
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"addToMap(current_content,%s,%s) end !! \n",curr_key,$1); 
#endif    
  }
  //free(curr_key);
  curr_key=NULL;
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"EPAIR FOUND !! \n"); 
  fprintf(stderr,"[%s=>%s]\n",curr_key,$1);
  fprintf(stderr,"[ZOO: service_conf.y line 505 free(%s)]\n",curr_key);
  fflush(stderr);
#endif
  }
| SPAIR  { curr_key=$1; if(debug) fprintf(stderr,"SPAIR FOUND !!\n"); }
 ;


processid
: ANID  {
  if(data==-1){
    data=1;
    my_service->name=$1;
    my_service->content=NULL;
    my_service->metadata=NULL;
    my_service->inputs=NULL;
    my_service->outputs=NULL;
  } else {
    if(current_data==1){
      if(my_service->content!=NULL && current_element->name!=NULL){
	//fprintf(stderr,"ELEMENT (%s)",current_element->name);
	if(my_service->inputs==NULL){
#ifdef DEBUG_SERVICE_CONF
	  fprintf(stderr,"count (%i) (%s)\n",tmp_count%2,$1);
	  fflush(stderr);
#endif
	  //if(tmp_count==1){
#ifdef DEBUG_SERVICE_CONF
	  fprintf(stderr,"(DATAINPUTS - 464)DUP current_element\n");
	  dumpElements(current_element);
#endif
	  my_service->inputs=dupElements(current_element);
#ifdef DEBUG_SERVICE_CONF
	  fprintf(stderr,"(DATAINPUTS - 466)FREE current_element\n");
#endif
	  freeElements(&current_element);
	  current_element=NULL;
	  tmp_count++;
	}
	else{
	  addToElements(my_service->inputs,current_element);
#ifdef DEBUG_SERVICE_CONF
	  fprintf(stderr,"(DATAINPUTS - 6)FREE current_element (after adding to allread existing inputs)");
#endif
	  freeElements(&current_element);
	}
#ifdef DEBUG_SERVICE_CONF
	dumpElements(my_service->inputs);
#endif
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
	current_element->name=strdup($1);
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
    else
      if(current_data==2){ 
	if(wait_inputs==true){
	  fprintf(stderr,"dup INPUTS\n");
	  if(current_element->name!=NULL){
	    if(my_service->inputs==NULL){
	      my_service->inputs=dupElements(current_element);
	    }
	    else{
#ifdef DEBUG_SERVICE_CONF
	      fprintf(stderr,"LAST NAME IN %s (current - %s)\n",$1,current_element->name);
#endif
	      addToElements(my_service->inputs,current_element);
	    }
#ifdef DEBUG_SERVICE_CONF
	    dumpElements(current_element);
	    fprintf(stderr,"(DATAOUTPUTS - 531) FREE current_element\n");
#endif
	    freeElements(&current_element);
	    free(&current_element);
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"free OUTPUTS\n");
#endif
	    current_element=NULL;
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"(DATAOUTPUTS - 536) ALLOCATE current_element\n");
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
	    fprintf(stderr,"(DATAOUTPUTS - 545) SET NAME OF current_element\n");
#endif
	    current_element->name=strdup($1);
	    current_element->content=NULL;
	    current_element->metadata=NULL;
	    current_element->format=NULL;
	    current_element->defaults=NULL;
	    current_element->supported=NULL;
	    current_element->next=NULL;
	  }
	  wait_inputs=false;
	  current_content=NULL;
	}
	else
	  if(current_element->name==NULL){
#ifdef DEBUG_SERVICE_CONF
	    fprintf(stderr,"NAME OUT %s\n",$1);
	    fprintf(stderr,"(DATAOUTPUTS - 545) SET NAME OF current_element\n");
#endif
	    current_element->name=strdup($1);
	    current_element->content=NULL;
	    current_element->metadata=NULL;
	    current_element->format=NULL;
	    current_element->defaults=NULL;
	    current_element->supported=NULL;
	    current_element->next=NULL;
	  }
	wait_outputs=true;
      }
  }
 }
 ;

%%

// srerror
//======================================================
/* fonction qui affiche l erreur si il y en a une */
//======================================================
void srerror(char *s)
{
  if(debug)
    fprintf(stderr,"\nligne %d : %s\n",srlineno,s);
}

/**
 * getServiceFromFile :
 * set service given as second parameter with informations extracted from the
 * definition file.
 */
int getServiceFromFile(char* file,service** service){

  freeMap(&previous_content);
  previous_content=NULL;
  freeMap(&current_content);
  current_content=NULL;
  freeMap(&scontent);
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"(STARTING)FREE current_element\n");
#endif
  freeElements(&current_element);
  current_element=NULL;
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"(STARTING)FREE my_service\n");
#endif
  //freeService(&my_service);
  //free(my_service);
#ifdef DEBUG_SERVICE_CONF
  fprintf(stderr,"(STARTING)FREE my_service done\n");
#endif
  my_service=NULL;
  scontent=NULL;

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
    fprintf(stderr,"error : le fichier specifie n'existe pas ou n'est pas accessible en lecture\n") ;
    return 22;
  }

  //printf(" ");
  int resultatYYParse = srparse() ;
  
  if(wait_outputs==true && current_element->name!=NULL){
    if(my_service->outputs==NULL){      
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"(DATAOUTPUTS - 623) DUP current_element\n");
#endif
      my_service->outputs=dupElements(current_element);
      current_element=NULL;
    }
    else{
#ifdef DEBUG_SERVICE_CONF
      fprintf(stderr,"(DATAOUTPUTS - 628) COPY current_element\n");
#endif
      addToElements(my_service->outputs,current_element);
    }
#ifdef DEBUG_SERVICE_CONF
    fprintf(stderr,"(DATAOUTPUTS - 631) FREE current_element\n");
#endif
    freeElements(&current_element);
  }
  if(current_element!=NULL){
    freeElements(&current_element);
    fprintf(stderr,"LINE 709");
    //free(current_element);
    current_element=NULL;
  }
  if(current_content!=NULL){
    freeMap(&current_content);
    fprintf(stderr,"LINE 715");
    //free(current_content);
    current_content=NULL;
  }
  fclose(srin);
#ifdef DEBUG_SERVICE_CONF
  dumpService(my_service);
#endif
  *service=my_service;

  return resultatYYParse;
}
