/**
 * Author : David SAGGIORATO
 *
 * Copyright (c) 2009-2010 GeoLabs SARL
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

#include "service_internal_perl.h"


static PerlInterpreter *my_perl;


void xs_init(pTHX)
{
	char *file = __FILE__;
	dXSUB_SYS;
	
	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}



int map_to_hash(map * m, HV ** hash_map) {
	HV * tmp = *hash_map;
	map * tmp_m = m;
	do {
		//printf("map name %s  value %s \n",m->name,m->value);

		if ( NULL == hv_store( tmp, tmp_m->name, strlen(tmp_m->name), sv_2mortal(newSVpv(tmp_m->value, strlen(tmp_m->value))), 0) ) {
			return 1;
			}
		tmp_m = tmp_m->next;
		}
	while (tmp_m != NULL);
	return 0;
}

int maps_to_hash( maps* m, HV ** hash_maps){
	HV * tmp = *hash_maps;
	if (m != NULL) {
		//printf("maps name %s \n",m->name);
		HV* hash_m = (HV *)sv_2mortal((SV *)newHV());
		if (map_to_hash(m->content,&hash_m) != 0){
			return 1;
		}
		
		if ( NULL == hv_store( tmp, m->name, strlen(m->name),sv_2mortal(newRV_inc((SV *)hash_m)), 0) ) {
			return 1;
		}	
		return maps_to_hash(m->next,hash_maps);
	}
	return 0;
}

int hash_to_map(HV * hh,map ** m){
	hv_iterinit(hh);
	*m = (map *)malloc(MAP_SIZE);
	if (*m == NULL){
		// erreur d'allocation memoire
		return 1;
	}
	map * tmp = *m;
	HE * he = hv_iternext(hh);
	while (he != NULL){
		//fprintf(stderr,"key : %s  value : %s \n",HeKEY(he),(char *)SvRV(HeVAL(he)));
		tmp->name = HeKEY(he);
		tmp->value = (char *)SvRV(HeVAL(he));
		he = hv_iternext(hh);
		if(he != NULL){
			tmp->next = (map *)malloc(MAP_SIZE);
			if (tmp->next == NULL){
				//erreur allocation memoire
				return 1;
			}
			tmp=tmp->next;
		}
		else {
			tmp->next = NULL;
		}

	}

	return 1;
}
	
int hash_to_maps(HV * hh,maps** m){
	hv_iterinit(hh);
	*m = (maps *)malloc(MAPS_SIZE);
	maps * tmp = *m;
	HE * he = hv_iternext(hh);
	map *mm;
	while (he != NULL) {
		//fprintf(stderr,"key ===> %s \n",HeKEY(he));
		tmp->name = HeKEY(he);
		hash_to_map((HV *) SvRV(HeVAL(he)),&mm);
		tmp->content = mm;
		he = hv_iternext(hh);
		if (he != NULL){
			tmp->next = (maps *)malloc(MAPS_SIZE);
			tmp= tmp->next;
		}
		else {
			tmp->next = NULL;
		}		
	}
	return 1;
}
	
int zoo_perl_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
	maps* m=*main_conf;
 	maps* inputs=*real_inputs;
	maps* outputs=*real_outputs;
  	int res=SERVICE_FAILED;
  	map * tmp=getMap(s->content,"serviceProvide");

	char *my_argv[] = { "", tmp->value };
	if ((my_perl = perl_alloc()) == NULL){
		fprintf(stderr,"no memmory");
		exit(1);
	}
	perl_construct( my_perl );
	perl_parse(my_perl, xs_init, 2, my_argv, (char **)NULL);
	perl_run(my_perl);
	

	HV* h_main_conf = (HV *)sv_2mortal((SV *)newHV());
	HV* h_real_inputs = (HV *)sv_2mortal((SV *)newHV());
	HV* h_real_outputs = (HV *)sv_2mortal((SV *)newHV());
	maps_to_hash(m,&h_main_conf);
	maps_to_hash(inputs,&h_real_inputs);
	maps_to_hash(outputs,&h_real_outputs);
	dSP;
    	ENTER;
    	SAVETMPS;
    	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newRV_inc((SV *)h_main_conf)));
	XPUSHs(sv_2mortal(newRV_inc((SV *)h_real_inputs)));
	XPUSHs(sv_2mortal(newRV_inc((SV *)h_real_outputs)));
	PUTBACK;
	call_pv(s->name, G_SCALAR);
	SPAGAIN;
	res = POPi;
	hash_to_maps(h_real_outputs,real_outputs);
	//dumpMaps(*real_outputs);
	PUTBACK;
    	FREETMPS;
    	LEAVE;
	return SERVICE_SUCCEEDED;
}

	































