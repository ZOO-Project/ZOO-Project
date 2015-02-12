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
 *
 * See Ref: http://hg.orfeo-toolbox.org/OTB/ Copyright
 * Some parts of this code are derived from ITK. See ITKCopyright.txt for 
 * details.
 */

#include "service_internal_otb.h"

using namespace otb::Wrapper;

/**
 * The ZooWatcher list
 */
WatcherListType m_WatcherList;
/**
 * A pointer to the conf maps containing the main.cfg settings
 */
maps* m_Conf;

/**
 * The command to create a ZooWatcher and add it to the global m_WatcherList
 */
class MyCommand : public itk::Command
{
 public:
  itkNewMacro( MyCommand );
 public:

  /**
   * The method that defines the action to be taken by the command. 
   *
   * @param caller an itk::Object pointer
   * @param event an itk::EventObject pointer
   */
  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *)caller, event);
  }
 
  /**
   * The method that defines the action to be taken by the command. 
   * Create a new ZooWatcher instance then add it to the m_WatcherList.
   *
   * @param caller a const itk::Object pointer
   * @param event an itk::EventObject pointer
   * @see ZooWatcher,ZooWatcher::SetConf
   */
  void Execute(const itk::Object *caller, const itk::EventObject & event)
  {
    const AddProcessToWatchEvent* eventToWatch = dynamic_cast< const AddProcessToWatchEvent*> ( &event );
    std::string m_CurrentDescription = eventToWatch->GetProcessDescription();
    ZooWatcher * watch = new ZooWatcher(eventToWatch->GetProcess(),
					eventToWatch->GetProcessDescription());
    watch->SetConf(&m_Conf);
    m_WatcherList.push_back(watch);
  }

};

/**
 * Replace all occurence of from by to in a str string
 *
 * @param str the string to transform
 * @param from the string to replace
 * @param to the string used as replacement
 * @return the resulting string 
 */
std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

/**
 * Load and run an OTB Application corresponding to the service by using inputs parameters.
 * Define the m_Conf 
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param real_inputs the maps containing the inputs
 * @param real_outputs the maps containing the outputs
 */
int zoo_otb_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  map* tmp0=getMapFromMaps(*main_conf,"lenv","cwd");
  char *ntmp=tmp0->value;
  map* tmp=NULL;
  int res=-1;

  std::vector<std::string> list = ApplicationRegistry::GetAvailableApplications();
  if (list.size() == 0){
    map* tmps=createMap("text","No OTB Application found.");
    addToMap(tmps,"code","InternalError");
    printExceptionReportResponse(m,tmps);
    freeMap(&tmps);
    free(tmps);
    res=-1;
  }
  else{
    for (std::vector<std::string>::const_iterator it = list.begin(); it != list.end(); ++it){
      if(s->name==*it){
	Application::Pointer m_Application=ApplicationRegistry::CreateApplication(*it);
	if (m_Application.IsNull()){
	  char tmpS[1024];
	  sprintf(tmpS, "The OTB Application %s cannot be loaded.", (*it).c_str());
	  map* tmps=createMap("text",tmpS);
	  addToMap(tmps,"code","InternalError");
	  printExceptionReportResponse(m,tmps);
	  freeMap(&tmps);
	  free(tmps);
	  res=-1;
	}else{
	  // Create Observer on AddProcessToWatchEvent
	  m_Conf=m;
	  MyCommand::Pointer myCommand = MyCommand::New();
	  m_Application->AddObserver(AddProcessToWatchEvent(), myCommand);
	  char tmpS[1024];
	  const std::vector<std::string> appKeyList = m_Application->GetParametersKeys(true);
	  for (unsigned int i = 0; i < appKeyList.size(); i++){
	    const std::string paramKey(appKeyList[i]);
	    std::vector<std::string> values;
	    Parameter::Pointer param = m_Application->GetParameterByKey(paramKey);
	    ParameterType type = m_Application->GetParameterType(paramKey);
	    if (type != ParameterType_Group && paramKey!="inxml" && paramKey!="outxml"){
	      map* test=getMapFromMaps(inputs,paramKey.c_str(),"cache_file");
	      if(test==NULL){
		test=getMapFromMaps(inputs,paramKey.c_str(),"inRequest");
		map* tmpPath=getMapFromMaps(m,"main","tmpPath");
		map* tmpSid=getMapFromMaps(m,"lenv","usid");
		char tmp[1024];
		map* tmpVal=getMapFromMaps(outputs,paramKey.c_str(),"mimeType");
		if(test!=NULL && test->value!=NULL && strncasecmp(test->value,"true",4)==0){
		  test=getMapFromMaps(inputs,paramKey.c_str(),"value");
		  if(type == ParameterType_OutputImage){
		    ImagePixelType outPixType = ImagePixelType_float;
		    if (strncasecmp(test->value,"uint8",5)==0)
		      outPixType = ImagePixelType_uint8;
		    else if (strncasecmp(test->value,"int16",5)==0)
		      outPixType = ImagePixelType_int16;
		    else if (strncasecmp(test->value,"uint16",6)==0)
		      outPixType = ImagePixelType_uint16;
		    else if (strncasecmp(test->value,"int32",5)==0)
		      outPixType = ImagePixelType_int32;
		    else if (strncasecmp(test->value,"uint32",6)==0)
		      outPixType = ImagePixelType_uint32;
		    else if (strncasecmp(test->value,"double",6)==0)
		      outPixType = ImagePixelType_double;
		    const char* ext="tiff";
		    if(tmpVal!=NULL){
		      if(strncasecmp(tmpVal->value,"image/jp2",9)==0)
			 ext="j2k";
		      else
			if(strncasecmp(tmpVal->value,"image/png",9)==0)
			 ext="png";
			else
			  if(strncasecmp(tmpVal->value,"image/jpeg",10)==0)
			    ext="jpeg";
		    }
		    sprintf(tmp,"%s/%s_%s.%s",tmpPath->value,s->name,tmpSid->value,ext);
		    m_Application->SetParameterString(paramKey, tmp);
		    setMapInMaps(inputs,paramKey.c_str(),"generated_file",tmp);
		    dynamic_cast<OutputImageParameter *> (param.GetPointer())->SetPixelType(outPixType);
		  }else{
		    if(test->value!=NULL)
		      m_Application->SetParameterString(paramKey, test->value);
		  }

		}else{
		  if(type == ParameterType_OutputVectorData){
		      char* ext="json";
		      if(tmpVal!=NULL){
			if(strncasecmp(tmpVal->value,"text/xml",8)==0)
			ext="gml";
		      else
			if(strncasecmp(tmpVal->value,"applicaton/json",15)==0)
			  ext="json";
			else
			  if(strncasecmp(tmpVal->value,"application/zip",14)==0)
			    ext="shp";
			  else
			    if(strncasecmp(tmpVal->value,"application/vnd.google-earth.kml+xml",36)==0)
			      ext="kml";
		      }
		      sprintf(tmp,"%s/%s_%s.%s",tmpPath->value,s->name,tmpSid->value,ext);
		      m_Application->SetParameterString(paramKey, tmp);
		      setMapInMaps(inputs,paramKey.c_str(),"generated_file",tmp);
		  }
		  else
		    if(type == ParameterType_OutputFilename){
		      char* ext="txt";
		      if(tmpVal!=NULL){
			if(strncasecmp(tmpVal->value,"text/xml",8)==0)
			  ext="xml";
			else
			  if(strncasecmp(tmpVal->value,"text/csv",15)==0)
			    ext="csv";
			  else
			    if(strncasecmp(tmpVal->value,"application/zip",14)==0)
			      ext="shp";
			    else
			      if(strncasecmp(tmpVal->value,"application/vnd.google-earth.kml+xml",36)==0)
				ext="kml";
			      else
				if(strncasecmp(tmpVal->value,"application/vnd.google-earth.kmz",32)==0){
				  ext="kmz";
				  sprintf(tmp,"%s/%s_%sxt.%s",tmpPath->value,s->name,tmpSid->value,ext);
				  m_Application->SetParameterString(paramKey, tmp);
				  setMapInMaps(outputs,paramKey.c_str(),"expected_generated_file",tmp);
				}

		      }
		      sprintf(tmp,"%s/%s_%s.%s",tmpPath->value,s->name,tmpSid->value,ext);
		      m_Application->SetParameterString(paramKey, tmp);
		      setMapInMaps(inputs,paramKey.c_str(),"generated_file",tmp);
		    }

		}
	      }else{
		if(type == ParameterType_InputImageList){
		  values.push_back(test->value);
		  map* tmpPath=getMapFromMaps(inputs,paramKey.c_str(),"length");
		  if(tmpPath!=NULL){
		    int len=atoi(tmpPath->value);
		    for(int k=1;k<len;k++){
		      char tmp[15];
		      sprintf(tmp,"cache_file_%d",k);
		      map* tmpVal=getMapFromMaps(inputs,paramKey.c_str(),tmp);
		      if(tmpVal!=NULL){
			values.push_back(tmpVal->value);
		      }
		    }
		  }
		  dynamic_cast<InputImageListParameter *> (param.GetPointer())->SetListFromFileName(values);
		}
		else
		  if(type == ParameterType_InputVectorData || type == ParameterType_InputFilename){
		    map* tmpPath=getMapFromMaps(m,"main","tmpPath");
		    map* tmpSid=getMapFromMaps(m,"lenv","sid");
		    char tmp[1024];
		    map* tmpVal=getMapFromMaps(inputs,paramKey.c_str(),"mimeType");
		    char* ext="json";
		    if(tmpVal!=NULL){
		      if(strncasecmp(tmpVal->value,"application/zip",14)==0){
			char tmpName[1024];
			symlink(test->value,ReplaceAll(test->value,".zca",".zip").c_str());
			sprintf(tmpName,"/vsizip/%s",ReplaceAll(test->value,".zca",".zip").c_str());
			char **files=VSIReadDir(tmpName);
			int nFiles = CSLCount( files );
			char tmpSSName[1024];
			sprintf(tmpSSName,"%s/Input_%s_%s",tmpPath->value,s->name,tmpSid->value);
			mkdir(tmpSSName,0777);
			    
			char tmpSName[1024];
			for(int kk=0;kk<nFiles;kk++){
			  sprintf(tmpSName,"%s/%s",tmpName,files[kk]);
			  VSILFILE* fmain=VSIFOpenL(tmpSName, "rb");
			  if(fmain!=NULL){
			    VSIFSeekL(fmain,0,SEEK_END);
			    long count=VSIFTellL(fmain);
			    VSIRewindL(fmain);

			    char *content=(char*) malloc((count+1)*sizeof(char));  
			    VSIFReadL(content,1,count*sizeof(char),fmain);
			  
			    char tmpSSSName[1024];
			    sprintf(tmpSSSName,"%s/%s",tmpSSName,files[kk]);
			    
			    FILE* fx=fopen(tmpSSSName, "wb");
			    fwrite(content,1,count,fx);
			    fclose(fx);
			    VSIFCloseL(fmain);
			    free(content);
			    std::string test1(tmpSSSName);
			    if(test1.find(".shp")!=std::string::npos){
			      setMapInMaps(inputs,paramKey.c_str(),"cache_file",tmpSSSName);
			      test=getMapFromMaps(inputs,paramKey.c_str(),"cache_file");
			    }
			  }
			}
		      }
		    }
		    
		    m_Application->SetParameterString(paramKey, test->value);
		  }
		  else
		    if(type == ParameterType_InputImage
		       || type == ParameterType_ComplexInputImage || type == ParameterType_InputVectorData
		       || type == ParameterType_InputFilename){
		      m_Application->SetParameterString(paramKey, test->value);
		  }		  
	      }
	    }
	    param->SetUserValue(true);
	    m_Application->UpdateParameters();
	  }

	  try{
	    if( m_Application->ExecuteAndWriteOutput() == 0 ){
	      std::vector< std::pair<std::string, std::string> > paramList;
	      paramList = m_Application->GetOutputParametersSumUp();
	      if(paramList.size()>0)
		for( unsigned int i=0; i<paramList.size(); i++){
		  setMapInMaps(outputs,paramList[i].first.c_str(),"value",paramList[i].second.c_str());
		}
	      else{
		const std::vector<std::string> appKeyList = m_Application->GetParametersKeys(true);
		for (unsigned int i = 0; i < appKeyList.size(); i++){
		  const std::string paramKey(appKeyList[i]);
		  std::vector<std::string> values;
		  Parameter::Pointer param = m_Application->GetParameterByKey(paramKey);
		  ParameterType type = m_Application->GetParameterType(paramKey);
		  if (type != ParameterType_Group && paramKey!="inxml" && paramKey!="outxml"
		      && (type == ParameterType_OutputImage || type == ParameterType_OutputFilename
			  || type == ParameterType_OutputVectorData ) ){
		    if(type == ParameterType_OutputImage || type == ParameterType_OutputFilename || type == ParameterType_OutputVectorData){
		      map* test=getMapFromMaps(outputs,paramKey.c_str(),"mimeType");
		      if(test!=NULL && strncasecmp(test->value,"application/zip",15)==0){
			
			test=getMapFromMaps(inputs,paramKey.c_str(),"generated_file");
			char tmpName[1024];
			sprintf(tmpName,"/vsizip/%s",ReplaceAll(test->value,".shp",".zip").c_str());
			VSILFILE* fmain=VSIFOpenL(tmpName, "w");
			FILE * file;
			char *tmp;
			char tmpSName[1024];
			long count;
			
			char *exts[4];
			exts[0]=".shp";
			exts[1]=".shx";
			exts[2]=".dbf";
			exts[3]=".prj";
			for(int c=0;c<4;c++){
			  sprintf(tmpSName,"%s/result%s",tmpName,exts[c]);
			  
			  file=fopen(ReplaceAll(test->value,".shp",exts[c]).c_str(),"rb");
			  if(file!=NULL){
			    fseek(file, 0, SEEK_END);
			    count = ftell(file);
			    rewind(file);
			    
			    tmp=(char*) malloc((count+1)*sizeof(char));  
			    fread(tmp,1,count*sizeof(char),file);
			    
			    VSILFILE* fx=VSIFOpenL(tmpSName, "wb");
			    VSIFWriteL(tmp,1,count,fx);
			    VSIFCloseL(fx);
			    fclose(file);
			    free(tmp);
			  }
			}
			
			VSIFCloseL(fmain);
			
			FILE* file1=fopen(ReplaceAll(test->value,".shp",".zip").c_str(), "rb");
			fseek(file1, 0, SEEK_END);
			count=ftell(file1);
			rewind(file1);
			
			tmp=(char*) malloc((count+1)*sizeof(char));  
			fread(tmp,1,count*sizeof(char),file1);
			
			file=fopen(ReplaceAll(test->value,".shp",".zip").c_str(),"wb");
			fwrite(tmp,1,count,file);
			fclose(file);
			free(tmp);
			fclose(file1);
			setMapInMaps(inputs,paramKey.c_str(),"generated_file",ReplaceAll(test->value,".shp",".zip").c_str());
		      }
		      test=getMapFromMaps(inputs,paramKey.c_str(),"generated_file");

		      if(test!=NULL){
			setMapInMaps(outputs,paramKey.c_str(),"generated_file",test->value);
		      }

		    }
		  }
		}
	      }
	      res=3;
	      break;
	    }
	    else{
	      sprintf(tmpS, "The OTB Application %s cannot be run.", s->name);
	      setMapInMaps(m,"lenv","message",tmpS);
	      res=SERVICE_FAILED;
	    }
	  }
	  catch(std::exception& err){
	    setMapInMaps(m,"lenv","message",err.what());
	    return SERVICE_FAILED;
	    
	  }
	  catch(...){
	    setMapInMaps(m,"lenv","message","An unknown exception has been raised during application execution");
	    res=SERVICE_FAILED;
	  }
	  break;
	}
      }
    }
  }

  for (unsigned int i = 0; i < m_WatcherList.size(); i++){
    m_WatcherList[i]->FreeConf();
    delete m_WatcherList[i];
    m_WatcherList[i] = NULL;
  }
  m_WatcherList.clear();

  return res;
}
