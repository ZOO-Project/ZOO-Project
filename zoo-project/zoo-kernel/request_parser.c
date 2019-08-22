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
#include "server_internal.h"
#include "response_print.h"
#include "caching.h"
#include "cgic.h"

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
  elements *cursor = elem;
  while(cursor!=NULL && el==NULL){
    if(cursor->child!=NULL)
      el = getElements (cursor->child, mi->name);
    cursor=cursor->next;
  }
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
 * Make sure that each value encoded in base64 in a maps is decoded.
 *
 * @param in the maps containing the values
 * @see readBase64
 */
void ensureDecodedBase64(maps **in){
  maps* cursor=*in;
  while(cursor!=NULL){
    map *tmp=getMap(cursor->content,"encoding");
    if(tmp!=NULL && strncasecmp(tmp->value,"base64",6)==0){
      tmp=getMap(cursor->content,"value");
      readBase64(&tmp);
      addToMap(cursor->content,"base64_value",tmp->value);
      int size=0;
      char *s=zStrdup(tmp->value);
      free(tmp->value);
      tmp->value=base64d(s,strlen(s),&size);
      free(s);
      char sizes[1024];
      sprintf(sizes,"%d",size);
      addToMap(cursor->content,"size",sizes);
    }
    map* length=getMap(cursor->content,"length");
    if(length!=NULL){
      int len=atoi(length->value);
      for(int i=1;i<len;i++){
	tmp=getMapArray(cursor->content,"encoding",i);
	if(tmp!=NULL && strncasecmp(tmp->value,"base64",6)==0){
	  char key[17];
	  sprintf(key,"base64_value_%d",i);
	  tmp=getMapArray(cursor->content,"value",i);
	  readBase64(&tmp);
	  addToMap(cursor->content,key,tmp->value);
	  int size=0;
	  char *s=zStrdup(tmp->value);
	  free(tmp->value);
	  tmp->value=base64d(s,strlen(s),&size);
	  free(s);
	  char sizes[1024];
	  sprintf(sizes,"%d",size);
	  sprintf(key,"size_%d",i);
	  addToMap(cursor->content,key,sizes);
	}
      }
    }
    if(cursor->child!=NULL)
      ensureDecodedBase64(&cursor->child);
    cursor=cursor->next;
  }
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
	return errorException (*main_conf, _("Unable to allocate memory"),
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
	    return errorException (*main_conf, _("Unable to allocate memory"),
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
		tmpmaps = createMaps(tmpn);
		if (tmpmaps == NULL)
		  {
		    free(cursor_input);
		    return errorException (*main_conf,
					   _("Unable to allocate memory"),
					   "InternalError", NULL);
		  }
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
			  return errorException (*main_conf, "Unable to fetch any resource", "InternalError", NULL);
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
		      tmp_output = createMaps(tmpc);
		      if (tmp_output == NULL)
			{
			  free(cursor_output);
			  return errorException (*main_conf,
						 _
						 ("Unable to allocate memory"),
						 "InternalError", NULL);
			}
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
 * Create a "missingIdentifier" maps in case it is NULL.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param mymaps the maps to update
 * @return 0 on success, 4 on failure
 */
int defineMissingIdentifier(maps** main_conf,maps** mymaps){
  if (*mymaps == NULL){
    *mymaps = createMaps("missingIndetifier");
    if (*mymaps == NULL){
      return errorException (*main_conf,
			     _("Unable to allocate memory"),
			     "InternalError", NULL);
    }
  }
  return 0;
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
  map* version=getMapFromMaps(*main_conf,"main","rversion");
  map* memory=getMapFromMaps(*main_conf,"main","memory");
  int vid=getVersionId(version->value);
  for (k=0; k < nodes->nodeNr; k++)
    {
      maps *tmpmaps = NULL;
      xmlNodePtr cur = nodes->nodeTab[k];

      if (nodes->nodeTab[k]->type == XML_ELEMENT_NODE)
	{
	  // A specific Input node.
	  if(vid==1){
	    xmlChar *val = xmlGetProp (cur, BAD_CAST "id");
	    tmpmaps = createMaps((char *) val);
	    xmlFree(val);
	  }

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
		  if (tmpmaps == NULL && val!=NULL)
		    {
		      tmpmaps = createMaps((char*)val);
		      if (tmpmaps == NULL)
			{
			  return errorException (*main_conf,
						 _
						 ("Unable to allocate memory"),
						 "InternalError", NULL);
			}
		      xmlFree (val);
		    }
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
		  defineMissingIdentifier(main_conf,&tmpmaps);
		  if(val!=NULL){
		    if (tmpmaps->content != NULL)
		      addToMap (tmpmaps->content,
				(char *) cur2->name, (char *) val);
		    else
		      tmpmaps->content =
			createMap ((char *) cur2->name, (char *) val);
		    xmlFree (val);
		  }
		}
	      // InputDataFormChoice (Reference or Data ?) / 2.0.0 DataInputType / Input 
	      if (xmlStrcasecmp (cur2->name, BAD_CAST "Input") == 0)
		{
		  char *xpathExpr=(char*)malloc(61+strlen(tmpmaps->name));
		  sprintf(xpathExpr,"/*/*[local-name()='Input' and @id='%s']/*[local-name()='Input']",tmpmaps->name);
		  xmlXPathObjectPtr tmpsptr = extractFromDoc (doc, xpathExpr);
		  xmlNodeSet *tmps = tmpsptr->nodesetval;
		  if(tmps!=NULL){
		    maps* request_output1=NULL;
		    if(xmlParseInputs(main_conf,s,&request_output1,doc,tmps,hInternet)<0)
		      return -1;
		    if(tmpmaps->child==NULL)
		      tmpmaps->child=dupMaps(&request_output1);
		    else
		      addMapsToMaps(&tmpmaps->child,request_output1);
		    freeMaps(&request_output1);
		    free(request_output1);
		  }
		  while(cur2->next!=NULL)
		    cur2=cur2->next;
		}
	      else if (xmlStrcasecmp (cur2->name, BAD_CAST "Reference") == 0)
		{
		  defineMissingIdentifier(main_conf,&tmpmaps);
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
			  char *has=NULL;
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
							     ("Unable to allocate memory"),
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
			  if (has != NULL){
			    hInternet->ihandle[hInternet->nb].header = NULL;
			    hInternet->ihandle[hInternet->nb].header =
			      curl_slist_append (hInternet->ihandle
						 [hInternet->nb].header,
						 has);
			    free (has);
			  }
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

				      //xmlFreeDoc (bdoc);
					  
				      map *btmp =
					getMap (tmpmaps->content, "Reference");
				      addToMap (tmpmaps->content, "Body", tmp);
				      if (btmp != NULL)
					{
					  addRequestToQueue(main_conf,hInternet,(char *) btmp->value,false);
					  InternetOpenUrl (hInternet,
							   btmp->value,
							   tmp,
							   xmlStrlen(btmps),
							   INTERNET_FLAG_NO_CACHE_WRITE,
							   0,
							   *main_conf);
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
				maps *tmpConf=createMaps("main");
				tmpConf->content=createMap("memory","load");
				bInternet = InternetOpen (
#ifndef WIN32
							  (LPCTSTR)
#endif
							  "ZooWPSClient\0",
							  INTERNET_OPEN_TYPE_PRECONFIG,
							  NULL, NULL, 0);
#ifndef WIN32
				if (!CHECK_INET_HANDLE (bInternet))
				  fprintf (stderr,
					   "WARNING : bInternet handle failed to initialize");
#endif
				bInternet.waitingRequests[0] =
				  zStrdup ((char *) val);
				res1 =
				  InternetOpenUrl (&bInternet,
						   bInternet.waitingRequests
						   [0], NULL, 0,
						   INTERNET_FLAG_NO_CACHE_WRITE,
						   0,
						   tmpConf);
				processDownloads (&bInternet);
				freeMaps(&tmpConf);
				free(tmpConf);
				char *tmp =
				  (char *)
				  malloc ((bInternet.ihandle[0].nDataLen +
					   1) * sizeof (char));
				if (tmp == NULL)
				  {
				    return errorException (*main_conf,
							   _
							   ("Unable to allocate memory"),
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
				InternetCloseHandle(&bInternet);
				addToMap (tmpmaps->content, "Body", tmp);
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
						       0,
						       *main_conf);
				    addIntToMap (tmpmaps->content, "Order", hInternet->nb);
				  }
				free (tmp);
				xmlFree (val);
			      }
			}
		      cur3 = cur3->next;
		    }
		}
	      else if (xmlStrcasecmp (cur2->name, BAD_CAST "Data") == 0)
		{
		  defineMissingIdentifier(main_conf,&tmpmaps);
		  xmlNodePtr cur4 = cur2->children;
		  if(vid==1){
		    // Get every dataEncodingAttributes from a Data node:
		    // mimeType, encoding, schema
		    const char *coms[3] =
		      { "mimeType", "encoding", "schema" };
		    for (l = 0; l < 3; l++){
		      xmlChar *val =
			  xmlGetProp (cur4, BAD_CAST coms[l]);
			if (val != NULL && strlen ((char *) val) > 0){
			  if (tmpmaps->content != NULL)
			    addToMap (tmpmaps->content,coms[l],(char *) val);
			  else
			    tmpmaps->content =
			      createMap (coms[l],(char *) val);
			}
			xmlFree (val);
		    }
		    while (cur4 != NULL){
		      while(cur4 != NULL && 
			    cur4->type != XML_CDATA_SECTION_NODE &&
			    cur4->type != XML_TEXT_NODE &&
			    cur4->type != XML_ELEMENT_NODE)
			cur4=cur4->next;
		      if(cur4!=NULL){
			if (cur4->type == XML_ELEMENT_NODE)
			  {
			    xmlChar *mv;
			    int buffersize;
			    xmlDocPtr doc1 = xmlNewDoc (BAD_CAST "1.0");
			    xmlDocSetRootElement (doc1, cur4);
			    xmlDocDumpFormatMemoryEnc (doc1, &mv,
						       &buffersize,
						       "utf-8", 0);
			    if (tmpmaps->content != NULL)
			      addToMap (tmpmaps->content, "value",
					(char *) mv);
			    else
			      tmpmaps->content =
				createMap ("value", (char *) mv);
			    free(mv);
			  }
			else{
			  if (tmpmaps->content != NULL)
			    addToMap (tmpmaps->content, "value",
				      (char *) cur4->content);
			  else
			    tmpmaps->content =
			      createMap ("value", (char *) cur4->content);
			}
			cur4=cur4->next;
		      }
		    }
		  }

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
				  xmlFree (val);
				}
			      else{
				if(l==0){
				  if (tmpmaps->content != NULL)
				    addToMap (tmpmaps->content, list[l],
					      "string");
				  else
				    tmpmaps->content =
				      createMap (list[l],"string");
				}
			      }
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
				    xmlFree (val);
				  }
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

		      if (getMap(tmpmaps->content,"dataType")==NULL && test!=NULL && strcasecmp (test->value, "base64") != 0)
			{
			  xmlChar *mv = NULL;
			  /*if(cur4!=NULL && cur4->xmlChildrenNode!=NULL)
			    xmlChar *mv = xmlNodeListGetString (doc,
								cur4->xmlChildrenNode,
								1);*/
			  map *ltmp =
			    getMap (tmpmaps->content, "mimeType");
			  if (/*mv == NULL
			      ||*/
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
				     && cur5->type != XML_CDATA_SECTION_NODE)
				cur5 = cur5->next;
			      if (cur5 != NULL
				  && cur5->type != XML_CDATA_SECTION_NODE)
				{
				  xmlDocSetRootElement (doc1, cur5);
				  xmlDocDumpFormatMemoryEnc (doc1, &mv,
							     &buffersize,
							     "utf-8", 0);
				  xmlFreeDoc (doc1);
				}
			      else
				{
				  if (cur5 != NULL
				      && cur5->type == XML_CDATA_SECTION_NODE){
				    xmlDocPtr doc2 = xmlReadMemory((const char*)cur5->content,xmlStrlen(cur5->content),
								   "input_content.xml", NULL, XML_PARSE_RECOVER);
				    xmlDocDumpFormatMemoryEnc (doc2, &mv,
							       &buffersize,
							       "utf-8", 0);
				    xmlFreeDoc (doc2);
				  }
				}
			      addIntToMap (tmpmaps->content, "size",
					   buffersize);
			    }else{

			    if(xmlStrcasecmp
			       (cur4->name, BAD_CAST "BoundingBoxData") == 0){
			      xmlDocPtr doc1 = xmlNewDoc(BAD_CAST "1.0");
			      int buffersize;
			      xmlDocSetRootElement(doc1,cur4);
			      xmlDocDumpFormatMemoryEnc(doc1,&mv,
							&buffersize,
							"utf-8",0);
			      addIntToMap (tmpmaps->content, "size",
					   buffersize);
			      xmlParseBoundingBox(main_conf,&tmpmaps->content,doc1);
			    }else{
			      xmlNodePtr cur5 = cur4->children;
			      while (cur5 != NULL
				     && cur5->type != XML_ELEMENT_NODE
				     && cur5->type != XML_TEXT_NODE
				     && cur5->type != XML_CDATA_SECTION_NODE)
				cur5 = cur5->next;
			      if (cur5 != NULL
				  && cur5->type != XML_CDATA_SECTION_NODE
				  && cur5->type != XML_TEXT_NODE)
				{
				  xmlDocPtr doc1 = xmlNewDoc (BAD_CAST "1.0");
				  int buffersize;
				  xmlDocSetRootElement (doc1, cur5);
				  xmlDocDumpFormatMemoryEnc (doc1, &mv,
							     &buffersize,
							     "utf-8", 0);
				  addIntToMap (tmpmaps->content, "size",
					       buffersize);
				}
			      else if (cur5 != NULL){
				map* handleText=getMapFromMaps(*main_conf,"main","handleText");
				if(handleText!=NULL && strcasecmp(handleText->value,"true")==0 && ltmp!= NULL && strstr(ltmp->value,"text/")!=NULL){
				  xmlChar *tmp = xmlNodeListGetRawString (doc,
									  cur4->xmlChildrenNode,
									  0);
				  addToMap (tmpmaps->content, "value",
					    (char *) tmp);
				  xmlFree (tmp);
				}else{
				  while(cur5!=NULL && cur5->type != XML_CDATA_SECTION_NODE)
				    cur5=cur5->next;
				  xmlFree(mv);
				  if(cur5!=NULL && cur5->content!=NULL){
				    mv=xmlStrdup(cur5->content);
				  }
			 	}
			      }
			    }
			  }
			  if (mv != NULL)
			    {
			      addToMap (tmpmaps->content, "value",
					(char*) mv);
			      xmlFree (mv);
			    }
			}
		      else
			{
			  xmlNodePtr cur5 = cur4->children;
			  while (cur5 != NULL
				 && cur5->type != XML_CDATA_SECTION_NODE)
			    cur5 = cur5->next;
			  if (cur5 != NULL
			      && cur5->type == XML_CDATA_SECTION_NODE)
			    {
			      addToMap (tmpmaps->content,
					"value",
					(char *) cur5->content);
			    }
			  else{
			    if(cur4->xmlChildrenNode!=NULL){
			      xmlChar *tmp = xmlNodeListGetRawString (doc,
								      cur4->xmlChildrenNode,
								      0);
			      addToMap (tmpmaps->content, "value",
					(char *) tmp);
			      xmlFree (tmp);
			    }
			  }
			}

		      cur4 = cur4->next;
		    }
		}
	      cur2 = cur2->next;
	      while (cur2 != NULL && cur2->type != XML_ELEMENT_NODE){
		cur2 = cur2->next;
	      }
	    }
	  if(memory!=NULL && strncasecmp(memory->value,"load",4)!=0)
	    if(getMap(tmpmaps->content,"to_load")==NULL){
	      addToMap(tmpmaps->content,"to_load","false");
	    }
	  {
	    map* test=getMap(tmpmaps->content,"value");
	    if(test==NULL && tmpmaps->child==NULL)
	      addToMap(tmpmaps->content,"value","");
	    maps *testPresence = getMaps (*request_output, tmpmaps->name);
	    maps *cursor=*request_output;
	    while(testPresence == NULL && cursor!=NULL){
	      if(cursor->child!=NULL){
		testPresence = getMaps (cursor->child, tmpmaps->name);
	      }
	      cursor=cursor->next;
	    }
	    if (testPresence != NULL)
	      {
		elements *elem = getElements (s->inputs, tmpmaps->name);
		elements *cursor=s->inputs;
		while(elem == NULL && cursor!=NULL){
		  if(cursor->child!=NULL){
		    elem = getElements (cursor->child, tmpmaps->name);
		  }
		  cursor=cursor->next;
		}
		if (elem != NULL)
		  {
		    if (appendMapsToMaps
			(*main_conf, testPresence, tmpmaps, elem) < 0)
		      {
			return errorException (*main_conf,
					       _("Unable to append maps to maps."),
					       "InternalError",
					       NULL);
		      }
		  }
	      }
	    else{
	      addMapsToMaps (request_output, tmpmaps);
	    }
	  }
	  freeMaps (&tmpmaps);
	  free (tmpmaps);
	  tmpmaps = NULL;
	}
    }
  return 1;
}

/**
 * Parse a BoundingBoxData node
 *
 * http://schemas.opengis.net/ows/1.1.0/owsCommon.xsd: BoundingBoxType
 * 
 * A map to store boundingbox information will contain:
 *  - LowerCorner : double double (minimum within this bounding box)
 *  - UpperCorner : double double (maximum within this bounding box)
 *  - crs : URI (Reference to definition of the CRS)
 *  - dimensions : int 
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request_inputs the map storing KVP raw value 
 * @param doc the xmlDocPtr containing the BoudingoxData node
 * @return a map containing all the bounding box keys
 */
int xmlParseBoundingBox(maps** main_conf,map** current_input,xmlDocPtr doc){
  xmlNode *root_element = xmlDocGetRootElement(doc);
  for(xmlAttrPtr attr = root_element->properties; NULL != attr; attr = attr->next){
    xmlChar *val = xmlGetProp (root_element, BAD_CAST attr->name);
    addToMap(*current_input,(char*)attr->name,(char*)val);
    xmlFree(val);
    xmlNodePtr cur = root_element->children;
    while(cur!=NULL && cur->type != XML_ELEMENT_NODE)
      cur=cur->next;
    while(cur!=NULL && cur->type==XML_ELEMENT_NODE){
      xmlChar *val =
	xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      addToMap(*current_input,(char*)cur->name,(char*)val);
      cur=cur->next;
      xmlFree(val);
      while(cur!=NULL && cur->type != XML_ELEMENT_NODE)
	cur=cur->next;
    }
  }
  return 0;
}

/**
 * Parse outputs from XML nodes and store them in a maps (WPS version 2.0.0).
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request_inputs the map storing KVP raw value 
 * @param request_output the maps to store the KVP pairs 
 * @param doc the xmlDocPtr containing the original request
 * @param cur the xmlNodePtr corresponding to the ResponseDocument or RawDataOutput XML node
 * @param raw true if the node is RawDataOutput, false in case of ResponseDocument
 * @return 0 on success, -1 on failure
 */
int xmlParseOutputs2(maps** main_conf,map** request_inputs,maps** request_output,xmlDocPtr doc,xmlNodeSet* nodes){
  int k = 0;
  int l = 0;
  for (k=0; k < nodes->nodeNr; k++){
    maps *tmpmaps = NULL;
    xmlNodePtr cur = nodes->nodeTab[k];
    if (cur->type == XML_ELEMENT_NODE){
      maps *tmpmaps = NULL;
      xmlChar *val = xmlGetProp (cur, BAD_CAST "id");
      if(val!=NULL)
	tmpmaps = createMaps((char *)val);
      else
	tmpmaps = createMaps("unknownIdentifier");
      const char ress[4][13] =
	{ "mimeType", "encoding", "schema", "transmission" };
      xmlFree (val);
      for (l = 0; l < 4; l++){
	val = xmlGetProp (cur, BAD_CAST ress[l]);
	if (val != NULL && strlen ((char *) val) > 0)
	  {
	    if (tmpmaps->content != NULL)
	      addToMap (tmpmaps->content, ress[l],
			(char *) val);
	    else
	      tmpmaps->content =
		createMap (ress[l], (char *) val);
	    if(l==3 && strncasecmp((char*)val,"reference",xmlStrlen(val))==0)
	      addToMap (tmpmaps->content,"asReference","true");
	  }
	xmlFree (val);
      }
      if(cur->children!=NULL){
	xmlNodePtr ccur = cur->children;
	while (ccur != NULL){
	  if(ccur->type == XML_ELEMENT_NODE){
	    char *xpathExpr=(char*)malloc(66+strlen(tmpmaps->name));
	    sprintf(xpathExpr,"/*/*[local-name()='Output' and @id='%s']/*[local-name()='Output']",tmpmaps->name);	    
	    xmlXPathObjectPtr tmpsptr = extractFromDoc (doc, xpathExpr);
	    xmlNodeSet* cnodes = tmpsptr->nodesetval;
	    xmlParseOutputs2(main_conf,request_inputs,&tmpmaps->child,doc,cnodes);
	    xmlXPathFreeObject (tmpsptr);
	    free(xpathExpr);
	    break;
	  }
	  ccur = ccur->next;
	}
      }
      if (*request_output == NULL){
	*request_output = dupMaps(&tmpmaps);
      }
      else{
	addMapsToMaps(request_output,tmpmaps);
      }
      freeMaps(&tmpmaps);
      free(tmpmaps);
    }
  }
  return 0;
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

	  maps *tmpmaps = createMaps("unknownIdentifier");
	  if (tmpmaps == NULL)
	    {
	      return errorException (*main_conf, _("Unable to allocate memory"),
				     "InternalError", NULL);
	    }

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
				
	    maps *tmpmaps = createMaps("unknownIdentifier"); // one per Output node
	    if (tmpmaps == NULL) {
	      return errorException (*main_conf,
				     _
				     ("Unable to allocate memory"),
				     "InternalError", NULL);
	    }
				
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
 * Parse XML request and store information in maps.
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

  map* version=getMapFromMaps(*main_conf,"main","rversion");
  int vid=getVersionId(version->value);

  xmlInitParser ();
  xmlDocPtr doc = xmlReadMemory (post, cgiContentLength, "input_request.xml", NULL, XML_PARSE_RECOVER);

  /**
   * Extract Input nodes from the XML Request.
   */
  xmlXPathObjectPtr tmpsptr =
    extractFromDoc (doc, (vid==0?"/*/*/*[local-name()='Input']":"/*/*[local-name()='Input']"));
  xmlNodeSet *tmps = tmpsptr->nodesetval;
  if(tmps==NULL || xmlParseInputs(main_conf,s,inputs,doc,tmps,hInternet)<0){
    xmlXPathFreeObject (tmpsptr);
    xmlFreeDoc (doc);
    xmlCleanupParser ();
    return -1;
  }
  xmlXPathFreeObject (tmpsptr);

  if(vid==1){
    tmpsptr =
      extractFromDoc (doc, "/*[local-name()='Execute']");
    bool asRaw = false;
    tmps = tmpsptr->nodesetval;
    if(tmps->nodeNr > 0){
      int k = 0;
      for (k=0; k < tmps->nodeNr; k++){
	maps *tmpmaps = NULL;
	xmlNodePtr cur = tmps->nodeTab[k];
	if (cur->type == XML_ELEMENT_NODE){
	  xmlChar *val = xmlGetProp (cur, BAD_CAST "mode");
	  if(val!=NULL)
	    addToMap(*request_inputs,"mode",(char*)val);
	  else
	    addToMap(*request_inputs,"mode","auto");
	  xmlFree(val);
	  val = xmlGetProp (cur, BAD_CAST "response");
	  if(val!=NULL){
	    addToMap(*request_inputs,"response",(char*)val);
	    if(strncasecmp((char*)val,"raw",xmlStrlen(val))==0)
	      addToMap(*request_inputs,"RawDataOutput","");
	    else
	      addToMap(*request_inputs,"ResponseDocument","");
	  }
	  else{
	    addToMap(*request_inputs,"response","document");
	    addToMap(*request_inputs,"ResponseDocument","");
	  }
	  xmlFree(val);
	}
      }
    }
    xmlXPathFreeObject (tmpsptr);
    tmpsptr =
      extractFromDoc (doc, "/*/*[local-name()='Output']");
    tmps = tmpsptr->nodesetval;
    if(tmps->nodeNr > 0){
      if(xmlParseOutputs2(main_conf,request_inputs,outputs,doc,tmps)<0){
	xmlXPathFreeObject (tmpsptr);
	xmlFreeDoc (doc);
	xmlCleanupParser ();
	return -1;
      }
    }
  }
  else{
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
  }
  xmlXPathFreeObject (tmpsptr);
  xmlFreeDoc (doc);
  xmlCleanupParser ();
  return 1;
}

/**
 * Parse request and store information in maps.
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

  map* errI0=NULL;
  runHttpRequests (main_conf, request_inputs, hInternet,&errI0);
  if(errI0!=NULL){
    printExceptionReportResponse (*main_conf, errI0);
    InternetCloseHandle (hInternet);
    return -1;
  }
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
              char *storageNameOnServer;
              char *fileNameOnServer;
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
		  fileNameOnServer=(char*)malloc((strlen(name) - t - 1 )*sizeof(char));
                  strcpy (fileNameOnServer, name + t + 1);

		  storageNameOnServer=(char*)malloc((strlen(path->value) + strlen(fileNameOnServer) + 2)*sizeof(char));
                  sprintf (storageNameOnServer, "%s/%s", path->value,
                           fileNameOnServer);
#ifdef DEBUG
                  fprintf (stderr, "Name on server %s\n",
                           storageNameOnServer);
                  fprintf (stderr, "fileNameOnServer: %s\n",
                           fileNameOnServer);
#endif
                  targetFile =
                    zOpen (storageNameOnServer, O_RDWR | O_CREAT | O_TRUNC,
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
                  zClose (targetFile);
		  free(fileNameOnServer);
		  free(storageNameOnServer);
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


/**
 * Verify if a parameter value is valid.
 * 
 * @param request the request map
 * @param res the error map potentially generated
 * @param toCheck the parameter to use
 * @param avalues the acceptable values (or null if testing only for presence)
 * @param mandatory verify the presence of the parameter if mandatory > 0 
 */
void checkValidValue(map* request,map** res,const char* toCheck,const char** avalues,int mandatory){
  map* lres=*res;
  map* r_inputs = getMap (request,toCheck);
  if (r_inputs == NULL){
    if(mandatory>0){
      const char *replace=_("Mandatory parameter <%s> was not specified");
      char *message=(char*)malloc((strlen(replace)+strlen(toCheck)+1)*sizeof(char));
      sprintf(message,replace,toCheck);
      if(lres==NULL){
	lres=createMap("code","MissingParameterValue");
	addToMap(lres,"text",message);
	addToMap(lres,"locator",toCheck);       
      }else{
	int length=1;
	map* len=getMap(lres,"length");
	if(len!=NULL){
	  length=atoi(len->value);
	}
	setMapArray(lres,"text",length,message);
	setMapArray(lres,"locator",length,toCheck);
	setMapArray(lres,"code",length,"MissingParameterValue");
      }
      free(message);
    }
  }else{
    if(avalues==NULL)
      return;
    int nb=0;
    int hasValidValue=-1;
    if(strncasecmp(toCheck,"Accept",6)==0){
      char *tmp=zStrdup(r_inputs->value);
      char *pToken,*saveptr;
      pToken=strtok_r(tmp,",",&saveptr);
      while(pToken!=NULL){
	while(avalues[nb]!=NULL){
	  if(strcasecmp(avalues[nb],pToken)==0){
	    hasValidValue=1;
	    break;
	  }
	  nb++;
	}
	pToken=strtok_r(NULL,",",&saveptr);
      }
      free(tmp);
    }else{
      while(avalues[nb]!=NULL){
	if(strcasecmp(avalues[nb],r_inputs->value)==0){
	  hasValidValue=1;
	  break;
	}
	nb++;
      }
    }
    if(hasValidValue<0){
      const char *replace=_("The value <%s> was not recognized, %s %s the only acceptable value.");
      nb=0;
      char *vvalues=NULL;
      const char* num=_("is");
      while(avalues[nb]!=NULL){
	char *tvalues;
	if(vvalues==NULL){
	  vvalues=(char*)malloc((strlen(avalues[nb])+3)*sizeof(char));
	  sprintf(vvalues,"%s",avalues[nb]);
	}
	else{
	  tvalues=zStrdup(vvalues);
	  vvalues=(char*)realloc(vvalues,(strlen(tvalues)+strlen(avalues[nb])+3)*sizeof(char));
	  sprintf(vvalues,"%s, %s",tvalues,avalues[nb]);
	  free(tvalues);
	  num=_("are");
	}
	nb++;
      }
      char *message=(char*)malloc((strlen(replace)+strlen(num)+strlen(vvalues)+strlen(toCheck)+1)*sizeof(char));
      sprintf(message,replace,toCheck,vvalues,num);
      const char *code="VersionNegotiationFailed";
      code="InvalidParameterValue";
      const char *locator=toCheck;
      if( strncasecmp(toCheck,"version",7)==0 ||
	  strncasecmp(toCheck,"AcceptVersions",14)==0 )
	code="VersionNegotiationFailed";
      if( strncasecmp(toCheck,"request",7)==0){
	code="OperationNotSupported";
	locator=r_inputs->value;
      }
      if(lres==NULL){
	lres=createMap("code",code);
	addToMap(lres,"text",message);
	addToMap(lres,"locator",locator);       
      }else{
	int length=1;
	map* len=getMap(lres,"length");
	if(len!=NULL){
	  length=atoi(len->value);
	}
	setMapArray(lres,"text",length,message);
	setMapArray(lres,"locator",length,locator);
	setMapArray(lres,"code",length,code);
      }
    }
  }
  if(lres!=NULL){
    *res=lres;
  }
}

/**
 * Parse cookie contained in request headers.
 *
 * @param conf the conf maps containinfg the main.cfg
 * @param cookie the 
 */
void parseCookie(maps** conf,const char* cookie){
  char* tcook=zStrdup(cookie);
  char *token, *saveptr;
  token = strtok_r (tcook, "; ", &saveptr);
  maps* res=createMaps("cookies");
  while (token != NULL){
    char *token1, *saveptr1, *name;
    int i=0;
    token1 = strtok_r (token, "=", &saveptr1);
    while (token1 != NULL){
      if(i==0){
	name=zStrdup(token1);
	i++;
      }
      else{
	if(res->content==NULL)
	  res->content=createMap(name,token1);
	else
	  addToMap(res->content,name,token1);
	free(name);
	name=NULL;
	i=0;
      }
      token1 = strtok_r (NULL, "=", &saveptr1);
    }
    if(name!=NULL)
      free(name);
    token = strtok_r (NULL, "; ", &saveptr);
  }
  addMapsToMaps(conf,res);
  freeMaps(&res);
  free(res);
  free(tcook);
}
