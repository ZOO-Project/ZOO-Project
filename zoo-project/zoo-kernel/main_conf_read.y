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
%{
#include <service.h>

static maps* my_maps=NULL;
static maps* current_maps=NULL;
static map* current_content=NULL;
static char* curr_key;
static int debug=0;

extern void crerror(const char *s);

void usage(void) ;

extern int crdebug;

extern char crtext[];

extern int crlineno;

extern FILE* crin;

extern int crlex(void);
extern int crlex_destroy(void);

%}

%union { char* s;char* chaine; char* key;char* val;}

%token <s> ID
%token <s> CHAINE

%token PAIR SPAIR EPAIR EPAIRS ANID
%type <chaine> PAIR
%type <chaine> EPAIRS
%type <chaine> EPAIR
%type <chaine> SPAIR

%token WHITESPACE NEWLINE

%type <s> ANID

%%

document
 : miscetoile miscetoile {}
 | contentetoile processid contentetoile document {}
 ;

miscetoile
 : {}
 ;

Attributeetoile
 : Attributeetoile  {}
 | 	                          {/* Epsilon */}
 ;

contentetoile
: contentetoile NEWLINE {}
 | contentetoile pair {}
 | {/* Epsilon */}
 ;

pair: PAIR {if(curr_key!=NULL) free(curr_key);curr_key=zStrdup($1);}
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
  curr_key=NULL;
  }
| SPAIR  {if(curr_key!=NULL) free(curr_key);curr_key=zStrdup($1);if(debug) printf("SPAIR FOUND !!\n"); }
 ;


processid
: ANID  {
   if(current_maps->name!=NULL){
     addMapToMap(&current_maps->content,current_content);
     freeMap(&current_content);
     free(current_content);
     current_maps->next=NULL;
     current_maps->next=createMaps($1);
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

/**
 * Print on stderr the message and the line number of the error which occurred.
 *
 * @param s the error message
 */
void crerror(const char *s)
{
  if(debug)
    printf("\nligne %d : %s\n",crlineno,s);
}

/**
 * Parse the main.cfg file and fill the maps structure.
 *
 * @param file the filename to parse
 * @param my_map the maps structure to fill
 */
int conf_read(const char* file,maps* my_map){
  
  crin = fopen(file,"r");
  if (crin==NULL){
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
  if(curr_key!=NULL){
    free(curr_key);
  }

  fclose(crin);
#ifndef WIN32
  crlex_destroy();
#endif

  return resultatYYParse;
}

