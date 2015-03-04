#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "service.h"
#include "service_internal.h"
#include <json/json.h>



void maptojson(json_object ** obj,map * m){
    json_object *jobj = json_object_new_object();
    map *tmp = m;
    while(tmp != NULL){
        json_object_object_add(jobj,tmp->name,json_object_new_string(tmp->value));
        tmp = tmp->next;
    }
    *obj = jobj;
}

void mapstojson(json_object ** obj,maps * m){
    json_object *jobj = json_object_new_object();
    maps *tmp = m;
    while(tmp!=NULL){
        json_object *map_obj;
        maptojson(&map_obj,tmp->content);
        json_object_object_add(jobj,tmp->name,map_obj);
        tmp = tmp->next;
    }
    *obj = jobj;
}


map* jsontomap(json_object * jobj){
    map *m;
    json_object_object_foreach(jobj, key, val){
        if (json_object_is_type(val, json_type_string)){
            addMapToMap(&m,createMap(key,json_object_get_string(val)));
        }
        else {
            /* Attention ce n'est pas une map */
            return NULL;
        }
     }
        return m;
}

int jsontomaps(json_object * jobj,maps ** m){
    maps *maps_tmp;
    json_object_object_foreach(jobj, key, val){
        maps_tmp = (maps *) malloc (MAPS_SIZE);
        maps_tmp->content = jsontomap(val); 
        maps_tmp->name = zStrdup(key);
        addMapsToMaps(m,maps_tmp);
    }
    return 0;
}
 

