#ifndef SERVICE_ZCFG_H
#define SERVICE_ZCFG_H 1


#include "service.h"
#include "service_internal.h"

maps *  get_main_conf();
int load_main_conf(char *path);
void init_services_conf (char *rootDir);
service *search_service (char *identifier);
void XML_CapabilitiesAllProcess (maps * m, xmlNodePtr nc);
void XML_Describe_Process (maps * m, xmlNodePtr nc, char *identifiers);
#endif
