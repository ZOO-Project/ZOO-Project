%{
//======================================================
/**
   Zoo main configuration file parser
**/
//======================================================

#include <string>
#include <stdio.h>
#include <ctype.h>
#include <service.h>
#include <vector>

static int defaultsc=0;
static maps* my_maps=NULL;
static maps* current_maps=NULL;
static map* previous_content=NULL;
static map* current_content=NULL;
static elements* current_element=NULL;
static map* scontent=NULL;
static char* curr_key;
static int debug=0;
static int previous_data=0;
static int current_data=0;
using namespace std;

extern void crerror(const char *s);

void usage(void) ;

extern int crdebug;

extern char crtext[];

extern int crlineno;

extern FILE* crin;

extern int crlex(void);
extern int crlex_destroy(void);

%}



//======================================================
/* le type des lval des jetons et des elements non terminaux bison */
//======================================================
%union { char* s;char* chaine; char* key;char* val;}
//======================================================

// jetons //
//======================================================
/* les jetons que l on retrouve dans FLEX */
//======================================================
/* texte on a besoin de récupérer une valeur char* pour la comparer */
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
%token SLASH Eq CHARDATA ATTVALUE PAIR SPAIR EPAIR EPAIRS ANID
%type <chaine> PAIR
%type <chaine> EPAIRS
%type <chaine> EPAIR
%type <chaine> SPAIR

//======================================================
/* <!-- xxx -> <? xxx yyy ?> */
%token PI PIERROR /** COMMENT **/
//======================================================
/* <!-- xxx -> <? xxx yyy ?> */
%token ERREURGENERALE CDATA WHITESPACE NEWLINE
//======================================================
// non terminaux typés
//======================================================
/* elements non terminaux de type char *     */
/* uniquement ceux qui devrons etre comparés */
//======================================================
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
 : miscetoile PIERROR {crerror("processing instruction begining with <?xml ?> impossible\n");}
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
  /* les non terminaux rendent les valeurs de leur identifiants de balise */
  /* en char*, donc on peut comparer ces valeurs avec la fonction C++ strcmp(const char*;const char*) */
  /* de string */
  if (strcmp($1,$3) != 0)
    {
      crerror("Opening and ending tag mismatch");
      printf("\n  ::details : tag '%s' et '%s' \n",$1,$3);
      return 1;
      // on retourne different de 0
      // sinon yyparse rendra 0
      // et dans le main on croira a le fichier xml est valide !
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

#ifdef DEBUG
	printf("* Identifiant : %s\n",$2);
#endif
	
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
 : INFCAR ID Attributeetoile SLASH SUPCAR	{}
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
 | contentetoile PIERROR	          {crerror("processing instruction <?xml ?> impossible\n");}
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

pair: PAIR {curr_key=strdup($1);/*printf("START 0 PAIR FOUND !! \n [%s]\n",$1);*/}
| EPAIR {
  if(current_content==NULL) 
    current_content=createMap(curr_key,$1);
  else{ 
    addToMap(current_content,curr_key,$1);
  }
  if(debug){ 
    printf("EPAIR FOUND !! \n"); 
    printf("[%s=>%s]\n",curr_key,$1);
  }
  free(curr_key);
  }
| SPAIR  {curr_key=strdup($1);if(debug) printf("SPAIR FOUND !!\n"); }
 ;


processid
: ANID  {
   if(current_maps->name!=NULL){
     addMapToMap(&current_maps->content,current_content);
     freeMap(&current_content);
     free(current_content);
     current_maps->next=NULL;
     current_maps->next=(maps*)malloc(MAPS_SIZE);
     current_maps->next->name=strdup($1);
     current_maps->next->content=NULL;
     current_maps->next->next=NULL;
     current_maps=current_maps->next;
     current_content=current_maps->content;
   }
   else{
     current_maps->name=(char*)malloc((strlen($1)+1)*sizeof(char));
     snprintf(current_maps->name,(strlen($1)+1),"%s",$1);
     current_maps->content=NULL;
     current_maps->next=NULL;
     current_content=NULL;
   }
 }
 ;

%%

// crerror
//======================================================
/* fonction qui affiche l erreur si il y en a une */
//======================================================
void crerror(const char *s)
{
  if(debug)
    printf("\nligne %d : %s\n",crlineno,s);
}

// main
//======================================================
/* fonction principale : entrée dans le programme */
//======================================================
int conf_read(const char* file,maps* my_map){
  
  crin = fopen(file,"r");
  if (crin==NULL){
    printf("error : le fichier specifie n'existe pas ou n'est pas accessible en lecture\n") ;
    return 2 ;
  }

  my_maps=my_map;
  my_maps->name=NULL;
  current_maps=my_maps;
  
  int resultatYYParse = crparse() ;
  if(current_content!=NULL){
    addMapToMap(&current_maps->content,current_content);
    current_maps->next=NULL;
    freeMap(&current_content);
    free(current_content);
  }

  fclose(crin);
#ifndef WIN32
  crlex_destroy();
#endif

  return resultatYYParse;
}


//======================================================
// FIN //
//======================================================
