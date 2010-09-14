/* pour pouvoir acceder au numero de ligne dans bison */
%option noyywrap
%option yylineno


%{
//======================================================
/**

 fichier flex de verification xml
 ter d'analyse syntaxique 2006-2007
 ==================================
 
 auteurs:
 Jean-Marie CODOL
   &
 Naitan GROLLEMUND




======
 ce fichier est li�a 3 autres fichiers :
 parser.y
 makefile
 README
 
 pour compiler ce projet :
 	  linux : taper $make all
 	  windows : ouvrir avec dev-c++, configurer le makefile du projet et compiler
	            naviguer avec la ligne de commande jusqu'au repertoire.
======


 pour plus d'informations, voir le README


**/
//======================================================


//======================================================
/* pour avoir acces a strdup() */
/* en effet, il faut garder une trace de la valeur */
/* car yytext va varier et on ne pourra plus reconnaitre les */
/* identifiants si on ne copie pas ces valeurs */
#include <string.h>

//======================================================
/* pour la liaison avec bison */
#include "service_conf.tab.h"

//======================================================
/* afficher ou pas une trace : */
#ifdef DEBUG_SERVICE_CONF
int affichetrace = 1;
#else
int affichetrace = 0;
#endif
//======================================================


//======================================================
/* on a l interdiction de mettre un commentaire avant une declaration : */
int attentionImpossibleDeTrouverXMLDeclapres = 0 ;
//======================================================

//======================================================
/* on a l interdiction de mettre un pi avant une declaration : */
int attentionImpossibleDeTrouverPIapres = 0 ;
//======================================================

%}


/*====================================================*/
/*====================================================*/
/* Les Separateurs xml */
/*====================================================*/
S		[ \t\r\n]+
/*====================================================*/




/*====================================================*/
/* CharRef regle 66 */
/*====================================================*/
CharRef		"&#"[0-9]+";"|"&#x"[0-9a-fA-F]+";"
/*====================================================*/




/*====================================================*/
/*   espaces? '=' espaces?    regle 25  */
/* si on veut pouvoir utiliser S dans bison, */
/* il faut cr�r au moins une autre start condition */
/* comme on n'utilise "egalevolue" que pour version et encoding */
/* il est pr��able de rajouter cette macro */
/*====================================================*/
egalevolue		{S}?"="{S}?
/*====================================================*/




/*====================================================*/
/* Name regle 5 */
/*====================================================*/
/*** Name		[a-zA-Z_:][a-zA-Z0-9.\-_:]* ***/
Name		([_:]|[\x41-\x5A]|[\x61-\x7A]|[\xC0-\xD6]|[\xD8-\xF6]|[\xF8-\xFF])(([\x41-\x5A]|[\x61-\x7A]|[\xC0-\xD6]|[\xD8-\xF6]|[\xF8-\xFF])|[0-9.\-_:])*
/*====================================================*/





/*====================================================*/
/* chardata  regle 14 */
/*====================================================*/
/**chardata	[a-zA-Z0-9_\-.:" "\"\'\\]***/
chardata	[^<]*
/*====================================================*/
attname	[a-zA-Z0-9_\-]+
attvalue1	[\*\+,;@a-zA-Z0-9_\-::.:" "\"\'/\\\(\)]+





/*====================================================*/
/* attvalue regle 10 */
/*====================================================*/
/* attvalue		\"([^"&]|{CharRef})*\"|\'([^'&]|{CharRef})*\' */
/* pas tr� classe mais ca marche . */
attvalue		\"[^"]*\"|\'[^']*\'\(\)
/*====================================================*/

whitespace                      [ ]{0,}
whitesp                      [ ]
newline                 [\r\n]|[\n]
newlines                 [\r\n]{1,}|[\n]{1,}


/*====================================================*/
/* initial = de debut a ?> du prolog ; DANSBALISE = dans une balise ; HORSBALISE = hors d'une balise */
/*====================================================*/
%x DANSBALISE HORSBALISE PAIRSTART
/*====================================================*/




%%

"\n" {  if (affichetrace==1) fprintf (stderr,"\n\nNEWLINE\n") ;return NEWLINE;}

{newline}+{whitesp}*			{  if (affichetrace==1) fprintf (stderr,"\n\nNEWLINE 1\n") ; return NEWLINE;}

<INITIAL,HORSBALISE>"["{attname}"]"             {  srlval.chaine=yytext;return ANID; }

<INITIAL,HORSBALISE>{attname}             {  srlval.chaine=yytext; return SPAIR; }

<PAIRSTART,HORSBALISE>{attvalue1}             { srlval.chaine=yytext;/*BEGIN(INITIAL);*/ return EPAIR;}

<PAIRSTART,INITIAL,HORSBALISE>{whitesp}*"="{whitesp}*             {  BEGIN(PAIRSTART);}

<PAIRSTART,INITIAL,HORSBALISE,DANSBALISE>{newline}+{whitesp}*             { BEGIN(INITIAL);  return NEWLINE;}

<INITIAL>"<?"[Xx][Mm][Ll]  {   if (attentionImpossibleDeTrouverXMLDeclapres == 1 || attentionImpossibleDeTrouverPIapres == 1) {/* il y a eut un commentaire ou une balise applicative avant la declaration xml */ fprintf(stderr,"\nerror : a la ligne %d : il y a eut un commentaire ou un PI avant la declaration xml\n",srlineno); exit (10) ; } ; return STARTXMLDECL;}

<INITIAL>"version"{egalevolue}\"1.0\"|"version"{egalevolue}\'1.0\'  { return VERSIONDECL;  }
<INITIAL>"version"{egalevolue}\"[^"]*\"|"version"{egalevolue}\'[^']*\'  {/* erreur de version encoding */ fprintf(stderr,"\nerror : a la ligne %d : la version xml n est pas reconnue : %s\n",srlineno,yytext); exit (9) ; }


<INITIAL>"encoding"{egalevolue}\"[Ii][Ss][Oo]"-8859-1"\"|"encoding"{egalevolue}\'[Ii][Ss][Oo]"-8859-1"\'  { return ENCODINGDECL;}
<INITIAL>"encoding"{egalevolue}\"[^"]*\"|"encoding"{egalevolue}\'[^']*\'  {/* erreur de version encoding */ 	fprintf(stderr,"\nerror : a la ligne %d : la version d encodage n est pas reconnue : %s\n",srlineno,yytext); exit (8) ; }


<INITIAL>"standalone"{egalevolue}\"yes\"|"standalone"{egalevolue}\'yes\'|"standalone"{egalevolue}\"no\"|"standalone"{egalevolue}\'no\'  { return SDDECL;}

<INITIAL>"standalone"{egalevolue}\"[^"]*\"|"standalone"{egalevolue}\'[^']*\'|"standalone"{egalevolue}\"[^"]*\"|"standalone"{egalevolue}\'[^']*\'  { /* erreur de version encoding */ 	fprintf(stderr,"\nerror : a la ligne %d : la version standalone n est pas reconnue : %s\n",srlineno,yytext); exit (7) ; }


<INITIAL>"?>"  { BEGIN(HORSBALISE); return ENDXMLDECL;}


<DANSBALISE,INITIAL,HORSBALISE>{S}   {  }


<HORSBALISE>"<?"[Xx][Mm][Ll]{S}({S}|{chardata})*"?>"|"<?"[Xx][Mm][Ll]"?>"	{ return PIERROR;}
<INITIAL,HORSBALISE>"<?"([^xX]|([xX][^mM])|([xX][mM][^lL]))({S}|([^?]|("?"[^>])))*"?>"		{ attentionImpossibleDeTrouverPIapres=1 ; return PI;}


<INITIAL,HORSBALISE>{newline}*"<"		    { BEGIN(DANSBALISE); return INFCAR;}


<DANSBALISE>">"			{ BEGIN(HORSBALISE);return SUPCAR;}


<DANSBALISE>"/"		{return SLASH;}


<DANSBALISE>{egalevolue}			{return Eq;}


<DANSBALISE>{Name}			{srlval.chaine=strdup(yytext);return ID;}


<DANSBALISE>{attvalue}		{return ATTVALUE;}


<INITIAL,HORSBALISE>"<!--"([^-]|"-"[^-])*"-->"		{attentionImpossibleDeTrouverXMLDeclapres=1; }


<INITIAL,DANSBALISE,HORSBALISE>.|\n	{fprintf(stderr,"error : ligne %d : caractere non reconnu '%s'\n",srlineno,yytext);}

%%

