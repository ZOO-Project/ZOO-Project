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
    map *m = (map *) malloc (MAP_SIZE);
    int i = 0;
    json_object_object_foreach(jobj, key, val){
        if (json_object_is_type(val, json_type_string)){
            if (i==0){
                m->name = zStrdup(key);
                m->value = zStrdup(json_object_get_string(val));
                m->next = NULL;
                i = 1;
            }
            else {
                map *tmp=createMap(key,json_object_get_string(val));
                addMapToMap(&m,tmp);
                freeMap(&tmp);
                free(tmp);
           }
        }
        else {
            fprintf(stderr,"jsontomap not map :%s \n",key);
            /* Attention ce n'est pas une map */
            return NULL;
        }
     }
        return m;
}

maps * jsontomaps(json_object * jobj){
    maps *mm = (maps *) malloc (MAPS_SIZE); 
    int i =0;
    json_object_object_foreach(jobj, key, val){
        if (i ==0){
            mm->content = jsontomap(val);
            mm->name = zStrdup(key);
            mm->next = NULL;
            i = 1;
        }
        else {
            maps *maps_tmp;
            maps_tmp = (maps *) malloc (MAPS_SIZE);
            maps_tmp->content = jsontomap(val); 
            maps_tmp->name = zStrdup(key);
            maps_tmp->next = NULL;
            addMapsToMaps(&mm,maps_tmp);
            freeMaps(&maps_tmp);
            free(maps_tmp);
            maps_tmp = NULL;
       }
    }
    return mm;
}
 

