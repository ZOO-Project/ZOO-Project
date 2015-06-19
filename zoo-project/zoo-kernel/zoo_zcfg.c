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
#include <glib.h>
#include <sys/stat.h>
#include "service.h"
#include "service_internal.h"
#include "response_print.h"

extern "C" {
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "service.h"
#include "service_internal.h"
}

int
readServiceFile (maps * conf, char *file, service ** service, char *name);
int
createRegistry (maps* m,registry ** r, char *reg_dir);
static int SCAN_DEPTH = 7;


static GList *serviceCfgList;

//static maps * main_conf = NULL;


/*int
load_main_conf(char * path){
    if (main_conf != NULL)
        freeMaps(&main_conf);
    main_conf = (maps *) malloc (MAP_SIZE);
    return conf_read (path, main_conf);
}

maps *  get_main_conf(){
    return  dupMaps(&main_conf);
}

*/

static int
scanServiceCfg (char *rootDir, GList ** serviceList, int level)
{
  if (level >= SCAN_DEPTH)
    return 1;
  struct dirent *d;
  DIR *dir = opendir (rootDir);
  if (dir != NULL)
    {
      while ((d = readdir (dir)))
        {
          if ((d->d_type == DT_DIR || d->d_type == DT_LNK)
              && d->d_name[0] != '.' && strstr (d->d_name, ".") == NULL)
            {
              char *name =
                (char *) malloc (strlen (rootDir) + strlen (d->d_name) + 2);
              name[0] = '\0';
              strncat (name, rootDir, strlen (rootDir));
              strncat (name, "/", strlen ("/"));
              strncat (name, d->d_name, strlen (d->d_name));
              scanServiceCfg (name, serviceList, level + 1);
            }
          else
            {
              if (d->d_name[0] != '.')
                if (strstr (d->d_name, ".zcfg") != NULL)
                  {
                    char *name =
                      (char *) malloc (strlen (rootDir) + strlen (d->d_name) +
                                       2);
                    name[0] = '\0';
                    strncat (name, rootDir, strlen (rootDir));
                    strncat (name, "/", strlen ("/"));
                    strncat (name, d->d_name, strlen (d->d_name));
                    *serviceList = g_list_append (*serviceList, name);
                  }
            }
        }
        if (level > 0)
            free(rootDir);
    }
  closedir (dir);
  return 1;
}

static int
get_identifier (char *root_dir, char *zcfg_path, char **identifier)
{
  // On extrait le repertoire racine ainsi que l'extention .zcfg pour contruire l'identifiant
  // root_dir = /var/www/zoo/cgi-bin/ zcfg_path=/var/www/zoo/cgi-bin/gdal/ndvi/ExtractNDVI.zcfg  ===>  gdal.ndvi.ExtractNDVI
  char *identifier_tmp =
    (char *) malloc ((strlen (zcfg_path) + 1) * sizeof (char));
  identifier_tmp[0] = '\0';
  int ext_len = strlen (".zcfg");
  int s_tmp_len = strlen (zcfg_path) - ext_len - strlen (root_dir);
  if (s_tmp_len > 1)
    {
      char *s_tmp = (char *) malloc ((s_tmp_len + 1) * sizeof (char));
      int j;
      for (j = 0; j < s_tmp_len; j++)
        s_tmp[j] = zcfg_path[strlen (root_dir) + j];
      s_tmp[s_tmp_len] = '\0';
      char *save_ptr_strtok;
      char *token = strtok_r (s_tmp, "/", &save_ptr_strtok);
      strncat (identifier_tmp, token, strlen (token));
      while (token != NULL)
        {
          token = strtok_r (NULL, "/", &save_ptr_strtok);
          if (token != NULL)
            {
              strncat (identifier_tmp, ".", strlen ("."));
              strncat (identifier_tmp, token, strlen (token));
            }

        }
      *identifier =
        (char *) malloc ((strlen (identifier_tmp) + 1) * sizeof (char));
      strncpy (*identifier, identifier_tmp, strlen (identifier_tmp) + 1);
      free (s_tmp);
    }
  free (identifier_tmp);
  return 1;
}

void
init_services_conf (char *rootDir,char *regDir)
{
  registry* zooRegistry=NULL;
  maps *m = (maps *) malloc (MAPS_SIZE);
  if (regDir != NULL)
    createRegistry (m,&zooRegistry,regDir);
  
  GList *L = NULL;
  scanServiceCfg (rootDir, &L, 0);
  GList *l = NULL;
  for (l = L; l; l = l->next)
    {
      service *s1 = (service *) malloc (SERVICE_SIZE);
      get_identifier (rootDir, (char *) (l->data), &(s1->identifier));
      s1->zcfg = (char *) (l->data);
      readServiceFile (m, (char *) l->data, &s1, s1->identifier);
      inheritance(zooRegistry,&s1);
      serviceCfgList = g_list_append (serviceCfgList, s1);
    }
  if(zooRegistry!=NULL){
    freeRegistry(&zooRegistry);
    free(zooRegistry);
  }
  //freeMaps(&m);
}


service *
search_service (char *identifier)
{
  GList *l;
  int i = 0;
  for (l = serviceCfgList; l; l = l->next)
    {
#ifdef DEBUG
      fprintf (stderr, "%d ### %s ###\n", i,
               ((service *) (l->data))->identifier);
      i++;
#endif
      if (strcasecmp (identifier, ((service *) (l->data))->identifier) == 0)
        return (service *) l->data;
    }
  return NULL;
}

/**
 * Create the profile registry.
 *
 * The profile registry is optional (created only if the registry key is
 * available in the [main] section of the main.cfg file) and can be used to
 * store the profiles hierarchy. The registry is a directory which should
 * contain the following sub-directories: 
 *  * concept: direcotry containing .html files describing concept
 *  * generic: directory containing .zcfg files for wps:GenericProcess
 *  * implementation: directory containing .zcfg files for wps:Process
 *
 * @param m the conf maps containing the main.cfg settings
 * @param r the registry to update
 * @param reg_dir the resgitry 
 * @param saved_stdout the saved stdout identifier
 * @return 0 if the resgitry is null or was correctly updated, -1 on failure
 */

int
createRegistry (maps* m,registry ** r, char *reg_dir)
{
  char registryKeys[3][15]={
    "concept",
    "generic",
    "implementation"
  };
  int scount = 0,i=0;
  if (reg_dir == NULL)
    return 0;
  for(i=0;i<3;i++){
    char * tmpName =
      (char *) malloc ((strlen (reg_dir) + strlen (registryKeys[i]) + 2) *
               sizeof (char));
    sprintf (tmpName, "%s/%s", reg_dir, registryKeys[i]);

    DIR *dirp1 = opendir (tmpName);
    struct dirent *dp1;
    while ((dp1 = readdir (dirp1)) != NULL){
      char* extn = strstr(dp1->d_name, ".zcfg");
      if(dp1->d_name[0] != '.' && extn != NULL && strlen(extn) == 5)
    {
      int t;
      char *tmps1=
        (char *) malloc ((strlen (tmpName) + strlen (dp1->d_name) + 2) *
                 sizeof (char));
      sprintf (tmps1, "%s/%s", tmpName, dp1->d_name);
      char *tmpsn = zStrdup (dp1->d_name);
      tmpsn[strlen (tmpsn) - 5] = 0;
      service* s1 = (service *) malloc (SERVICE_SIZE);
      if (s1 == NULL)
        {
          return -1;
        }
      t = readServiceFile (m, tmps1, &s1, tmpsn);
      free (tmpsn);
      if (t < 0)
        {
          map *tmp00 = getMapFromMaps (m, "lenv", "message");
          char tmp01[1024];
          if (tmp00 != NULL)
        sprintf (tmp01, _("Unable to parse the ZCFG file: %s (%s)"),
             dp1->d_name, tmp00->value);
          else
        sprintf (tmp01, _("Unable to parse the ZCFG file: %s."),
             dp1->d_name);
          return -1;
        }
#ifdef DEBUG
      dumpService (s1);
      fflush (stdout);
      fflush (stderr);
#endif
      if(strncasecmp(registryKeys[i],"implementation",14)==0){
        inheritance(*r,&s1);
      }
      addServiceToRegistry(r,registryKeys[i],s1);
      freeService (&s1);
      free (s1);
      scount++;
    }
    }
    (void) closedir (dirp1);
  }
  return 0;
}


void
CapabilitiesAllProcess (maps * m, xmlNodePtr nc)
{
  GList *l;
  for (l = serviceCfgList; l; l = l->next)
    {
      service *serv = (service *) l->data;
      printGetCapabilitiesForProcess(m,nc,serv);

    }
}


void DescribeProcess(maps * m, xmlNodePtr nc,char * identifiers)
{
  if (strcasecmp ("all", identifiers) == 0){
    GList *l;
    for (l = serviceCfgList; l; l = l->next)
      {
        service *serv = (service *) l->data;
        printDescribeProcessForProcess(m,nc,serv);
      }
  }
  else {
    char *save_ptr_strtok;
    char *token = strtok_r (identifiers, ",", &save_ptr_strtok);
    while (token != NULL)
        {
          service *serv = search_service (token);
          if (serv != NULL)
            printDescribeProcessForProcess (m, nc, serv);
          token = strtok_r (NULL, ",", &save_ptr_strtok);
        }
    }
}
