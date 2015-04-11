/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2015 GeoLabs SARL
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

#include "request_parser.h"
#include "service_internal.h"

/**
 * Apply XPath Expression on XML document.
 *
 * @param doc the XML Document
 * @param search the XPath expression
 * @return xmlXPathObjectPtr containing the resulting nodes set
 */
xmlXPathObjectPtr
extractFromDoc (xmlDocPtr doc, const char *search)
{
  xmlXPathContextPtr xpathCtx;
  xmlXPathObjectPtr xpathObj;
  xpathCtx = xmlXPathNewContext (doc);
  xpathObj = xmlXPathEvalExpression (BAD_CAST search, xpathCtx);
  xmlXPathFreeContext (xpathCtx);
  return xpathObj;
}

/**
 * Create (or append to) an array valued maps value = "["",""]"
 *
 * @param m the conf maps containing the main.cfg settings
 * @param mo the map to update
 * @param mi the map to append
 * @param elem the elements containing default definitions
 * @return 0 on success, -1 on failure
 */
int appendMapsToMaps (maps * m, maps * mo, maps * mi, elements * elem){
  maps *tmpMaps = getMaps (mo, mi->name);
  map *tmap = getMapType (tmpMaps->content);
  elements *el = getElements (elem, mi->name);
  int hasEl = 1;
  if (el == NULL)
    hasEl = -1;
  if (tmap == NULL)
    {
      if (hasEl > 0)
	tmap = getMapType (el->defaults->content);
    }
  
  map *testMap = NULL;
  if (hasEl > 0)
    {
      testMap = getMap (el->content, "maxOccurs");
    }
  else
    {
      testMap = createMap ("maxOccurs", "unbounded");
    }
    
  if (testMap != NULL)
    {
      if (strncasecmp (testMap->value, "unbounded", 9) != 0
	  && atoi (testMap->value) > 1)
	{
	  addMapsArrayToMaps (&mo, mi, tmap->name);
	  map* nb=getMapFromMaps(mo,mi->name,"length");
	  if (nb!=NULL && atoi(nb->value)>atoi(testMap->value))
	    {
	      char emsg[1024];
	      sprintf (emsg,
		       _("The maximum allowed occurrences for <%s> (%i) was exceeded."),
		       mi->name, atoi (testMap->value));
	      errorException (m, emsg, "InternalError", NULL);
	      return -1;
	    }
	}
      else
	{
	  if (strncasecmp (testMap->value, "unbounded", 9) == 0)
	    {
	      if (hasEl < 0)
		{
		  freeMap (&testMap);
		  free (testMap);
		}
	      if (addMapsArrayToMaps (&mo, mi, tmap->name) < 0)
		{
		  char emsg[1024];
		  map *tmpMap = getMap (mi->content, "length");
		  sprintf (emsg,
			   _
			   ("ZOO-Kernel was unable to load your data for %s position %s."),
			   mi->name, tmpMap->value);
		  errorException (m, emsg, "InternalError", NULL);
		  return -1;
		}
	    }
	  else
	    {
	      char emsg[1024];
	      sprintf (emsg,
		       _
		       ("The maximum allowed occurrences for <%s> is one."),
		       mi->name);
	      errorException (m, emsg, "InternalError", NULL);
	      return -1;
	    }
	}
    }
  return 0;
}

/**
 * Parse inputs provided as KVP and store them in a maps.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param s the service
 * @param request_inputs the map storing KVP raw value 
 * @param request_output the maps to store the KVP pairs 
 * @param hInternet the HINTERNET queue to add potential requests
 * @return 0 on success, -1 on failure
 */
int kvpParseInputs(maps** main_conf,service* s,map *request_inputs,maps** request_output,HINTERNET* hInternet){
  // Parsing inputs provided as KVP
  maps *tmpmaps = *request_output;
  map* r_inputs = getMap (request_inputs, "DataInputs");
  char* cursor_input;
  if (r_inputs != NULL){
    //snprintf (cursor_input, 40960, "%s", r_inputs->value);
    if(strstr(r_inputs->value,"=")==NULL)
      cursor_input = url_decode (r_inputs->value);
    else
      cursor_input = zStrdup (r_inputs->value);
    int j = 0;

    // Put each DataInputs into the inputs_as_text array
    char *pToken;
    pToken = strtok (cursor_input, ";");
    char **inputs_as_text = (char **) malloc (100 * sizeof (char *));
    if (inputs_as_text == NULL)
      {
	free(cursor_input);
	return errorException (*main_conf, _("Unable to allocate memory."),
			       "InternalError", NULL);
      }
    int i = 0;
    while (pToken != NULL)
      {
	inputs_as_text[i] =
	  (char *) malloc ((strlen (pToken) + 1) * sizeof (char));
	if (inputs_as_text[i] == NULL)
	  {
	    free(cursor_input);
	    return errorException (*main_conf, _("Unable to allocate memory."),
				   "InternalError", NULL);
	  }
	snprintf (inputs_as_text[i], strlen (pToken) + 1, "%s", pToken);
	pToken = strtok (NULL, ";");
	i++;
      }
	
    for (j = 0; j < i; j++)
      {
	char *tmp = zStrdup (inputs_as_text[j]);
	free (inputs_as_text[j]);
	char *tmpc;
	tmpc = strtok (tmp, "@");
	while (tmpc != NULL)
	  {
	    char *tmpv = strstr (tmpc, "=");
	    char tmpn[256];
	    memset (tmpn, 0, 256);
	    if (tmpv != NULL)
	      {
		strncpy (tmpn, tmpc,
			 (strlen (tmpc) - strlen (tmpv)) * sizeof (char));
		tmpn[strlen (tmpc) - strlen (tmpv)] = 0;
	      }
	    else
	      {
		strncpy (tmpn, tmpc, strlen (tmpc) * sizeof (char));
		tmpn[strlen (tmpc)] = 0;
	      }
	    if (tmpmaps == NULL)
	      {
		tmpmaps = (maps *) malloc (MAPS_SIZE);
		if (tmpmaps == NULL)
		  {
		    free(cursor_input);
		    return errorException (*main_conf,
					   _("Unable to allocate memory."),
					   "InternalError", NULL);
		  }
		tmpmaps->name = zStrdup (tmpn);
		if (tmpv != NULL)
		  {
		    char *tmpvf = url_decode (tmpv + 1);
		    tmpmaps->content = createMap ("value", tmpvf);
		    free (tmpvf);
		  }
		else
		  tmpmaps->content = createMap ("value", "Reference");
		tmpmaps->next = NULL;
	      }
	    tmpc = strtok (NULL, "@");
	    while (tmpc != NULL)
	      {
		char *tmpv1 = strstr (tmpc, "=");
		char tmpn1[1024];
		memset (tmpn1, 0, 1024);
		if (tmpv1 != NULL)
		  {
		    strncpy (tmpn1, tmpc, strlen (tmpc) - strlen (tmpv1));
		    tmpn1[strlen (tmpc) - strlen (tmpv1)] = 0;
		    addToMap (tmpmaps->content, tmpn1, tmpv1 + 1);
		  }
		else
		  {
		    strncpy (tmpn1, tmpc, strlen (tmpc));
		    tmpn1[strlen (tmpc)] = 0;
		    map *lmap = getLastMap (tmpmaps->content);
		    char *tmpValue =
		      (char *) malloc ((strlen (tmpv) + strlen (tmpc) + 1) *
				       sizeof (char));
		    sprintf (tmpValue, "%s@%s", tmpv + 1, tmpc);
		    free (lmap->value);
		    lmap->value = zStrdup (tmpValue);
		    free (tmpValue);
		    tmpc = strtok (NULL, "@");
		    continue;
		  }
		if (strcmp (tmpn1, "xlink:href") != 0)
		  addToMap (tmpmaps->content, tmpn1, tmpv1 + 1);
		else if (tmpv1 != NULL)
		  {
		    char *tmpx2 = url_decode (tmpv1 + 1);
		    if (strncasecmp (tmpx2, "http://", 7) != 0 &&
			strncasecmp (tmpx2, "ftp://", 6) != 0 &&
			strncasecmp (tmpx2, "https://", 8) != 0)
		      {
			char emsg[1024];
			sprintf (emsg,
				 _
				 ("Unable to find a valid protocol to download the remote file %s"),
				 tmpv1 + 1);
			free(cursor_input);
			return errorException (*main_conf, emsg, "InternalError", NULL);
		      }
		    addToMap (tmpmaps->content, tmpn1, tmpx2);
		    {
		      if (loadRemoteFile
			  (&*main_conf, &tmpmaps->content, hInternet, tmpx2) < 0)
			{
			  free(cursor_input);
			  return errorException (*main_conf, "Unable to fetch any ressource", "InternalError", NULL);
			}
		      }
		    free (tmpx2);
		    addIntToMap (tmpmaps->content, "Order", hInternet->nb);
		    addToMap (tmpmaps->content, "Reference", tmpv1 + 1);
		  }
		tmpc = strtok (NULL, "@");
	      }
	    if (*request_output == NULL)
	      *request_output = dupMaps (&tmpmaps);
	    else
	      {
		maps *testPresence =
		  getMaps (*request_output, tmpmaps->name);
		if (testPresence != NULL)
		  {
		    elements *elem =
		      getElements (s->inputs, tmpmaps->name);
		    if (elem != NULL)
		      {
			if (appendMapsToMaps
			    (*main_conf, *request_output, tmpmaps,
			     elem) < 0)
			  {
			    free(cursor_input);
			    return errorException (*main_conf, "Unable to append maps", "InternalError", NULL);
			  }
		      }
		  }
		else
		  addMapsToMaps (request_output, tmpmaps);
	      }
	    freeMaps (&tmpmaps);
	    free (tmpmaps);
	    tmpmaps = NULL;
	    free (tmp);
	  }
      }
    free (inputs_as_text);
    free(cursor_input);
  }
  return 1;
}

/**
 * Parse outputs provided as KVP and store them in a maps.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request_inputs the map storing KVP raw value 
 * @param request_output the maps to store the KVP pairs 
 * @return 0 on success, -1 on failure
 */
int kvpParseOutputs(maps** main_conf,map *request_inputs,maps** request_output){
  /**
   * Parsing outputs provided as KVP
   */
  map *r_inputs = NULL;
  r_inputs = getMap (request_inputs, "ResponseDocument");
  if (r_inputs == NULL)
    r_inputs = getMap (request_inputs, "RawDataOutput");

  if (r_inputs != NULL)
    {
      char *cursor_output = zStrdup (r_inputs->value);
      int j = 0;

      /**
       * Put each Output into the outputs_as_text array
       */
      char *pToken;
      maps *tmp_output = NULL;
      pToken = strtok (cursor_output, ";");
      char **outputs_as_text = (char **) malloc (128 * sizeof (char *));
      if (outputs_as_text == NULL)
	{
	  free(cursor_output);
	  return errorException (*main_conf, _("Unable to allocate memory"),
				 "InternalError", NULL);
	}
      int i = 0;
      while (pToken != NULL)
	{
	  outputs_as_text[i] =
	    (char *) malloc ((strlen (pToken) + 1) * sizeof (char));
	  if (outputs_as_text[i] == NULL)
	    {
	      free(cursor_output);
	      return errorException (*main_conf, _("Unable to allocate memory"),
				     "InternalError", NULL);
	    }
	  snprintf (outputs_as_text[i], strlen (pToken) + 1, "%s",
		    pToken);
	  pToken = strtok (NULL, ";");
	  i++;
	}
      for (j = 0; j < i; j++)
	{
	  char *tmp = zStrdup (outputs_as_text[j]);
	  free (outputs_as_text[j]);
	  char *tmpc;
	  tmpc = strtok (tmp, "@");
	  int k = 0;
	  while (tmpc != NULL)
	    {
	      if (k == 0)
		{
		  if (tmp_output == NULL)
		    {
		      tmp_output = (maps *) malloc (MAPS_SIZE);
		      if (tmp_output == NULL)
			{
			  free(cursor_output);
			  return errorException (*main_conf,
						 _
						 ("Unable to allocate memory."),
						 "InternalError", NULL);
			}
		      tmp_output->name = zStrdup (tmpc);
		      tmp_output->content = NULL;
		      tmp_output->next = NULL;
		    }
		}
	      else
		{
		  char *tmpv = strstr (tmpc, "=");
		  char tmpn[256];
		  memset (tmpn, 0, 256);
		  strncpy (tmpn, tmpc,
			   (strlen (tmpc) -
			    strlen (tmpv)) * sizeof (char));
		  tmpn[strlen (tmpc) - strlen (tmpv)] = 0;
		  if (tmp_output->content == NULL)
		    {
		      tmp_output->content = createMap (tmpn, tmpv + 1);
		      tmp_output->content->next = NULL;
		    }
		  else
		    addToMap (tmp_output->content, tmpn, tmpv + 1);
		}
	      k++;
	      tmpc = strtok (NULL, "@");
	    }
	  if (*request_output == NULL)
	    *request_output = dupMaps (&tmp_output);
	  else
	    addMapsToMaps (request_output, tmp_output);
	  freeMaps (&tmp_output);
	  free (tmp_output);
	  tmp_output = NULL;
	  free (tmp);
	}
      free (outputs_as_text);
      free(cursor_output);
    }
  return 1;
}

/**
 * Parse inputs from XML nodes and store them in a maps.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param s the service
 * @param request_output the maps to store the KVP pairs 
 * @param doc the xmlDocPtr containing the original request
 * @param nodes the input nodes array
 * @param hInternet the HINTERNET queue to add potential requests
 * @return 0 on success, -1 on failure
 */
int xmlParseInputs(maps** main_conf,service* s,maps** request_output,xmlDocPtr doc,xmlNodeSet* nodes,HINTERNET* hInternet){
  int k = 0;
  int l = 0;
  for (k=0; k < nodes->nodeNr; k++)
    {
      maps *tmpmaps = NULL;
      xmlNodePtr cur = nodes->nodeTab[k];

      if (nodes->nodeTab[k]->type == XML_ELEMENT_NODE)
	{
	  // A specific Input node.
	  xmlNodePtr cur2 = cur->children;
	  while (cur2 != NULL)
	    {
	      while (cur2 != NULL && cur2->type != XML_ELEMENT_NODE)
		cur2 = cur2->next;
	      if (cur2 == NULL)
		break;
	      // Indentifier
	      if (xmlStrncasecmp
		  (cur2->name, BAD_CAST "Identifier",
		   xmlStrlen (cur2->name)) == 0)
		{
		  xmlChar *val =
		    xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
		  if (tmpmaps == NULL)
		    {
		      tmpmaps = (maps *) malloc (MAPS_SIZE);
		      if (tmpmaps == NULL)
			{
			  return errorException (*main_conf,
						 _
						 ("Unable to allocate memory."),
						 "InternalError", NULL);
			}
		      tmpmaps->name = zStrdup ((char *) val);
		      tmpmaps->content = NULL;
		      tmpmaps->next = NULL;
		    }
		  xmlFree (val);
		}
	      // Title, Asbtract
	      if (xmlStrncasecmp
		  (cur2->name, BAD_CAST "Title",
		   xmlStrlen (cur2->name)) == 0
		  || xmlStrncasecmp (cur2->name, BAD_CAST "Abstract",
				     xmlStrlen (cur2->name)) == 0)
		{
		  xmlChar *val =
		    xmlNodeListGetString (doc, cur2->xmlChildrenNode, 1);
		  if (tmpmaps == NULL)
		    {
		      tmpmaps = (maps *) malloc (MAPS_SIZE);
		      if (tmpmaps == NULL)
			{
			  return errorException (*main_conf,
						 _
						 ("Unable to allocate memory."),
						 "InternalError", NULL);
			}
		      tmpmaps->name = zStrdup ("missingIndetifier");
		      tmpmaps->content =
			createMap ((char *) cur2->name, (char *) val);
		      tmpmaps->next = NULL;
		    }
		  else
		    {
		      if (tmpmaps->content != NULL)
			addToMap (tmpmaps->content,
				  (char *) cur2->name, (char *) val);
		      else
			tmpmaps->content =
			  createMap ((char *) cur2->name, (char *) val);
		    }
		  xmlFree (val);
		}
	      // InputDataFormChoice (Reference or Data ?) 
	      if (xmlStrcasecmp (cur2->name, BAD_CAST "Reference") == 0)
		{
		  // Get every attribute from a Reference node
		  // mimeType, encoding, schema, href, method
		  // Header and Body gesture should be added here
		  const char *refs[5] =
		    { "mimeType", "encoding", "schema", "method",
		      "href"
		    };
		  for (l = 0; l < 5; l++)
		    {
		      xmlChar *val = xmlGetProp (cur2, BAD_CAST refs[l]);
		      if (val != NULL && xmlStrlen (val) > 0)
			{
			  if (tmpmaps->content != NULL)
			    addToMap (tmpmaps->content, refs[l],
				      (char *) val);
			  else
			    tmpmaps->content =
			      createMap (refs[l], (char *) val);

			  map *ltmp = getMap (tmpmaps->content, "method");
			  if (l == 4 )
			    {
			      if ((ltmp==NULL || strncmp (ltmp->value, "POST",4) != 0))
				{
				  if (loadRemoteFile
				      (main_conf, &tmpmaps->content, hInternet,
				       (char *) val) != 0)
				    {
				      return errorException (*main_conf,
							     _("Unable to add a request in the queue."),
							     "InternalError",
							     NULL);
				    }
				  addIntToMap (tmpmaps->content, "Order", hInternet->nb);
				}
			      addToMap (tmpmaps->content, "Reference", (char*) val);
			    }
			}
		      xmlFree (val);
		    }
		  // Parse Header and Body from Reference
		  xmlNodePtr cur3 = cur2->children;
		  while (cur3 != NULL)
		    {
		      while (cur3 != NULL
			     && cur3->type != XML_ELEMENT_NODE)
			cur3 = cur3->next;
		      if (cur3 == NULL)
			break;
		      if (xmlStrcasecmp (cur3->name, BAD_CAST "Header") ==
			  0)
			{
			  const char *ha[2];
			  ha[0] = "key";
			  ha[1] = "value";
			  int hai;
			  char *has;
			  char *key;
			  for (hai = 0; hai < 2; hai++)
			    {
			      xmlChar *val =
				xmlGetProp (cur3, BAD_CAST ha[hai]);
#ifdef POST_DEBUG
			      fprintf (stderr, "%s = %s\n", ha[hai],
				       (char *) val);
#endif
			      if (hai == 0)
				{
				  key = zStrdup ((char *) val);
				}
			      else
				{
				  has =
				    (char *)
				    malloc ((4 + xmlStrlen (val) +
					     strlen (key)) *
					    sizeof (char));
				  if (has == NULL)
				    {
				      return errorException (*main_conf,
							     _
							     ("Unable to allocate memory."),
							     "InternalError",
							     NULL);
				    }
				  snprintf (has,
					    (3 + xmlStrlen (val) +
					     strlen (key)), "%s: %s", key,
					    (char *) val);
				  free (key);
				}
			      xmlFree (val);
			    }
			  hInternet->ihandle[hInternet->nb].header =
			    curl_slist_append (hInternet->ihandle
					       [hInternet->nb].header,
					       has);
			  if (has != NULL)
			    free (has);
			}
		      else
			{
#ifdef POST_DEBUG
			  fprintf (stderr,
				   "Try to fetch the body part of the request ...\n");
#endif
			  if (xmlStrcasecmp (cur3->name, BAD_CAST "Body")
			      == 0)
			    {
#ifdef POST_DEBUG
			      fprintf (stderr, "Body part found (%s) !!!\n",
				       (char *) cur3->content);
#endif
			      char *tmp = NULL;
			      xmlNodePtr cur4 = cur3->children;
			      while (cur4 != NULL)
				{
				  while (cur4 && cur4 != NULL && cur4->type && cur4->type != XML_ELEMENT_NODE){
				    if(cur4->next)
				      cur4 = cur4->next;
				    else
				      cur4 = NULL;
				  }
				  if(cur4 != NULL) {
				    xmlDocPtr bdoc =
				      xmlNewDoc (BAD_CAST "1.0");
				    bdoc->encoding =
				      xmlCharStrdup ("UTF-8");
				    xmlDocSetRootElement (bdoc, cur4);
				    xmlChar *btmps;
				    int bsize;
				    // TODO : check for encoding defined in the input
				    xmlDocDumpFormatMemoryEnc(bdoc, &btmps, &bsize, "UTF-8", 0);
				    if (btmps != NULL){
				      tmp = (char *) malloc ((bsize + 1) * sizeof (char));

				      sprintf (tmp, "%s", (char*) btmps);
					  
				      xmlFreeDoc (bdoc);
					  
				      map *btmp =
					getMap (tmpmaps->content, "Reference");
				      if (btmp != NULL)
					{
					  addRequestToQueue(main_conf,hInternet,(char *) btmp->value,false);
					  InternetOpenUrl (hInternet,
							   btmp->value,
							   tmp,
							   xmlStrlen(btmps),
							   INTERNET_FLAG_NO_CACHE_WRITE,
							   0);
					  addIntToMap (tmpmaps->content, "Order", hInternet->nb);
					}
				      xmlFree (btmps);
				      free (tmp);
				      break;
				    }
				  }
				  cur4 = cur4->next;
				}
			    }
			  else
			    if (xmlStrcasecmp
				(cur3->name,
				 BAD_CAST "BodyReference") == 0)
			      {
				xmlChar *val =
				  xmlGetProp (cur3, BAD_CAST "href");
				HINTERNET bInternet, res1, res;
				bInternet = InternetOpen (
#ifndef WIN32
							  (LPCTSTR)
#endif
							  "ZooWPSClient\0",
							  INTERNET_OPEN_TYPE_PRECONFIG,
							  NULL, NULL, 0);
				if (!CHECK_INET_HANDLE (bInternet))
				  fprintf (stderr,
					   "WARNING : bInternet handle failed to initialize");
				bInternet.waitingRequests[0] =
				  strdup ((char *) val);
				res1 =
				  InternetOpenUrl (&bInternet,
						   bInternet.waitingRequests
						   [0], NULL, 0,
						   INTERNET_FLAG_NO_CACHE_WRITE,
						   0);
				processDownloads (&bInternet);
				char *tmp =
				  (char *)
				  malloc ((bInternet.ihandle[0].nDataLen +
					   1) * sizeof (char));
				if (tmp == NULL)
				  {
				    return errorException (*main_conf,
							   _
							   ("Unable to allocate memory."),
							   "InternalError",
							   NULL);
				  }
				size_t bRead;
				InternetReadFile (bInternet.ihandle[0],
						  (LPVOID) tmp,
						  bInternet.
						  ihandle[0].nDataLen,
						  &bRead);
				tmp[bInternet.ihandle[0].nDataLen] = 0;
				InternetCloseHandle (&bInternet);
				map *btmp =
				  getMap (tmpmaps->content, "href");
				if (btmp != NULL)
				  {
				    addRequestToQueue(main_conf,hInternet,(char *) btmp->value,false);

				    res =
				      InternetOpenUrl (hInternet,
						       btmp->value,
						       tmp,
						       strlen(tmp),
						       INTERNET_FLAG_NO_CACHE_WRITE,
						       0);
				    addIntToMap (tmpmaps->content, "Order", hInternet->nb);
				  }
				free (tmp);
			      }
			}
		      cur3 = cur3->next;
		    }
		}
	      else if (xmlStrcasecmp (cur2->name, BAD_CAST "Data") == 0)
		{
		  xmlNodePtr cur4 = cur2->children;
		  while (cur4 != NULL)
		    {
		      while (cur4 != NULL
			     && cur4->type != XML_ELEMENT_NODE)
			cur4 = cur4->next;
		      if (cur4 == NULL)
			break;
		      if (xmlStrcasecmp
			  (cur4->name, BAD_CAST "LiteralData") == 0)
			{
			  // Get every attribute from a LiteralData node
			  // dataType , uom
			  char *list[2];
			  list[0] = zStrdup ("dataType");
			  list[1] = zStrdup ("uom");
			  for (l = 0; l < 2; l++)
			    {
			      xmlChar *val =
				xmlGetProp (cur4, BAD_CAST list[l]);
			      if (val != NULL
				  && strlen ((char *) val) > 0)
				{
				  if (tmpmaps->content != NULL)
				    addToMap (tmpmaps->content, list[l],
					      (char *) val);
				  else
				    tmpmaps->content =
				      createMap (list[l], (char *) val);
				}
			      xmlFree (val);
			      free (list[l]);
			    }
			}
		      else
			if (xmlStrcasecmp
			    (cur4->name, BAD_CAST "ComplexData") == 0)
			  {
			    // Get every attribute from a Reference node
			    // mimeType, encoding, schema
			    const char *coms[3] =
			      { "mimeType", "encoding", "schema" };
			    for (l = 0; l < 3; l++)
			      {
				xmlChar *val =
				  xmlGetProp (cur4, BAD_CAST coms[l]);
				if (val != NULL
				    && strlen ((char *) val) > 0)
				  {
				    if (tmpmaps->content != NULL)
				      addToMap (tmpmaps->content, coms[l],
						(char *) val);
				    else
				      tmpmaps->content =
					createMap (coms[l], (char *) val);
				  }
				xmlFree (val);
			      }
			  }

		      map *test = getMap (tmpmaps->content, "encoding");

		      if (test == NULL)
			{ 
			  if (tmpmaps->content != NULL)
			    addToMap (tmpmaps->content, "encoding",
				      "utf-8");
			  else
			    tmpmaps->content =
			      createMap ("encoding", "utf-8");
			  test = getMap (tmpmaps->content, "encoding");
			}

		      if (strcasecmp (test->value, "base64") != 0)
			{ 
			  xmlChar *mv = xmlNodeListGetString (doc,
							      cur4->xmlChildrenNode,
							      1);
			  map *ltmp =
			    getMap (tmpmaps->content, "mimeType");
			  if (mv == NULL
			      ||
			      (xmlStrcasecmp
			       (cur4->name, BAD_CAST "ComplexData") == 0
			       && (ltmp == NULL
				   || strncasecmp (ltmp->value,
						   "text/xml", 8) == 0)))
			    {
			      xmlDocPtr doc1 = xmlNewDoc (BAD_CAST "1.0");
			      int buffersize;
			      xmlNodePtr cur5 = cur4->children;
			      while (cur5 != NULL
				     && cur5->type != XML_ELEMENT_NODE
				     && cur5->type !=
				     XML_CDATA_SECTION_NODE)
				cur5 = cur5->next;
			      if (cur5 != NULL
				  && cur5->type != XML_CDATA_SECTION_NODE)
				{
				  xmlDocSetRootElement (doc1, cur5);
				  xmlDocDumpFormatMemoryEnc (doc1, &mv,
							     &buffersize,
							     "utf-8", 1);
				  char size[1024];
				  sprintf (size, "%d", buffersize);
				  addToMap (tmpmaps->content, "size",
					    size);
				  xmlFreeDoc (doc1);
				}
			      else
				{
				  if (cur5 != NULL
				      && cur5->type == XML_CDATA_SECTION_NODE){
				    xmlDocPtr doc2 = xmlParseMemory((const char*)cur5->content,xmlStrlen(cur5->content));
				    xmlDocSetRootElement (doc1,xmlDocGetRootElement(doc2));
				    xmlDocDumpFormatMemoryEnc (doc1, &mv,
							       &buffersize,
							       "utf-8", 1);
				    char size[1024];
				    sprintf (size, "%d", buffersize);
				    addToMap (tmpmaps->content, "size",
					      size);
				    xmlFreeDoc (doc2);
				    xmlFreeDoc (doc1);
				  }
				}
			    }else{
			    xmlNodePtr cur5 = cur4->children;
			    while (cur5 != NULL
				   && cur5->type != XML_CDATA_SECTION_NODE)
			      cur5 = cur5->next;
			    if (cur5 != NULL
				&& cur5->type == XML_CDATA_SECTION_NODE){
			      xmlFree(mv);
			      mv=xmlStrdup(cur5->content);
			    }
			  }
			  if (mv != NULL)
			    {
			      addToMap (tmpmaps->content, "value",
					(char *) mv);
			      xmlFree (mv);
			    }
			}
		      else
			{
			  xmlChar *tmp = xmlNodeListGetRawString (doc,
								  cur4->xmlChildrenNode,
								  0);
			  addToMap (tmpmaps->content, "value",
				    (char *) tmp);
			  map *tmpv = getMap (tmpmaps->content, "value");
			  char *res = NULL;
			  char *curs = tmpv->value;
			  int i = 0;
			  for (i = 0; i <= strlen (tmpv->value) / 64;
			       i++)
			    {
			      if (res == NULL)
				res =
				  (char *) malloc (67 * sizeof (char));
			      else
				res =
				  (char *) realloc (res,
						    (((i + 1) * 65) +
						     i) * sizeof (char));
			      int csize = i * 65;
			      strncpy (res + csize, curs, 64);
			      if (i == xmlStrlen (tmp) / 64)
				strcat (res, "\n\0");
			      else
				{
				  strncpy (res + (((i + 1) * 64) + i),
					   "\n\0", 2);
				  curs += 64;
				}
			    }
			  free (tmpv->value);
			  tmpv->value = zStrdup (res);
			  free (res);
			  xmlFree (tmp);
			}
		      cur4 = cur4->next;
		    }
		}
	      cur2 = cur2->next;
	    }

	  {
	    maps *testPresence =
	      getMaps (*request_output, tmpmaps->name);
	    if (testPresence != NULL)
	      {
		elements *elem = getElements (s->inputs, tmpmaps->name);
		if (elem != NULL)
		  {
		    if (appendMapsToMaps
			(*main_conf, *request_output, tmpmaps, elem) < 0)
		      {
			return errorException (*main_conf,
					       _("Unable to append maps to maps."),
					       "InternalError",
					       NULL);
		      }
		  }
	      }
	    else
	      addMapsToMaps (request_output, tmpmaps);
	  }
	  freeMaps (&tmpmaps);
	  free (tmpmaps);
	  tmpmaps = NULL;
	}
    }
  return 1;
}

/**
 * Parse outputs from XML nodes and store them in a maps.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request_inputs the map storing KVP raw value 
 * @param request_output the maps to store the KVP pairs 
 * @param doc the xmlDocPtr containing the original request
 * @param cur the xmlNodePtr corresponding to the ResponseDocument or RawDataOutput XML node
 * @param raw true if the node is RawDataOutput, false in case of ResponseDocument
 * @return 0 on success, -1 on failure
 */
int xmlParseOutputs(maps** main_conf,map** request_inputs,maps** request_output,xmlDocPtr doc,xmlNodePtr cur,bool raw){
  int l=0;
  if( raw == true)
    {
      addToMap (*request_inputs, "RawDataOutput", "");
      if (cur->type == XML_ELEMENT_NODE)
	{

	  maps *tmpmaps = (maps *) malloc (MAPS_SIZE);
	  if (tmpmaps == NULL)
	    {
	      return errorException (*main_conf, _("Unable to allocate memory."),
				     "InternalError", NULL);
	    }
	  tmpmaps->name = zStrdup ("unknownIdentifier");
	  tmpmaps->content = NULL;
	  tmpmaps->next = NULL;

	  // Get every attribute from a RawDataOutput node
	  // mimeType, encoding, schema, uom
	  const char *outs[4] =
	    { "mimeType", "encoding", "schema", "uom" };
	  for (l = 0; l < 4; l++)
	    {
	      xmlChar *val = xmlGetProp (cur, BAD_CAST outs[l]);
	      if (val != NULL)
		{
		  if (strlen ((char *) val) > 0)
		    {
		      if (tmpmaps->content != NULL)
			addToMap (tmpmaps->content, outs[l],
				  (char *) val);
		      else
			tmpmaps->content =
			  createMap (outs[l], (char *) val);
		    }
		  xmlFree (val);
		}
	    }
	  xmlNodePtr cur2 = cur->children;
	  while (cur2 != NULL && cur2->type != XML_ELEMENT_NODE)
	    cur2 = cur2->next;
	  while (cur2 != NULL)
	    {
	      if (xmlStrncasecmp
		  (cur2->name, BAD_CAST "Identifier",
		   xmlStrlen (cur2->name)) == 0)
		{
		  xmlChar *val =
		    xmlNodeListGetString (NULL, cur2->xmlChildrenNode, 1);
		  free (tmpmaps->name);
		  tmpmaps->name = zStrdup ((char *) val);
		  xmlFree (val);
		}
	      cur2 = cur2->next;
	      while (cur2 != NULL && cur2->type != XML_ELEMENT_NODE)
		cur2 = cur2->next;
	    }
	  if (*request_output == NULL)
	    *request_output = dupMaps (&tmpmaps);
	  else
	    addMapsToMaps (request_output, tmpmaps);
	  if (tmpmaps != NULL)
	    {
	      freeMaps (&tmpmaps);
	      free (tmpmaps);
	      tmpmaps = NULL;
	    }
	}
    }
  else
    {
      addToMap (*request_inputs, "ResponseDocument", "");

      if (cur->type == XML_ELEMENT_NODE) {
	// Get every attribute: storeExecuteResponse, lineage, status
	const char *ress[3] =
	  { "storeExecuteResponse", "lineage", "status" };
	xmlChar *val;
	for (l = 0; l < 3; l++)
	  {
	    val = xmlGetProp (cur, BAD_CAST ress[l]);
	    if (val != NULL && strlen ((char *) val) > 0)
	      {
		addToMap (*request_inputs, ress[l], (char *) val);
	      }
	    xmlFree (val);
	  }
			
	xmlNodePtr cur1 = cur->children;		
	while (cur1 != NULL) // iterate over Output nodes
	  {
	    if (cur1->type != XML_ELEMENT_NODE || 
		xmlStrncasecmp(cur1->name, BAD_CAST "Output", 
			       xmlStrlen (cur1->name)) != 0) {
	      cur1 = cur1->next;
	      continue;
	    }
				
	    maps *tmpmaps = (maps *) malloc (MAPS_SIZE); // one per Output node
	    if (tmpmaps == NULL) {
	      return errorException (*main_conf,
				     _
				     ("Unable to allocate memory."),
				     "InternalError", NULL);
	    }
	    tmpmaps->name = zStrdup ("unknownIdentifier");
	    tmpmaps->content = NULL;
	    tmpmaps->next = NULL;
				
	    xmlNodePtr elems = cur1->children;
				
	    while (elems != NULL) {

	      // Identifier
	      if (xmlStrncasecmp
		  (elems->name, BAD_CAST "Identifier",
		   xmlStrlen (elems->name)) == 0)
		{
		  xmlChar *val =
		    xmlNodeListGetString (doc, elems->xmlChildrenNode, 1);
		
		  free(tmpmaps->name);
		  tmpmaps->name = zStrdup ((char *) val);
		  if (tmpmaps->content == NULL) {
		    tmpmaps->content = createMap("Identifier", zStrdup ((char *) val));
		  }
		  else {
		    addToMap(tmpmaps->content, "Identifier", zStrdup ((char *) val));
		  }

		  map* tt = getMap (*request_inputs, "ResponseDocument");
		  if (strlen(tt->value) == 0) {
		    addToMap (*request_inputs, "ResponseDocument",
			      (char *) val);
		  }
		  else {
		    char* tmp = (char*) malloc((strlen(tt->value) + 1 
						+ strlen((char*) val) + 1) * sizeof(char));
		    sprintf (tmp, "%s;%s", tt->value, (char *) val);
		    free(tt->value);
		    tt->value = tmp;
		  }
		  xmlFree (val);
		}
	      
	      // Title, Abstract
	      else if (xmlStrncasecmp(elems->name, BAD_CAST "Title",
				      xmlStrlen (elems->name)) == 0
		       || xmlStrncasecmp(elems->name, BAD_CAST "Abstract",
					 xmlStrlen (elems->name)) == 0)
		{
		  xmlChar *val =
		    xmlNodeListGetString (doc, elems->xmlChildrenNode, 1);
							
		  if (tmpmaps->content == NULL) {
		    tmpmaps->content = createMap((char*) elems->name, zStrdup ((char *) val));
		  }
		  else {
		    addToMap(tmpmaps->content, (char*) elems->name, zStrdup ((char *) val));
		  }
		  xmlFree (val);
		}
	      elems = elems->next;
	    }
				
	    // Get every attribute from an Output node:
	    // mimeType, encoding, schema, uom, asReference
	    const char *outs[5] =
	      { "mimeType", "encoding", "schema", "uom", "asReference" };
					  
	    for (l = 0; l < 5; l++) {
	      xmlChar *val = xmlGetProp (cur1, BAD_CAST outs[l]);				
	      if (val != NULL && xmlStrlen(val) > 0) {
		if (tmpmaps->content != NULL) {
		  addToMap (tmpmaps->content, outs[l], (char *) val);
		}			  
		else {
		  tmpmaps->content = createMap (outs[l], (char *) val);
		}	
	      }
	      xmlFree (val);
	    }
				
	    if (*request_output == NULL) {
	      *request_output = tmpmaps;
	    }	
	    else if (getMaps(*request_output, tmpmaps->name) != NULL) {
	      return errorException (*main_conf,
				     _
				     ("Duplicate <Output> elements in WPS Execute request"),
				     "InternalError", NULL);
	    }
	    else {
	      maps* mptr = *request_output;
	      while (mptr->next != NULL) {
		mptr = mptr->next;
	      }
	      mptr->next = tmpmaps;	
	    }					
	    cur1 = cur1->next;
	  }			 
      }
    }
  return 1;
}

/**
 * Parse XML request and store informations in maps.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param post the string containing the XML request
 * @param request_inputs the map storing KVP raw value 
 * @param s the service
 * @param inputs the maps to store the KVP pairs 
 * @param outputs the maps to store the KVP pairs 
 * @param hInternet the HINTERNET queue to add potential requests
 * @return 0 on success, -1 on failure
 */
int xmlParseRequest(maps** main_conf,const char* post,map** request_inputs,service* s,maps** inputs,maps** outputs,HINTERNET* hInternet){
  xmlInitParser ();
  xmlDocPtr doc = xmlParseMemory (post, cgiContentLength);

  /**
   * Extract Input nodes from the XML Request.
   */
  xmlXPathObjectPtr tmpsptr =
    extractFromDoc (doc, "/*/*/*[local-name()='Input']");
  xmlNodeSet *tmps = tmpsptr->nodesetval;
  if(xmlParseInputs(main_conf,s,inputs,doc,tmps,hInternet)<0){
    xmlXPathFreeObject (tmpsptr);
    xmlFreeDoc (doc);
    xmlCleanupParser ();
    return -1;
  }
  xmlXPathFreeObject (tmpsptr);

  // Extract ResponseDocument / RawDataOutput from the XML Request 
  tmpsptr =
    extractFromDoc (doc, "/*/*/*[local-name()='ResponseDocument']");
  bool asRaw = false;
  tmps = tmpsptr->nodesetval;
  if (tmps->nodeNr == 0)
    {
      xmlXPathFreeObject (tmpsptr);
      tmpsptr =
	extractFromDoc (doc, "/*/*/*[local-name()='RawDataOutput']");
      tmps = tmpsptr->nodesetval;
      asRaw = true;
    }
  if(tmps->nodeNr != 0){
    if(xmlParseOutputs(main_conf,request_inputs,outputs,doc,tmps->nodeTab[0],asRaw)<0){
      xmlXPathFreeObject (tmpsptr);
      xmlFreeDoc (doc);
      xmlCleanupParser ();
      return -1;
    }
  }
  xmlXPathFreeObject (tmpsptr);
  xmlFreeDoc (doc);
  xmlCleanupParser ();
  return 1;
}

/**
 * Parse request and store informations in maps.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param post the string containing the XML request
 * @param request_inputs the map storing KVP raw value 
 * @param s the service
 * @param inputs the maps to store the KVP pairs 
 * @param outputs the maps to store the KVP pairs 
 * @param hInternet the HINTERNET queue to add potential requests
 * @return 0 on success, -1 on failure
 * @see kvpParseOutputs,kvpParseInputs,xmlParseRequest
 */
int parseRequest(maps** main_conf,map** request_inputs,service* s,maps** inputs,maps** outputs,HINTERNET* hInternet){
  map *postRequest = NULL;
  postRequest = getMap (*request_inputs, "xrequest");
  if (postRequest == NULLMAP)
    {
      if(kvpParseOutputs(main_conf,*request_inputs,outputs)<0){
	return -1;
      }
      if(kvpParseInputs(main_conf,s,*request_inputs,inputs,hInternet)<0){
	return -1;
      }
    }
  else
    {
      //Parse XML request
      if(xmlParseRequest(main_conf,postRequest->value,request_inputs,s,inputs,outputs,hInternet)<0){
	return -1;
      }
    }
  return 1;
}

/**
 * Ensure that each requested arguments are present in the request
 * DataInputs and ResponseDocument / RawDataOutput. Potentially run
 * http requests from the queue in parallel.
 * For optional inputs add default values defined in the ZCFG file.
 * 
 * @param main_conf
 * @param s 
 * @param original_request
 * @param request_inputs
 * @param request_outputs
 * @param hInternet 
 * 
 * @see runHttpRequests
 */
int validateRequest(maps** main_conf,service* s,map* original_request, maps** request_inputs,maps** request_outputs,HINTERNET* hInternet){

  runHttpRequests (main_conf, request_inputs, hInternet);
  InternetCloseHandle (hInternet);

  map* errI=NULL;
  char *dfv = addDefaultValues (request_inputs, s->inputs, *main_conf, 0,&errI);

  maps *ptr = *request_inputs;
  while (ptr != NULL)
    {
      map *tmp0 = getMap (ptr->content, "size");
      map *tmp1 = getMap (ptr->content, "maximumMegabytes");
      if (tmp1 != NULL && tmp0 != NULL)
        {
          float i = atof (tmp0->value) / 1048576.0;
          if (i >= atoi (tmp1->value))
            {
              char tmps[1024];
              map *tmpe = createMap ("code", "FileSizeExceeded");
              snprintf (tmps, 1024,
                        _
                        ("The <%s> parameter has a size limit (%s MB) defined in the ZOO ServicesProvider configuration file, but the reference you provided exceeds this limit (%f MB)."),
                        ptr->name, tmp1->value, i);
              addToMap (tmpe, "locator", ptr->name);
              addToMap (tmpe, "text", tmps);
              printExceptionReportResponse (*main_conf, tmpe);
              freeMap (&tmpe);
              free (tmpe);
	      return -1;
            }
        }
      ptr = ptr->next;
    }

  map* errO=NULL;
  char *dfv1 =
    addDefaultValues (request_outputs, s->outputs, *main_conf, 1,&errO);
  if (strcmp (dfv1, "") != 0 || strcmp (dfv, "") != 0)
    {
      char tmps[1024];
      map *tmpe = NULL;
      if (strcmp (dfv, "") != 0)
        {
	  tmpe = createMap ("code", "MissingParameterValue");
	  int nb=0;
	  int length=1;
	  map* len=getMap(errI,"length");
	  if(len!=NULL)
	    length=atoi(len->value);
	  for(nb=0;nb<length;nb++){
	    map* errp=getMapArray(errI,"value",nb);
	    snprintf (tmps, 1024,
		      _
		      ("The <%s> argument was not specified in DataInputs but is required according to the ZOO ServicesProvider configuration file."),
		      errp->value);
	    setMapArray (tmpe, "locator", nb , errp->value);
	    setMapArray (tmpe, "text", nb , tmps);
	    setMapArray (tmpe, "code", nb , "MissingParameterValue");
	  }
	}
      if (strcmp (dfv1, "") != 0)
        {
	  int ilength=0;
	  if(tmpe==NULL)
	    tmpe = createMap ("code", "InvalidParameterValue");
	  else{
	    map* len=getMap(tmpe,"length");
	    if(len!=NULL)
	      ilength=atoi(len->value);
	  }
	  int nb=0;
	  int length=1;
	  map* len=getMap(errO,"length");
	  if(len!=NULL)
	    length=atoi(len->value);
	  for(nb=0;nb<length;nb++){
	    map* errp=getMapArray(errO,"value",nb);
	    snprintf (tmps, 1024,
		      _
		      ("The <%s> argument specified as %s identifier was not recognized (not defined in the ZOO Configuration File)."),
		      errp->value,
		      ((getMap(original_request,"RawDataOutput")!=NULL)?"RawDataOutput":"ResponseDocument"));
	    setMapArray (tmpe, "locator", nb+ilength , errp->value);
	    setMapArray (tmpe, "text", nb+ilength , tmps);
	    setMapArray (tmpe, "code", nb+ilength , "InvalidParameterValue");
	  }
	}
      printExceptionReportResponse (*main_conf, tmpe);
      if(errI!=NULL){
	freeMap(&errI);
	free(errI);
      }
      if(errO!=NULL){
	freeMap(&errO);
	free(errO);
      }
      freeMap (&tmpe);
      free (tmpe);
      return -1;
    }
  maps *tmpReqI = *request_inputs;
  while (tmpReqI != NULL)
    {
      char name[1024];
      if (getMap (tmpReqI->content, "isFile") != NULL)
        {
          if (cgiFormFileName (tmpReqI->name, name, sizeof (name)) ==
              cgiFormSuccess)
            {
              int BufferLen = 1024;
              cgiFilePtr file;
              int targetFile;
              char storageNameOnServer[2048];
              char fileNameOnServer[64];
              char contentType[1024];
              char buffer[1024];
              char *tmpStr = NULL;
              int size;
              int got, t;
              map *path = getMapFromMaps (*main_conf, "main", "tmpPath");
              cgiFormFileSize (tmpReqI->name, &size);
              cgiFormFileContentType (tmpReqI->name, contentType,
                                      sizeof (contentType));
              if (cgiFormFileOpen (tmpReqI->name, &file) == cgiFormSuccess)
                {
                  t = -1;
                  while (1)
                    {
                      tmpStr = strstr (name + t + 1, "\\");
                      if (NULL == tmpStr)
                        tmpStr = strstr (name + t + 1, "/");
                      if (NULL != tmpStr)
                        t = (int) (tmpStr - name);
                      else
                        break;
                    }
                  strcpy (fileNameOnServer, name + t + 1);

                  sprintf (storageNameOnServer, "%s/%s", path->value,
                           fileNameOnServer);
#ifdef DEBUG
                  fprintf (stderr, "Name on server %s\n",
                           storageNameOnServer);
                  fprintf (stderr, "fileNameOnServer: %s\n",
                           fileNameOnServer);
#endif
                  targetFile =
                    open (storageNameOnServer, O_RDWR | O_CREAT | O_TRUNC,
                          S_IRWXU | S_IRGRP | S_IROTH);
                  if (targetFile < 0)
                    {
#ifdef DEBUG
                      fprintf (stderr, "could not create the new file,%s\n",
                               fileNameOnServer);
#endif
                    }
                  else
                    {
                      while (cgiFormFileRead (file, buffer, BufferLen, &got)
                             == cgiFormSuccess)
                        {
                          if (got > 0)
                            write (targetFile, buffer, got);
                        }
                    }
                  addToMap (tmpReqI->content, "lref", storageNameOnServer);
                  cgiFormFileClose (file);
                  close (targetFile);
#ifdef DEBUG
                  fprintf (stderr, "File \"%s\" has been uploaded",
                           fileNameOnServer);
#endif
                }
            }
        }
      tmpReqI = tmpReqI->next;
    }

  ensureDecodedBase64 (request_inputs);
  return 1;
}
