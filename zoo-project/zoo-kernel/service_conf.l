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
%option noyywrap
%option yylineno


%{

#include <string.h>
#include "service_conf.tab.h"

char *lmsg=NULL;

%}

S		[ \t\r\n]+

CharRef		"&#"[0-9]+";"|"&#x"[0-9a-fA-F]+";"

egalevolue		{S}?"="{S}?

Name		([_:]|[\x41-\x5A]|[\x61-\x7A]|[\xC0-\xD6]|[\xD8-\xF6]|[\xF8-\xFF])(([\x41-\x5A]|[\x61-\x7A]|[\xC0-\xD6]|[\xD8-\xF6]|[\xF8-\xFF])|[0-9.\-_:])*

chardata	[^<]*

attname	[a-zA-Z0-9._\-]+
attvalue1	[\x80-\xbf\xc2-\xdf\xe0-\xef\xf0-\xf4#°²θé\^\*\+\,\;?!~`Ãº@a-zA-Z0-9%_\-:\:.:" "\"\'/\\\(\)\t\|\$\&>\[\]]+

attvalue		\"[^"]*\"|\'[^']*\'\(\)

whitespace                      [\t]{0,}|[ ]{0,}
whitesp                      [\t]|[ ]
newline                 [\r\n]|[\n]
newlines                 [\r\n]{1,}|[\n]{1,}


%x DANSBALISE HORSBALISE PAIRSTART




%%

"\n" { }

{newline}+{whitesp}*			{ return NEWLINE; }

<INITIAL,HORSBALISE>"["{attname}"]"             {  srlval.chaine=yytext;return ANID; }

<INITIAL,HORSBALISE>{attname}             {  srlval.chaine=yytext; return SPAIR; }

<PAIRSTART,HORSBALISE>{attvalue1}             { srlval.chaine=yytext;/*BEGIN(INITIAL);*/ return EPAIR;}

<PAIRSTART,INITIAL,HORSBALISE>{whitesp}*"="{whitesp}*             {  BEGIN(PAIRSTART);}

<PAIRSTART,INITIAL,HORSBALISE>{newline}+{whitesp}*             { BEGIN(INITIAL);  return NEWLINE;}

<DANSBALISE,INITIAL,HORSBALISE>{S}   {  }

<INITIAL,HORSBALISE>{newline}*"<"		    { BEGIN(DANSBALISE); return INFCAR;}


<DANSBALISE>">"			{ BEGIN(HORSBALISE);return SUPCAR;}


<DANSBALISE>"/"		{return SLASH;}


<DANSBALISE>{egalevolue}			{return Eq;}


<DANSBALISE>{Name}{newline}*			{memmove(srlval.chaine,yytext,(strlen(yytext)+1)*sizeof(char));return ID;}


<DANSBALISE>{attvalue}		{return ATTVALUE;}


<INITIAL,DANSBALISE,HORSBALISE>.|\n	{lmsg=(char*)malloc(1024*sizeof(char));sprintf(lmsg,"error: line %d: character not allowed '%s'",srlineno,yytext);fprintf(stderr,"%s \n",lmsg);srlval.chaine=lmsg;fprintf(stderr,"%s \n",lmsg);return -1;}

%%

