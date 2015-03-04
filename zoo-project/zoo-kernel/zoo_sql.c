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

int add_status(const char * uuid) {
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
