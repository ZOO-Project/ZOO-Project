#ifndef ZOO_JSON_H
#define ZOO_JSON_H 1


#include "service.h"
#include "service_internal.h"
#include <json/json.h>
void maptojson(json_object ** obj,map * m);

void mapstojson(json_object ** obj,maps * m);

map* jsontomap(json_object * jobj);
int jsontomaps(json_object * jobj,maps **m);

#endif
