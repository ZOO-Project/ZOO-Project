/*
 * Zoo main configuration file parser
 *
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
%option yylineno

%{

#include <string.h>
#include "main_conf_read.tab.h"
static int affichetrace = 0 ;

%}

attname	[a-zA-Z0-9_\-:./\{\}-]+

attvalue1	[%\*,;@a-zA-Z0-9_\-.:" "\"\'/\\\(\)\+\x41-\xff?&=\=\!$-]+

whitesp                      [ ]
newline                 [\r\n]|[\n]

%x DANSBALISE HORSBALISE PAIRSTART

%%

"\n" {if (affichetrace==1) printf ("\n\nNEWLINE\n") ; return NEWLINE;}

{newline}+{whitesp}*			{if (affichetrace==1) printf ("\n\nNEWLINE 1\n") ; return NEWLINE;}

<INITIAL,HORSBALISE>"["{attname}"]"             {if (affichetrace==1) printf ("\n\nANID:%s\n",yytext); crlval.chaine=yytext;crlval.chaine[strlen(crlval.chaine)-1]=0;crlval.chaine+=1;return ANID; }

<INITIAL,HORSBALISE>{attname}             {if (affichetrace==1) printf ("\n\nATT_NAME:%s\n",yytext); crlval.chaine=yytext; return SPAIR; }

<PAIRSTART,HORSBALISE>{attvalue1}             {if (affichetrace==1) printf ("\n\nATT_VALUE:%s\n",yytext);crlval.chaine=yytext;BEGIN(INITIAL);return EPAIR;}

<PAIRSTART,INITIAL,HORSBALISE>{whitesp}*"="{whitesp}*             {BEGIN(PAIRSTART);}

<PAIRSTART,INITIAL,HORSBALISE,DANSBALISE>{newline}+{whitesp}*			{if (affichetrace==1) printf ("\n\nNEWLINE 2\n") ; BEGIN(INITIAL); return NEWLINE;}

%%


int crwrap()
{return 1;}
