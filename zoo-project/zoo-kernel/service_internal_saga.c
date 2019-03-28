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

#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <wx/string.h>
#include <wx/app.h>
#include <api_core.h>
#include <data_manager.h>
#include <saga_api.h>
#define _ZOO_SAGA
#include "service_internal_saga.h"
#include "server_internal.h"
#include "mimetypes.h"

/**
 * Global SAGA-GIS output counter 
 */
int sagaOutputCounter=0;

/**
 * Observer used to access the ongoing status of a running OTB Application
 */
class SagaWatcher
{
 public:
  static int Callback(TSG_UI_Callback_ID ID, CSG_UI_Parameter &Param_1, CSG_UI_Parameter &Param_2);
  /** 
   * Define the message value
   *
   * @param conf the maps pointer to copy
   */
  static void SetMessage(const char *mess)
  {
    FreeMessage();
    message=zStrdup(mess);
  }
  /** 
   * Free the message value
   *
   */
  static void FreeMessage()
  {
    if(message!=NULL)
      free(message);
    message=NULL;
  }
  /** 
   * Copy the original conf in the m_conf property
   *
   * @param conf the maps pointer to copy
   */
  void SetConf(maps **conf)
  {
    m_conf=dupMaps(conf);
  }
  /**  
   * Get Configuration maps (m_conf)
   * @return the m_conf property
   */
  const maps& GetConf()
  {
    return *m_conf;
  }
  /**  
   * Free Configuration maps (m_Conf)
   */
  void FreeConf(){
    freeMaps(&m_conf);
    free(m_conf);
  }
 private:
  /** Main conf maps */
  static maps* m_conf;
  /** Status */
  static int status;
  /** Message */
  static char* message;
};

maps* SagaWatcher::m_conf;
char* SagaWatcher::message=zStrdup("No message left");
int SagaWatcher::status=1;

/**
 * The callback function called at any SAGA-GIS module step
 *
 * @param id a TSG_UI_Callback_ID as defined in api_core.h (line 1290)
 * @param param1 
 * @param param2
 */
int
SagaWatcher::
Callback(TSG_UI_Callback_ID id, CSG_UI_Parameter &param1, CSG_UI_Parameter &param2)
{

  int res = 1;
  switch( id )
    {
    default:
      return 0;
      break;

    case CALLBACK_DLG_ERROR:
      return 1;
      break;

    case CALLBACK_DLG_PARAMETERS:
    case CALLBACK_PROCESS_SET_OKAY:
    case CALLBACK_DATAOBJECT_COLORS_GET:
    case CALLBACK_DATAOBJECT_COLORS_SET:
    case CALLBACK_DATAOBJECT_PARAMS_GET:
    case CALLBACK_DATAOBJECT_PARAMS_SET:
    case CALLBACK_DATAOBJECT_UPDATE:
    case CALLBACK_DATAOBJECT_SHOW:
    case CALLBACK_DLG_CONTINUE:
    case CALLBACK_PROCESS_SET_READY:
    case CALLBACK_PROCESS_GET_OKAY:
      return res;
      break;

    case CALLBACK_PROCESS_SET_PROGRESS:
      {
	int cPercent= param2.Number != 0.0 ? 1 + (int)(100.0 * param1.Number / param2.Number) : 100 ;
	if( cPercent != status ){
	  status=cPercent;
	}else
	  return res;
      }
      break;

    case CALLBACK_PROCESS_SET_TEXT:
      SetMessage(param1.String.b_str());
      break;

    case CALLBACK_MESSAGE_ADD:
      SetMessage(param1.String.b_str());
      break;

    case CALLBACK_MESSAGE_ADD_ERROR:
      SetMessage(param1.String.b_str());
      break;

    case CALLBACK_MESSAGE_ADD_EXECUTION:
      SetMessage(param1.String.b_str());
      break;

    case CALLBACK_DLG_MESSAGE:
      SetMessage((param2.String + ": " + param1.String).b_str());
      break;

    case CALLBACK_DATAOBJECT_ADD:
      if(SG_Get_Data_Manager().Add((CSG_Data_Object *)param1.Pointer))
	res = 1 ;
      else
	res = 0;
      return res;
      break;

    }
  updateStatus(m_conf,status,message);
  return( res );
}

TSG_PFNC_UI_Callback Get_Callback (SagaWatcher watcher){
  return( &(watcher.Callback) );
}


/**
 * Get the default file extension for SAGA-GIS parameter type.
 * Extensions are the following:
 *  - sgrd for grid and data_object
 *  - shp for shapes and tin
 *  - csv for tables
 *  - spc for points
 *
 * @param param a SAGA-GIS Parameter
 */ 
const char* sagaGetDefaultExt(CSG_Parameter* param){
  if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("grid"))
     || CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("data_object"))){
    return "sgrd";
  }
  else if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("shapes")) ||
	  CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("tin"))){
    return "shp";
  }
  else if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("table"))){
    return "csv";
  }
  else if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("points"))){
    return "spc";
  }
  return "unknown";
}

/**
 * Load a datasource in the SAGA-GIS Data Manager.
 *
 * @param param a SAGA-GIS Parameter
 * @param arg the arguments map passed to a SAGA-GIS module
 * @return false on failure, true in case of success
 */
bool sagaLoadInput(CSG_Parameter* param,map* arg){
  if(!param->is_Input() || !param->is_Enabled()){
    return true;
  }
  map* carg=getMap(arg,CSG_String(param->Get_Identifier()).b_str());
  if(carg!=NULL){
    wxString fileName(carg->value);
    if(param->is_DataObject()){
      // In case it there is a single datasource
      if(!SG_Get_Data_Manager().Find(&fileName) && !SG_Get_Data_Manager().Add(&fileName) && !param->is_Optional() ){
	return false;
      }
      fprintf(stderr,"%s %s\n",carg->name,carg->value);
      fflush(stderr);
      return( param->Set_Value(SG_Get_Data_Manager().Find(&fileName,false)) );
    }
    else
      if(param->is_DataObject_List()){
	// In case there are multiple datasources
	param->asList()->Del_Items();
	wxString fileNames(fileName);
	while( fileNames.Length() > 0 ){
	  fileName = fileNames.BeforeFirst(';').Trim(false);
	  fileNames = fileNames.AfterFirst (';');	    
	  if( !SG_Get_Data_Manager().Find(&fileName) ){
	    SG_Get_Data_Manager().Add(&fileName);
	  }
	  param->asList()->Add_Item(SG_Get_Data_Manager().Find(&fileName,false));
	}
      }
  }
  return true;
}

/**
 * Extract all SAGA-GIS parameters from a parameters list and set its values to
 * the one defined in the map.
 *
 * @parap params the parameters list
 * @parap argument the argument map containing the value to use
 * @return true in success, false in other case
 */
bool sagaSetParameters(CSG_Parameters *params,map* argument){

  int pc=params->Get_Count();
  params->Restore_Defaults();

  for(int k=0;k<pc;k++){
    CSG_Parameter * param=params->Get_Parameter(k);
    if( param->is_Output() ){
      map* omap=getMap(argument,CSG_String(param->Get_Identifier()).b_str());
      if( param->is_DataObject() && param->is_Optional() && !param->asDataObject() && omap!=NULL){
	param->Set_Value(DATAOBJECT_CREATE);
      }
    }
    else
      if( param->is_Option() && !param->is_Information() ){
	map* inmap=getMap(argument,CSG_String(param->Get_Identifier()).b_str());
	if(inmap!=NULL){
	    switch( param->Get_Type() ){
	    case PARAMETER_TYPE_Bool:
	      if(strncasecmp(inmap->value,"true",4)==0 || strncasecmp(inmap->value,"1",1)==0){
		param->Set_Value(1);
	      }else
		param->Set_Value(0);
	      break;
	    case PARAMETER_TYPE_Parameters:
	      // TODO: nested inputs gesture
	      break;
	    case PARAMETER_TYPE_Int:
	      param->Set_Value((int)strtol(inmap->value,NULL,10));
	      break;
	    case PARAMETER_TYPE_Double:
	    case PARAMETER_TYPE_Degree:
	      param->Set_Value((double)strtod(inmap->value,NULL));
	      break;
	    case PARAMETER_TYPE_String:
	      param->Set_Value(CSG_String(inmap->value));
	      break;
	    case PARAMETER_TYPE_FilePath:
	      param->Set_Value(CSG_String(inmap->value));
	      break;
	    case PARAMETER_TYPE_FixedTable:
	      {
		CSG_Table Table(inmap->value);
		param->asTable()->Assign_Values(&Table);
	      }
	      break;
	    case PARAMETER_TYPE_Choice:
	      {
		int val=(int)strtol(inmap->value,(char**)NULL,10);
		if(val==0 && strncasecmp(inmap->value,"0",1)!=0)
		  param->Set_Value(CSG_String(inmap->value));
		else
		  param->Set_Value(val);
	      }
	      break;
	    default:
	      break;
	    }
	}else{
	  if(param->Get_Type()==PARAMETER_TYPE_Range){
	    inmap=getMap(argument,(CSG_String(param->Get_Identifier())+"_MIN").b_str());
	    if(inmap!=NULL)
#if SAGA_MAJOR_VERSION == 2	      
	      param->asRange()->Set_LoVal(strtod(inmap->value,NULL));
#else
	      param->asRange()->Set_Min(strtod(inmap->value,NULL));
#endif	    
	    inmap=getMap(argument,(CSG_String(param->Get_Identifier())+"_MAX").b_str());
	    if(inmap!=NULL)
#if SAGA_MAJOR_VERSION == 2	      
	      param->asRange()->Set_HiVal(strtod(inmap->value,NULL));	    
#else
	      param->asRange()->Set_Max(strtod(inmap->value,NULL));
#endif	    
	  }
	  if(inmap==NULL){
	    param->Restore_Default();
	  }
	}
      }
  }

  for(int k=0;k<pc;k++){
    CSG_Parameter * param=params->Get_Parameter(k);
    if( param->is_Input() )
      if(!sagaLoadInput(param,argument)){
	fprintf(stderr,"%s %d \n",__FILE__,__LINE__);
	return false;
      }
  }
  return true;
}

/**
 * Save all values outputed by a SAGA-GIS module invocation to files
 *
 * @param params the parameters list
 * @param main_conf the conf maps containing the main.cfg settings
 * @param outputs the maps to set the generated_file for each output
 */
bool sagaSaveOutputs(CSG_Parameters *params,maps* main_conf,maps** outputs)
{
  for(int j=0; j<params->Get_Count(); j++)
    {
      CSG_Parameter *param = params->Get_Parameter(j);
      maps* cMaps=getMaps(*outputs,CSG_String(param->Get_Identifier()).b_str());
      // Specific TIN case
      if(cMaps==NULL && CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("tin")))
	cMaps=getMaps(*outputs,(CSG_String(param->Get_Identifier())+"_POINTS").b_str());
      if(cMaps!=NULL){
	map* tmpPath=getMapFromMaps(main_conf,"main","tmpPath");
	map* sid=getMapFromMaps(main_conf,"lenv","usid");
	const char *file_ext=sagaGetDefaultExt(param);

	if( param->is_Input() )
	  {
	    if( param->is_DataObject() )
	      {
		CSG_Data_Object *pObject = param->asDataObject();
		
		if( pObject && pObject->is_Modified() && SG_File_Exists(pObject->Get_File_Name()) )
		  {
		    pObject->Save(pObject->Get_File_Name());
		    addToMap(cMaps->content,"generated_file",CSG_String(pObject->Get_File_Name()).b_str());
		  }
	      }
	    
	    else if( param->is_DataObject_List() )
	      {
		for(int i=0; i<
#if SAGA_MAJOR_VERSION == 2
		      param->asList()->Get_Count()
#else
		      param->asList()->Get_Data_Count()
#endif		      
		      ; i++)
		  {
		    CSG_Data_Object *pObject =
#if SAGA_MAJOR_VERSION == 2		      
		      param->asList()->asDataObject(i)
#else
		      param->asList()->Get_Data(i)
#endif		      
		      ;
		    
		    if( pObject->is_Modified() && SG_File_Exists(pObject->Get_File_Name()) )
		      {
			pObject->Save(pObject->Get_File_Name());
			setMapArray(cMaps->content,"generated_file",i,CSG_String(pObject->Get_File_Name()).b_str());
		      }
		  }
	      }
	  }
	else
	  if( param->is_Output() )
	    {
	      char *realFileName=(char*)malloc((strlen(file_ext)+strlen(sid->value)+strlen(cMaps->name)+14)*sizeof(char));
	      char *fullFileName=(char*)malloc((strlen(file_ext)+strlen(sid->value)+strlen(cMaps->name)+strlen(tmpPath->value)+16)*sizeof(char));
	      sprintf(realFileName,"Output_%s_%s_%d",cMaps->name,sid->value,sagaOutputCounter);
	      sprintf(fullFileName,"%s/Output_%s_%s_%d.%s",tmpPath->value,cMaps->name,sid->value,sagaOutputCounter,file_ext);
	      sagaOutputCounter+=1;
	      wxString fileName(fullFileName);
	      addToMap(cMaps->content,"generated_name",realFileName);
	      free(realFileName);
	      free(fullFileName);

	      if( param->is_DataObject() )
		{
		  if( param->asDataObject() )
		    {
		      param->asDataObject()->Save(&fileName);
		      addToMap(cMaps->content,"generated_file",CSG_String(param->asDataObject()->Get_File_Name()).b_str());
		    }
		}
	    
	      else if( param->is_DataObject_List() )
		{
		  CSG_Strings	fileNames;
		
		  while( fileName.Length() > 0 )
		    {
		      CSG_String current_file(&fileName);
		      current_file = current_file.BeforeFirst(';');
		      if( current_file.Length() > 0 )
			{
			  fileNames += current_file;
			  fileName = fileName.AfterFirst(';');
			}
		      else
			{
			  fileNames += &fileName;
			  fileName.Clear();
			}
		    }
#if SAGA_MAJOR_VERSION == 2		  
		  int nFileNames = param->asList()->Get_Count() <= fileNames.Get_Count() ? fileNames.Get_Count() : fileNames.Get_Count() - 1;
		  for(int i=0; i<param->asList()->Get_Count(); i++)
		    {
		      if( i < nFileNames )
			{
			  param->asList()->asDataObject(i)->Save(fileNames[i]);
			}
		      else
			{
			  param->asList()->asDataObject(i)->Save(CSG_String::Format(SG_T("%s_%0*d"),
										    fileNames[fileNames.Get_Count() - 1].c_str(),
										    SG_Get_Digit_Count(param->asList()->Get_Count()),
										    1 + i - nFileNames
										    ));
			}
		      setMapArray(cMaps->content,"generated_file",i,
				  CSG_String(param->asList()->asDataObject(i)->Get_File_Name()).b_str());
		    }
#else
		  int nFileNames = param->asList()->Get_Data_Count() <= fileNames.Get_Count() ? fileNames.Get_Count() : fileNames.Get_Count() - 1;
		  for(int i=0; i<param->asList()->Get_Data_Count(); i++)
		    {
		      if( i < nFileNames )
			{
			  param->asList()->Get_Data(i)->Save(fileNames[i]);
			}
		      else
			{
			  param->asList()->Get_Data(i)->Save(CSG_String::Format(SG_T("%s_%0*d"),
										    fileNames[fileNames.Get_Count() - 1].c_str(),
										    SG_Get_Digit_Count(param->asList()->Get_Data_Count()),
										    1 + i - nFileNames
										    ));
			}
		      setMapArray(cMaps->content,"generated_file",i,
				  CSG_String(param->asList()->Get_Data(i)->Get_File_Name()).b_str());
		    }
		  
#endif		  
		}
	    }
      }
    }
  return( true );
}

/**
 * Invoke the execution of a SAGA-GIS module.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param lib_name the SAGA-GIS library name
 * @param module_name the SAGA-GIS module name
 * @param arguments the map containing the arguments to pass to the module
 * @param outputs default to NULL, contains the maps to fill with the result
 */
int sagaExecuteCmd(maps** main_conf,const char* lib_name,const char* module_name,map* arguments,maps** outputs=NULL){
  int res=SERVICE_FAILED;

#if SAGA_MAJOR_VERSION == 2  
  CSG_Module_Library * library=SG_Get_Module_Library_Manager().Get_Library(CSG_String(lib_name),true);
#else
  CSG_Tool_Library * library=SG_Get_Tool_Library_Manager().Get_Library(CSG_String(lib_name),true);
#endif  
  if( library == NULL){
    char tmp[255];
    sprintf(tmp,"Counld not load the %s SAGA library",lib_name);
    setMapInMaps(*main_conf,"lenv","message",tmp);
    res=SERVICE_FAILED;
    return res;
  }

#if SAGA_MAJOR_VERSION == 2  
  CSG_Module * module=library->Get_Module(atoi(module_name));
#else
  CSG_Tool * module=library->Get_Tool(atoi(module_name));
#endif  
  if(module == NULL){
    char tmp[255];
    sprintf(tmp,"Counld not load the %s module from the %s SAGA library",module_name,lib_name);
    setMapInMaps(*main_conf,"lenv","message",tmp);
    res=SERVICE_FAILED;
    return res;
  }

  CSG_Parameters * params=module->Get_Parameters();
  if(!params){
    char tmp[255];
    sprintf(tmp,"Counld not find any param for the %s module from the %s SAGA library",module_name,lib_name);
    setMapInMaps(*main_conf,"lenv","message",tmp);
    res=SERVICE_FAILED;
    return res;
  }
  
  sagaSetParameters(params,arguments);

  module->Update_Parameter_States();

  bool retval=false;
  if(module->On_Before_Execution()){
    retval=module->Execute();
    module->On_After_Execution();
  }
  
  if(retval && outputs!=NULL){
    sagaSaveOutputs(module->Get_Parameters(),*main_conf,outputs);
    SG_Get_Data_Manager().Delete_Unsaved();
    return SERVICE_SUCCEEDED;
  }

  return SERVICE_FAILED;

}

/**
 * Export a SAGA-GIS Shapes to a file in a specific format (GML,KML,GeoJSON).
 * saga_cmd io_gdal 4 -FILE my.format -SHAPES my.shp -FORMAT XXX
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param in the output maps to fill with the resulting file
 */
int sagaExportOGR(maps** conf, maps** in){
  map* mtype=getMap((*in)->content,"mimeType");
  map* gfile=getMap((*in)->content,"generated_file");
  char* fext=NULL;
  map* arg=NULL;
  if(strncasecmp(mtype->value,"text/xml",8)==0){
    fext=zStrdup("xml");
  }
  else if(strncasecmp(mtype->value,"application/json",16)==0){
    fext=zStrdup("json");
  }
  else{
    fext=zStrdup("kml");
  }
  char* tmpName=(char*)malloc((strlen(gfile->value)+2)*sizeof(char));
  strncpy(tmpName,gfile->value,(strlen(gfile->value)-3)*sizeof(char));
  strncpy(&tmpName[0]+(strlen(gfile->value)-3),fext,(strlen(fext))*sizeof(char));
  tmpName[strlen(fext)+(strlen(gfile->value)-3)]=0;
  arg=createMap("SHAPES",gfile->value);
  addToMap(arg,"FILE",tmpName);
  if(strncasecmp(mtype->value,"text/xml",8)==0){
    addToMap(arg,"FORMAT","GML");
  }
  else if(strncasecmp(mtype->value,"application/json",16)==0){
    addToMap(arg,"FORMAT","GeoJSON");
  }
  else{
#ifdef HAVE_LIBKML
    addToMap(arg,"FORMAT","LIBKML");
#else
    addToMap(arg,"FORMAT","KML");
#endif
  }
  free(fext);
  free(gfile->value);
  gfile->value=zStrdup(tmpName);
  free(tmpName);
  
  sagaExecuteCmd(conf,"io_gdal","4",arg);
  freeMap(&arg);
  free(arg);
}

/**
 * Export a SAGA-GIS pointcloud to a las file.
 * saga_cmd io_shapes_las 0 -POINTS my.spc -FILE my.las
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param in the output maps to fill with the resulting file
 */
void sagaExportPC(maps** conf, maps** in){
  map* mtype=getMap((*in)->content,"mimeType");
  map* gfile=getMap((*in)->content,"generated_file");
  char* fext="las";
  map* arg=NULL;
  char* tmpName=(char*)malloc((strlen(gfile->value)+2)*sizeof(char));
  strncpy(tmpName,gfile->value,(strlen(gfile->value)-3)*sizeof(char));
  strncpy(&tmpName[0]+(strlen(gfile->value)-3),fext,(strlen(fext))*sizeof(char));
  tmpName[strlen(fext)+(strlen(gfile->value)-3)]=0;
  arg=createMap("POINTS",gfile->value);
  addToMap(arg,"FILE",tmpName);
  free(gfile->value);
  gfile->value=zStrdup(tmpName);
  sagaExecuteCmd(conf,"io_shapes_las","0",arg);
  freeMap(&arg);
  free(arg);
  free(tmpName);
}

/**
 * Export a SAGA-GIS Grid to a file in a specific format (tiff,hdr,aa).
 * saga_cmd io_gdal 1 -FILE my.format -GRIDS my.sgrd -FORMAT XXX
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param in the output maps to fill with the resulting file
 */
int sagaExportGDAL(maps** conf, maps** in/*,CSG_Parameter* param*/){
  map* mtype=getMap((*in)->content,"extension");
  map* gfile=getMap((*in)->content,"generated_file");
  char* fext=NULL;
  map* arg;

  if(mtype!=NULL)
    fext=zStrdup(mtype->value);
  else{
    fext=zStrdup("tiff");
  }

  mtype=getMap((*in)->content,"mimeType");
  if(strncasecmp(mtype->value,"image/tiff",10)==0){
    arg=createMap("FORMAT","1");
  }
  else if(strncasecmp(mtype->value,"application/x-ogc-envi",22)==0){
    arg=createMap("FORMAT","ENVI .hdr Labelled");
  }
  else{
    arg=createMap("FORMAT","ARC Digitized Raster Graphics");
  }

  if(gfile!=NULL){
    char* tmpName=(char*)malloc((strlen(gfile->value)+1)*sizeof(char));
    strncpy(tmpName,gfile->value,(strlen(gfile->value)-4)*sizeof(char));
    strncpy(&tmpName[0]+(strlen(gfile->value)-4),fext,(strlen(fext))*sizeof(char));
    tmpName[strlen(fext)+(strlen(gfile->value)-4)]=0;
    addToMap(arg,"FILE",tmpName);
    addToMap(arg,"GRIDS",gfile->value);
    free(tmpName);
    free(fext);
    free(gfile->value);
    map* tmp=getMap(arg,"FILE");
    gfile->value=zStrdup(tmp->value);
    sagaExecuteCmd(conf,"io_gdal","1",arg);
  }
  else{
    // Empty result
    return true;
  }
  freeMap(&arg);
  free(arg);
}

/**
 * Export a SAGA-GIS TIN to a file in a specific format (GML,KML,GeoJSON).
 * Exporting TIN produce 5 separated files (POINTS, CENTER, EDGES, TRIANGLES 
 * and POLYGONS). Even if a client can choose which result it want to have,
 * SAGA-GIS module will be invoked in a way that it will produce in any case
 * each possible outputs. The selection of a specific output is made in the 
 * ZOO-Kernel itself and not specifically at this level.
 * saga_cmd tin_tools 3 -TIN my.shp -POINTS p.shp ...
 *
 * @param conf the conf maps containing the main.cfg settings
 * @param in the output maps to fill with the resulting file
 * @see sagaExportOGR
 */
int sagaExportTIN(maps** conf, maps** in,const char* tname/*,CSG_Parameter* param*/){
  map* mtype=getMap((*in)->content,"mimeType");
  map* gfile=getMap((*in)->content,"generated_file");
  char* fext="shp";
  map* arg=createMap("TIN",gfile->value);

  char* tinOut[5]={
    "POINTS",
    "CENTER",
    "EDGES",
    "TRIANGLES",
    "POLYGONS"
  };
  maps* resouts=NULL;

  int i=0;
  for(i=0;i<5;i++){
    char* tmpName=(char*)malloc((strlen(gfile->value)+strlen(tinOut[i])+4)*sizeof(char));
    strncpy(tmpName,gfile->value,(strlen(gfile->value)-3)*sizeof(char));
    char *tmpSubName=(char*) malloc((strlen(tinOut[i])+3)*sizeof(char));
    sprintf(tmpSubName,"_%s.",tinOut[i]);
    strncpy(&tmpName[0]+(strlen(gfile->value)-4),tmpSubName,(strlen(tmpSubName))*sizeof(char));
    strncpy(&tmpName[0]+(strlen(gfile->value)+strlen(tmpSubName)-4),fext,(strlen(fext))*sizeof(char));
    tmpName[strlen(fext)+(strlen(gfile->value)+strlen(tmpSubName)-4)]=0;

    maps* louts=createMaps(tinOut[i]);
    louts->content=createMap("mimeType","UNKOWN");
    
    addToMap(arg,tinOut[i],tmpName);
    
    free(tmpName);
    if(resouts==NULL)
      resouts=dupMaps(&louts);
    else
      addMapsToMaps(&resouts,louts);
    freeMaps(&louts);
    free(louts);
  }
  
  sagaExecuteCmd(conf,"tin_tools","3",arg,&resouts);

  for(i=0;i<5;i++){
    map* generatedFile=getMapFromMaps(resouts,tinOut[i],"generated_file");
    setMapInMaps(*in,(CSG_String(tname)+"_"+tinOut[i]).b_str(),"generated_file",generatedFile->value);
    maps* cout=getMaps(*in,(CSG_String(tname)+"_"+tinOut[i]).b_str());
    sagaExportOGR(conf,&cout);
  }
  return true;
}

/**
 * Import GDAL Datasource into SAGA-GIS.
 * saga_cmd io_gdal 0 -TRANSFORM 0 -FILES my.format -GRIDS /tmpPath/MyGridXXX.sgrd
 * 
 * @param conf the conf maps containing the main.cfg settings
 * @param in in the inputs maps
 */
int sagaImportGDAL(maps** conf, maps** in){
  map* l=getMap((*in)->content,"length");
  bool shouldClean=false;
  if(l==NULL){
    l=createMap("length","1");
    shouldClean=true;
  }
  int len=strtol(l->value,NULL,10);
  int i=0;
  for(i=0;i<len;i++){
    map* arg=createMap("TRANSFORM","0");
    addToMap(arg,"INTERPOL","4");
    map* v=getMapArray((*in)->content,"cache_file",i);
    if(v!=NULL)
      addToMap(arg,"FILES",v->value);
    addToMap(arg,"GRIDS","");

    maps* louts=createMaps("GRIDS");
    louts->content=createMap("mimeType","UNKOWN");

    sagaExecuteCmd(conf,"io_gdal","0",arg,&louts);

    map* tmp=getMapFromMaps(louts,"GRIDS","generated_file");
    setMapArray((*in)->content,"saga_value",i,tmp->value);

    freeMaps(&louts);
    free(louts);
    freeMap(&arg);
    free(arg);
  }
  if(shouldClean){
    freeMap(&l);
    free(l);
  }
}

/**
 * Import OGR Datasource into SAGA-GIS.
 * saga_cmd io_gdal 3 -SHAPES my.shp -FILES my.format
 * 
 * @param conf the conf maps containing the main.cfg settings
 * @param in in the inputs maps
 */
int sagaImportOGR(maps** conf, maps** in){
  char *ext;
  map* arg;
  map* l=getMap((*in)->content,"length");
  bool shouldClean=false;
  if(l==NULL){
    l=createMap("length","1");
    shouldClean=true;
  }
  int len=strtol(l->value,NULL,10);
  int i=0;
  for(i=0;i<len;i++){
    map* v=getMapArray((*in)->content,"cache_file",i);
    arg=createMap("SHAPES","");
    if(v!=NULL)
      addToMap(arg,"FILES",v->value);

    maps* louts=createMaps("SHAPES");
    louts->content=createMap("mimeType","UNKOWN");

    sagaExecuteCmd(conf,"io_gdal","3",arg,&louts);

    map* tmp=getMapFromMaps(louts,"SHAPES","generated_file");
    setMapArray((*in)->content,"saga_value",i,tmp->value);

    freeMaps(&louts);
    free(louts);
    freeMap(&arg);
    free(arg);
  }
  if(shouldClean){
    freeMap(&l);
    free(l);
  }
}

/**
 * Import TIN into SAGA-GIS. Calling this function suppose that sagaImportOGR
 * was called first.
 * saga_cmd tin_tools 2 -SHAPES myShapes.shp -TIN myTin.shp
 * 
 * @param conf the conf maps containing the main.cfg settings
 * @param in in the inputs maps
 * @see sagaImportOGR
 */
bool sagaImportTIN(maps** conf, maps** in){
  char *ext;
  map* arg;
  map* l=getMap((*in)->content,"length");
  bool shouldClean=false;
  if(l==NULL){
    l=createMap("length","1");
    shouldClean=true;
  }
  int len=strtol(l->value,NULL,10);
  int i=0;
  for(i=0;i<len;i++){
    map* v=getMapArray((*in)->content,"saga_value",i);
    arg=createMap("TIN","");
    if(v!=NULL)
      addToMap(arg,"SHAPES",v->value);
    maps* louts=createMaps("TIN");
    louts->content=createMap("mimeType","UNKOWN");
    sagaExecuteCmd(conf,"tin_tools","2",arg,&louts);
    map* tmp=getMapFromMaps(louts,"TIN","generated_file");
    v=getMapArray((*in)->content,"saga_value",i);
    if(tmp!=NULL){
      if(v!=NULL){
	free(v->value);
	v->value=zStrdup(tmp->value);
      }
      else
	setMapArray((*in)->content,"saga_value",i,tmp->value);
    }
    freeMaps(&louts);
    free(louts);
    freeMap(&arg);
    free(arg);
  }
  if(shouldClean){
    freeMap(&l);
    free(l);
  }
  return true;
}

/**
 * Import table into SAGA-GIS. 
 * saga_cmd io_table 1 -TABLE myTable -FILENAME myFile -SEPARATOR 2
 * 
 * @param conf the conf maps containing the main.cfg settings
 * @param in in the inputs maps
 */
int sagaImportTable(maps** conf, maps** in){
  char *ext;
  map* arg;
  map* l=getMap((*in)->content,"length");
  bool shouldClean=false;
  if(l==NULL){
    l=createMap("length","1");
    shouldClean=true;
  }
  int len=strtol(l->value,NULL,10);
  int i=0;
  for(i=0;i<len;i++){

    // Create and fill arg map
    arg=createMap("SEPARATOR","2");
    addToMap(arg,"TABLE","");
    map* v=getMapArray((*in)->content,"cache_file",i);
    if(v!=NULL)
      addToMap(arg,"FILENAME",v->value);

    // Create the output maps
    maps* louts=createMaps("TABLE");
    louts->content=createMap("mimeType","UNKOWN");

    // Execute the saga command
    sagaExecuteCmd(conf,"io_table","1",arg,&louts);

    // Fetch result and add it to the original map as saga_value
    map* tmp=getMapFromMaps(louts,"TABLE","generated_file");
    setMapArray((*in)->content,"saga_value",i,tmp->value);

    // Cleanup
    freeMaps(&louts);
    free(louts);
    freeMap(&arg);
    free(arg);

  }
  // Cleanup if required
  if(shouldClean){
    freeMap(&l);
    free(l);
  }
}

/**
 * Import las file as pointcloud into SAGA-GIS. 
 * saga_cmd io_shapes_las 1 -POINTS my.spc -FILENAME my.las
 * 
 * @param conf the conf maps containing the main.cfg settings
 * @param in in the inputs maps
 */
int sagaImportPC(maps** conf, maps** in){
  char *ext;
  map* arg;
  map* l=getMap((*in)->content,"length");
  bool shouldClean=false;
  if(l==NULL){
    l=createMap("length","1");
    shouldClean=true;
  }
  int len=strtol(l->value,NULL,10);
  int i=0;
  for(i=0;i<len;i++){

    // Create and fill arg map
    arg=createMap("POINTS","");
    map* v=getMapArray((*in)->content,"cache_file",i);
    if(v!=NULL)
      addToMap(arg,"FILES",v->value);

    // Create the output maps
    maps* louts=createMaps("POINTS");
    louts->content=createMap("mimeType","UNKOWN");

    // Execute the saga command
    sagaExecuteCmd(conf,"io_shapes_las","1",arg,&louts);

    // Fetch result and add it to the original map as saga_value
    map* tmp=getMapFromMaps(louts,"POINTS","generated_file");
    setMapArray((*in)->content,"saga_value",i,tmp->value);

    // Cleanup
    freeMaps(&louts);
    free(louts);
    freeMap(&arg);
    free(arg);

  }
  // Cleanup if required
  if(shouldClean){
    freeMap(&l);
    free(l);
  }
}

/**
 * Load and invoke a SAGA-GIS module defined in a service metadata definitions.
 * Load all the input data into SAGA-GIS using io_gdal, io_tables and 
 * io_shapes_las for SAGA grids/shapes, tables and pointcloud respectively. 
 * Load and run the module from its library and invoke it using the data 
 * imported in SAGA-GIS at first stage. After the execution, export the outputs
 * to files using io_gdal and io_shapes_las for grids/shapes and pointcloud 
 * respectively.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param inputs the maps containing the inputs
 * @param outputs the maps containing the outputs
 */
int zoo_saga_support(maps** main_conf,map* request,service* s,maps** inputs,maps** outputs){
  int res=SERVICE_FAILED;
  if( !wxInitialize() ){
    fprintf(stderr,"initialisation failed");
    return SERVICE_FAILED;
  }
  setlocale(LC_NUMERIC, "C");
  static bool g_bShow_Messages = false;

  dumpMapsValuesToFiles(main_conf,inputs);

  SagaWatcher watcher=SagaWatcher();
  watcher.SetConf(main_conf);

  SG_Set_UI_Callback(Get_Callback(watcher));

#if SAGA_MAJOR_VERSION == 2  
  int n = SG_Get_Module_Library_Manager().Add_Directory(wxT(MODULE_LIBRARY_PATH),false);
  if( SG_Get_Module_Library_Manager().Get_Count() <= 0 )
#else
  int n = SG_Get_Tool_Library_Manager().Add_Directory(wxT(MODULE_LIBRARY_PATH),false);
  if( SG_Get_Tool_Library_Manager().Get_Count() <= 0 )
#endif
  {
    setMapInMaps(*main_conf,"lenv","message","Could not load any SAGA tool library");
    res=SERVICE_FAILED;
    return res;
  }

  map* serviceProvider=getMap(s->content,"serviceProvider");

  // Load the SAGA-GIS library corresponding to the serviceProvider
#if SAGA_MAJOR_VERSION == 2  
  CSG_Module_Library * library=SG_Get_Module_Library_Manager().Get_Library(CSG_String(serviceProvider->value),true);
#else
  CSG_Tool_Library * library=SG_Get_Tool_Library_Manager().Get_Library(CSG_String(serviceProvider->value),true);
#endif
  if( library == NULL){
    char tmp[255];
    sprintf(tmp,"Counld not load the %s SAGA library",serviceProvider->value);
    setMapInMaps(*main_conf,"lenv","message",tmp);
    res=SERVICE_FAILED;
    return res;
  }
  
  // Load the SAGA-GIS module corresponding to the service name from the library
#if SAGA_MAJOR_VERSION == 2  
  CSG_Module * module=library->Get_Module(atoi(s->name));
#else
  CSG_Tool * module=library->Get_Tool(atoi(s->name));
#endif
  if(module == NULL){
    char tmp[255];
    sprintf(tmp,"Counld not load the %s module from the %s SAGA library",
	    s->name,serviceProvider->value);
    setMapInMaps(*main_conf,"lenv","message",tmp);
    res=SERVICE_FAILED;
    return res;
  }

  // Load all the parameters defined for the module
  CSG_Parameters * params=module->Get_Parameters();
  if(!params){
    char tmp[255];
    sprintf(tmp,"Counld not find any param for the %s module from the %s SAGA library",
	    s->name,serviceProvider->value);
    setMapInMaps(*main_conf,"lenv","message",tmp);
    res=SERVICE_FAILED;
    return res;
  }
  int pc=params->Get_Count();

  // Loop over each inputs to transform raster files to grid when needed, 
  // import tables, shapes or point clouds
  for(int k=0;k<pc;k++){
    CSG_Parameter * param=params->Get_Parameter(k);
    if(param!=NULL && !param->is_Output()){
      maps* inmap=getMaps(*inputs,CSG_String(param->Get_Identifier()).b_str());
      if(inmap!=NULL){
	map* tmp=getMap(inmap->content,"value");
	if(tmp==NULL || strncasecmp(tmp->value,"NULL",4)!=0){
	  if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("grid"))){
	    sagaImportGDAL(main_conf,&inmap);
	  }
	  else if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("tin"))){
	    sagaImportOGR(main_conf,&inmap);
	    sagaImportTIN(main_conf,&inmap);
	  }
	  else if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("shapes"))){
	    sagaImportOGR(main_conf,&inmap);
	  }
	  else{
	    if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("table"))){
	      sagaImportTable(main_conf,&inmap);
	    }
	    else
	      if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("points"))){
		sagaImportPC(main_conf,&inmap);
	      }
	  }
	}
      }
    }
  }

  // Create a map conraining arguments to pass to the SAGA-GIS module
  // Fetch all input value (specifically for data imported into SAGA-GIS)
  maps* inp=*inputs;
  int k=0;
  map* cParams=NULL;
  while(inp!=NULL){
    map* len=getMap(inp->content,"length");
    bool shouldClean=false;
    if(len==NULL){
      len=createMap("length","1");
      shouldClean=true;
    }
    int len0=strtol(len->value,NULL,10);
    int i=0;
    char *cinput=NULL;
    int clen=0;
    for(i=0;i<len0;i++){
      map* val=getMapArray(inp->content,"saga_value",i);
      if(val==NULL)
	val=getMapArray(inp->content,"value",i);
      if(val!=NULL && strncasecmp(val->value,"NULL",4)!=0){
	if(cinput==NULL){
	  cinput=zStrdup(val->value);
	}
	else{
	  cinput=(char*)realloc(cinput,(clen+strlen(val->value)+1)*sizeof(char));
	  strncpy(&cinput[0]+clen,";",1);
	  strncpy(&cinput[0]+(clen+1),val->value,strlen(val->value));
	  clen+=1;
	}
	clen+=strlen(val->value);
	cinput[clen]=0;
      }
    }
    if(cinput!=NULL && strncasecmp(cinput,"NULL",4)!=0){
      if(cParams==NULL)
	cParams=createMap(inp->name,cinput);
      else
	addToMap(cParams,inp->name,cinput);
      free(cinput);
    }
    inp=inp->next;
  }

  // Fetch all output and define a resulting filename
  inp=*outputs;
  map* tmpPath=getMapFromMaps(*main_conf,"main","tmpPath");
  map* sid=getMapFromMaps(*main_conf,"lenv","usid");
  while(inp!=NULL){
    for(int k=0;k<pc;k++){
      CSG_Parameter * param=params->Get_Parameter(k);
      if(CSG_String(param->Get_Identifier()).Cmp(inp->name)==0){
	const char *file_ext=sagaGetDefaultExt(param);
	char *fileName=(char*)malloc((strlen(file_ext)+strlen(sid->value)+strlen(inp->name)+strlen(tmpPath->value)+11)*sizeof(char));
	sprintf(fileName,"%s/Output_%s_%s.%s",tmpPath->value,inp->name,sid->value,file_ext);
	if(cParams==NULL)
	  cParams=createMap(inp->name,fileName);
	else
	  addToMap(cParams,inp->name,fileName);
      }
    }
    inp=inp->next;
  }

  sagaSetParameters(params,cParams);

  module->Update_Parameter_States();
  
  bool retval=false;
  if(module->On_Before_Execution()){
    retval=module->Execute();
    module->On_After_Execution();
  }

  sagaSaveOutputs(params,*main_conf,outputs);

  // Loop over each outputs to transform grid to raster file when needed, 
  // export tables, shapes or point clouds
  for(int k=0;k<pc;k++){
    CSG_Parameter * param=params->Get_Parameter(k);
    if(param!=NULL && param->is_Output()){
      maps* inmap=getMaps(*outputs,CSG_String(param->Get_Identifier()).b_str());
      if(inmap!=NULL){
	if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("grid"))
	   || CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("data_object"))){
	  sagaExportGDAL(main_conf,&inmap);
	}else{
	  if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("shapes"))){
	    sagaExportOGR(main_conf,&inmap);
	  }
	  else{
	    if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("table"))){
	    }
	    else{
	      if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("tin"))){
		sagaExportTIN(main_conf,&inmap,"TIN");
	      }
	      else
		if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("points"))){
		  sagaExportPC(main_conf,&inmap);
	      }
	    }
	  }
	}
      }
      else if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("tin"))){
	sagaExportTIN(main_conf,outputs,CSG_String(param->Get_Identifier()).b_str());
      }
	      
    }
  }

  wxUninitialize();

  return SERVICE_SUCCEEDED;
}
