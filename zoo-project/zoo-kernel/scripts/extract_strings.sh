#!/bin/bash
mkdir -p ../locale/.cache
for j in ../../../docker/*cfg ;
  do
    for i in Title Abstract;
     do
      grep -i $i $j | sed "s:=:=_(\":g;s:$:\"):g" ;
     done;
 done > ../locale/.cache/my_service_string_to_translate.c
grep links_title -A 10 ../../../docker/oas.cfg | grep -v links_title >> ../locale/.cache/my_service_string_to_translate.c
