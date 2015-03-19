/**
 * Author : David Saggiorato
 *
 *  Copyright 2008-2009 GeoLabs SARL. All rights reserved.
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



#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>

MYSQL *con;
char * host;
char *user;
char *passwd;
char *bdd;
int port;


void init_sql(const char* h,const char *u, const char *pas, const char * b,int p){
    host = strdup(h);
    user = strdup(u);
    passwd = strdup(pas);
    bdd = strdup(b);
    port=p;
}


int init_connection(){
    con = mysql_init(NULL);
    if (con == NULL){
        /* erreur allocation et initialisation mysql */
        return -1;
    }
    if (mysql_real_connect(con,host,user,passwd,bdd,port,NULL,0) == NULL){
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        return -2;
    }
    return 0;
}


char * get_uuid(){
    init_connection();
    char * query = "Select uuid();";
    if (mysql_query(con, query) != 0){
        fprintf(stderr,"%s\n", mysql_error(con));
        return NULL;
    }
    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL){
        fprintf(stderr,"get_uuid: no uuid\n");
        return NULL;
    }
    int num_fields = mysql_num_fields(result);
    if (num_fields != 1)
        return NULL;
    MYSQL_ROW row = mysql_fetch_row(result);
    char * tmp = strdup(row[0]);
    mysql_free_result(result);
    mysql_close(con);
    return tmp;
 }

int add_job(const char * uuid) {
    init_connection();
    const char * query = "insert into status (uuid,status,created_time) values ('%s','queue',now())";
    char * query_f = (char*) malloc(strlen(query) + strlen(uuid) + 1);
    sprintf(query_f,query,uuid);
    if (mysql_query(con, query_f) != 0){
        fprintf(stderr, "%s\n", mysql_error(con));
        free(query_f);
        return -1;
    }
    free(query_f);
    mysql_close(con);
    return 0;
}

int start_job(const char *uuid){
    init_connection();
    const char * query = "update status set start_date=now(), status='running', progress=0 where uuid='%s';";
    char * query_f = (char*) malloc(strlen(query) + strlen(uuid) + 1);
    sprintf(query_f,query,uuid);
    if (mysql_query(con, query_f) != 0){
        fprintf(stderr, "%s\n", mysql_error(con));
        free(query_f);
        return -1;
    }
    free(query_f);
    mysql_close(con);
    return 0;
}


