#ifndef ZOO_SQL_H
#define ZOO_SQL_H 1


void init_sql(const char* host,const char *user, const char *passwd, const char * bdd,int port);
char * get_uuid();
int add_status(const char * uuid);
#endif
