#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <sys/stat.h>
#include "service.h"
#include "service_internal.h"
extern "C" {
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "service.h"
#include "service_internal.h"
}


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
init_services_conf (char *rootDir)
{
  maps *m = (maps *) malloc (MAPS_SIZE);
  GList *L = NULL;
  scanServiceCfg (rootDir, &L, 0);
  GList *l = NULL;
  for (l = L; l; l = l->next)
    {
      service *s1 = (service *) malloc (SERVICE_SIZE);
      get_identifier (rootDir, (char *) (l->data), &(s1->identifier));
      s1->zcfg = (char *) (l->data);
      readServiceFile (m, (char *) l->data, &s1, s1->identifier);
      serviceCfgList = g_list_append (serviceCfgList, s1);
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


void
XML_CapabilitiesAllProcess (maps * m, xmlNodePtr nc)
{
  GList *l;
  for (l = serviceCfgList; l; l = l->next)
    {

      service *serv = (service *) l->data;

      xmlNsPtr ns, ns_ows, ns_xlink;
      xmlNodePtr n = NULL, nc1, nc2;
  /**
   * Initialize or get existing namspaces
   */
      int wpsId =
        zooXmlAddNs (NULL, "http://www.opengis.net/wps/1.0.0", "wps");
      ns = usedNs[wpsId];
      int owsId = zooXmlAddNs (NULL, "http://www.opengis.net/ows/1.1", "ows");
      ns_ows = usedNs[owsId];
      int xlinkId = zooXmlAddNs (n, "http://www.w3.org/1999/xlink", "xlink");
      ns_xlink = usedNs[xlinkId];

      map *tmp1;
      if (serv->content != NULL)
        {
          nc1 = xmlNewNode (ns, BAD_CAST "Process");
          tmp1 = getMap (serv->content, "processVersion");
          if (tmp1 != NULL)
            xmlNewNsProp (nc1, ns, BAD_CAST "processVersion",
                          BAD_CAST tmp1->value);
          //map *tmp3 = getMapFromMaps (m, "lenv", "level");
          //addPrefix (m, tmp3, serv);
          printDescription (nc1, ns_ows, serv->identifier, serv->content);
          tmp1 = serv->metadata;
          while (tmp1 != NULL)
            {
              nc2 = xmlNewNode (ns_ows, BAD_CAST "Metadata");
              xmlNewNsProp (nc2, ns_xlink, BAD_CAST tmp1->name,
                            BAD_CAST tmp1->value);
              xmlAddChild (nc1, nc2);
              tmp1 = tmp1->next;
            }
          xmlAddChild (nc, nc1);
        }
    }
}

void
XML_Describe_Process (maps * m, xmlNodePtr nc, char *identifiers)
{
  if (strcasecmp ("all", identifiers) == 0)
    {
      GList *l;
      for (l = serviceCfgList; l; l = l->next)
        {
          service *serv = (service *) l->data;
          printDescribeProcessForProcess (m, nc, serv);
        }
    }
  else
    {
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
