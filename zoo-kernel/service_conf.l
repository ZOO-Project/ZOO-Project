%option noyywrap
%option yylineno


%{
//======================================================
/**

 authors : Jean-Marie CODOL, Naitan GROLLEMUND

**/
//======================================================


#include <string.h>
#include "service_conf.tab.h"

#ifdef DEBUG_SERVICE_CONF
int affichetrace = 1;
#else
int affichetrace = 0;
#endif


int attentionImpossibleDeTrouverXMLDeclapres = 0 ;

int attentionImpossibleDeTrouverPIapres = 0 ;

%}


S		[ \t\r\n]+

CharRef		"&#"[0-9]+";"|"&#x"[0-9a-fA-F]+";"

egalevolue		{S}?"="{S}?

Name		([_:]|[\x41-\x5A]|[\x61-\x7A]|[\xC0-\xD6]|[\xD8-\xF6]|[\xF8-\xFF])(([\x41-\x5A]|[\x61-\x7A]|[\xC0-\xD6]|[\xD8-\xF6]|[\xF8-\xFF])|[0-9.\-_:])*

chardata	[^<]*

attname	[a-zA-Z0-9_\-]+
attvalue1	[\^\*\+,;@a-zA-Z0-9_\-::.:" "\"\'/\\\(\)\t\|]+

attvalue		\"[^"]*\"|\'[^']*\'\(\)

whitespace                      [\t]{0,}|[ ]{0,}
whitesp                      [\t]|[ ]
newline                 [\r\n]|[\n]
newlines                 [\r\n]{1,}|[\n]{1,}


%x DANSBALISE HORSBALISE PAIRSTART




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


<DANSBALISE>{Name}{newline}*			{memmove(srlval.chaine,yytext,(strlen(yytext)+1)*sizeof(char));return ID;}


<DANSBALISE>{attvalue}		{return ATTVALUE;}


<INITIAL,HORSBALISE>"<!--"([^-]|"-"[^-])*"-->"		{attentionImpossibleDeTrouverXMLDeclapres=1; }


<INITIAL,DANSBALISE,HORSBALISE>.|\n	{fprintf(stderr,"error : ligne %d : caractere non reconnu '%s'\n",srlineno,yytext);}

%%

