#include <locale.h>

#include <wx/app.h>
#include <wx/utils.h>

#include <api_core.h>
#include <module_library.h>

int Callback(TSG_UI_Callback_ID ID, CSG_UI_Parameter &Param_1, CSG_UI_Parameter &Param_2){
  return( 1 );
}

TSG_PFNC_UI_Callback Get_Callback(void){
  return( &Callback );
}

const char* cleanAbstract(CSG_String abstract){
  char* tmp=strdup(abstract.b_str());
  CSG_String val=CSG_String("")+tmp;
  if(val.Contains("¸")){
    val.Replace("¸"," ");
  }
  if(val.Contains("\n")){
    val.Replace("\n","<br/>");
  }
  if(val.Contains("<")){
    val.Replace("<","&lt;");
  }
  if(val.Contains(">")){
    val.Replace(">","&gt;");
    fprintf(stderr,"%s\n",val.b_str());
  }
  free(tmp);
  fprintf(stderr,"%s\n",val.b_str());
  return val.b_str();
}

void printBasicMetadata(FILE* stdout,CSG_Parameter* param,bool out=false,bool range=false,bool min=true,bool tin=false,char* tname=NULL,char* ttitle=NULL){
  fprintf(stdout," [%s]\n",(CSG_String(param->Get_Identifier())+(range?(min?"_MIN":"_MAX"):(tin?CSG_String("_")+tname:""))).b_str());
  fprintf(stdout,"  Title = %s\n",(CSG_String(param->Get_Name())+(range?(min?" (min value)":" (max value)"):(tin&&ttitle!=NULL?ttitle:""))).b_str());
  if(CSG_String(param->Get_Description()).is_Empty())
    fprintf(stdout,"  Abstract = %s\n",CSG_String(param->Get_Name()).b_str());
  else{
    fprintf(stdout,"  Abstract = %s\n",cleanAbstract(CSG_String(param->Get_Description()).BeforeFirst('\n')));
  }
  if(!out){
    if(param->is_Option() || param->is_Optional()){
      fprintf(stdout,"  minOccurs = 0\n");
      if(param->is_Option())
	fprintf(stdout,"  maxOccurs = 1\n");
      else{
	if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("_list")))
	  fprintf(stdout,"  maxOccurs = 1024\n");
	else
	  fprintf(stdout,"  maxOccurs = 1\n");
      }
    }else{
      if(param->is_Input())
	fprintf(stdout,"  minOccurs = 1\n");
      if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("_list")))
	fprintf(stdout,"  maxOccurs = 1024\n");
      else
	fprintf(stdout,"  maxOccurs = 1\n");
    }
  }
  if(range){
    
  }
}

void printGrids(FILE* stdout){
  fprintf(stdout,"  <ComplexData>\n");
  fprintf(stdout,"   <Default>\n");
  fprintf(stdout,"    mimeType = image/tiff\n");
  fprintf(stdout,"   </Default>\n");
  fprintf(stdout,"   <Supported>\n");
  fprintf(stdout,"    mimeType = application/x-ogc-envi\n");
  fprintf(stdout,"   </Supported>\n");
  fprintf(stdout,"   <Supported>\n");
  fprintf(stdout,"    mimeType = application/x-ogc-aaigrid\n");
  fprintf(stdout,"   </Supported>\n");
  fprintf(stdout,"  </ComplexData>\n");
}

void printShapes(FILE* stdout){
  fprintf(stdout,"  <ComplexData>\n");
  fprintf(stdout,"   <Default>\n");
  fprintf(stdout,"    mimeType = text/xml\n");
  fprintf(stdout,"    encoding = utf-8\n");
  fprintf(stdout,"   </Default>\n");
  fprintf(stdout,"   <Supported>\n");
  fprintf(stdout,"    mimeType = application/vnd.google-earth.kml+xml\n");
  fprintf(stdout,"   </Supported>\n");
  fprintf(stdout,"   <Supported>\n");
  fprintf(stdout,"    mimeType = application/json\n");
  fprintf(stdout,"   </Supported>\n");
  fprintf(stdout,"  </ComplexData>\n");
}

void printTables(FILE* stdout){
  fprintf(stdout,"  <ComplexData>\n");
  fprintf(stdout,"   <Default>\n");
  fprintf(stdout,"    mimeType = text/csv\n");
  fprintf(stdout,"    encoding = utf-8\n");
  fprintf(stdout,"   </Default>\n");
  fprintf(stdout,"   <Supported>\n");
  fprintf(stdout,"    mimeType = text/csv\n");
  fprintf(stdout,"    encoding = base64\n");
  fprintf(stdout,"   </Supported>\n");
  fprintf(stdout,"  </ComplexData>\n");
}

void printPoints(FILE* stdout){
  fprintf(stdout,"  <ComplexData>\n");
  fprintf(stdout,"   <Default>\n");
  fprintf(stdout,"    mimeType = application/x-ogc-lasf\n");
  fprintf(stdout,"    extension = las\n");
  fprintf(stdout,"   </Default>\n");
  fprintf(stdout,"  </ComplexData>\n");
}

int main(int argc, char *argv[]) {
  if( !wxInitialize() ){
    fprintf(stderr,"initialisation failed");
    return -1;
  }
  setlocale(LC_NUMERIC, "C");
  static bool g_bShow_Messages = false;
  SG_Set_UI_Callback(Get_Callback());
  int n = SG_Get_Module_Library_Manager().Add_Directory(wxT(MODULE_LIBRARY_PATH),false);
  if( SG_Get_Module_Library_Manager().Get_Count() <= 0 ){
    fprintf(stderr,"could not load any tool library");
    return -2;
  }

  for(int i=0;i<SG_Get_Module_Library_Manager().Get_Count();i++){
    
    CSG_Module_Library * library=SG_Get_Module_Library_Manager().Get_Library(i);
    int lc=library->Get_Count();
    if(!library->Get_Library_Name().Contains("io_")) {

      for(int j=0;j<lc;j++){
	CSG_Module * module=library->Get_Module(j);
	if(module!=NULL && !module->needs_GUI() /*&& !module->is_Interactive()*/ ){

	  mkdir(library->Get_Library_Name().b_str(),0755);
	  FILE *stdout1=fopen((library->Get_Library_Name()+"/"+module->Get_ID()+".zcfg").b_str(),"w+");
	  fprintf(stdout1,"[%d]\n",j);
	  fprintf(stdout1," Title = %s\n",module->Get_Name().b_str());
	  if(CSG_String(module->Get_Description()).is_Empty())
	    fprintf(stdout1," Abstract = %s\n",module->Get_Name().b_str());
	  else
	    fprintf(stdout1," Abstract = %s\n",cleanAbstract(module->Get_Description()));
	  fprintf(stdout1," storeSupported = true\n");
	  fprintf(stdout1," statusSupported = true\n");
	  fprintf(stdout1," serviceType = SAGA\n");
	  fprintf(stdout1," serviceProvider = %s\n",library->Get_Library_Name().b_str());
	  CSG_Parameters * params=module->Get_Parameters();
	  int pc=params->Get_Count();

	  fprintf(stdout1," <DataInputs>\n");
	  for(int k=0;k<pc;k++){
	    CSG_Parameter * param=params->Get_Parameter(k);
	  
	    // Node should be used for defining Complex ComplexData
	    if(CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("color"))){
	      
	    }
	    else if(CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("range"),true)){

	      param->Restore_Default();

	      printBasicMetadata(stdout1,param,false,true,true);
	      fprintf(stdout1,"  <LiteralData>\n");
	      fprintf(stdout1,"   dataType = float\n");
	      CSG_Parameter_Range *range=param->asRange();
	      fprintf(stdout1,"   <Default>\n");
	      fprintf(stdout1,"    value = %f\n",((CSG_Parameter_Range*)param->Get_Data())->Get_LoVal());
	      fprintf(stdout1,"   </Default>\n");
	      fprintf(stdout1,"  </LiteralData>\n");

	      printBasicMetadata(stdout1,param,false,true,false);
	      fprintf(stdout1,"  <LiteralData>\n");
	      fprintf(stdout1,"   dataType = float\n");
	      fprintf(stdout1,"   <Default>\n");
	      fprintf(stdout1,"    value = %f\n",((CSG_Parameter_Range*)param->Get_Data())->Get_HiVal());
	      fprintf(stdout1,"   </Default>\n");
	      fprintf(stdout1,"  </LiteralData>\n");

	    }
	    else if(param!=NULL && !param->is_Output() 
		    && 
		    !CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("parameters"),true)
		    && 
		    !CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("grid_system"),true)
		    && 
		    !CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("node"),true)){
	    
	      printBasicMetadata(stdout1,param);

	      if(CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("file"),true)){
		if(CSG_String(param->Get_Description()).Contains(CSG_String("HTML"))){
		  fprintf(stdout1,"  <ComplexData>\n");	       
		  fprintf(stdout1,"   <Default>\n");
		  fprintf(stdout1,"    mimeType = text/html\n");
		  fprintf(stdout1,"    encoding = utf-8\n");
		  fprintf(stdout1,"   </Default>\n");
		  fprintf(stdout1,"  <ComplexData>\n");
		}
		else{
		  fprintf(stdout1,"  <ComplexData>\n");	       
		  fprintf(stdout1,"   <Default>\n");
		  if(CSG_String(param->Get_Description()).Contains(CSG_String("HTML"))){
		    fprintf(stdout1,"    mimeType = text/html\n");
		  }else
		    if(CSG_String(param->Get_Description()).Contains(CSG_String("SVG"))){
		      fprintf(stdout1,"    mimeType = image/svg+xml\n");
		    }else
		      fprintf(stdout1,"    mimeType = text/plain\n");
		  if(CSG_String(param->Get_Description()).Contains(CSG_String("ASCII"))){
		    fprintf(stdout1,"    encoding = ascii\n");
		  }else
		    fprintf(stdout1,"    encoding = utf-8\n");
		  fprintf(stdout1,"   </Default>\n");
		  fprintf(stdout1,"  </ComplexData>\n");
		}
	      }
	      if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("shapes")) ||
		 CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("tin")) ){
		printShapes(stdout1);
	      }
	      if(CSG_String(param->Get_Type_Identifier()).Contains("points")){
		printPoints(stdout1);
	      }
	      if(CSG_String(param->Get_Type_Identifier()).Contains("grid") ||
		 CSG_String(param->Get_Type_Identifier()).Contains("data_")){
		printGrids(stdout1);
	      }
	      if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("table"))){
		printTables(stdout1);
	      }
	      if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("text"))){
		fprintf(stdout1,"  <LiteralData>\n");
		fprintf(stdout1,"   dataType = string\n");
		fprintf(stdout1,"   <Default>\n");
		if( !param->Get_Data()->Get_Default().is_Empty() ){
		  fprintf(stdout1,"    value = %s\n",param->Get_Data()->Get_Default().b_str());
		}
		fprintf(stdout1,"   </Default>\n");
		fprintf(stdout1,"  </LiteralData>\n");
	      }
	      if(CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("double"),true)
		 || 
		 CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("degree"),true)
		 || 
		 CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("integer"),true)){
		fprintf(stdout1,"  <LiteralData>\n");
		if(CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("double"),true) || 
		 CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("degree"),true))
		  fprintf(stdout1,"   dataType = float\n");
		else
		  fprintf(stdout1,"   dataType = integer\n");

		fprintf(stdout1,"   <Default>\n");
		if( !param->Get_Data()->Get_Default().is_Empty() ){
		  fprintf(stdout1,"    value = %s\n",param->Get_Data()->Get_Default().b_str());
		}
		if(param->asValue()->has_Minimum() && param->asValue()->has_Maximum()){
		  fprintf(stdout1,"    rangeMin = %f\n",param->asValue()->Get_Minimum());
		  fprintf(stdout1,"    rangeMax = %f\n",param->asValue()->Get_Maximum());
		  fprintf(stdout1,"    rangeClosure = c\n");
		}
		fprintf(stdout1,"   </Default>\n");
		fprintf(stdout1,"  </LiteralData>\n");
	      }
	      if(CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("boolean"),true)){
		fprintf(stdout1,"  <LiteralData>\n");
		fprintf(stdout1,"   dataType = bool\n");
		fprintf(stdout1,"   <Default>\n");
		if( !param->Get_Data()->Get_Default().is_Empty() ){
		  fprintf(stdout1,"    value = %s\n",(param->Get_Data()->Get_Default().Contains("0")?"false":"true"));
		}
		fprintf(stdout1,"   </Default>\n");
		fprintf(stdout1,"  </LiteralData>\n");	
	      }
	      if(CSG_String(param->Get_Type_Identifier()).is_Same_As(CSG_String("choice"),true)){
		CSG_Parameter_Choice* choice=param->asChoice();
		if(choice!=NULL){
		  fprintf(stdout1,"  <LiteralData>\n");
		  fprintf(stdout1,"   dataType = string\n");
		  fprintf(stdout1,"   AllowedValues = ");
		  int clen=choice->Get_Count();
		  if(clen>0){
		    for(int l=0;l<clen;l++){
		      //fprintf(stdout1,"%d",l);
		      fprintf(stdout1,"%s",cleanAbstract(CSG_String(choice->Get_Item(l))));
		      if(l+1<clen)
			fprintf(stdout1,",");
		    }
		    fprintf(stdout1,"\n");
		  }
		  fprintf(stdout1,"   <Default>\n");
		  if( !param->Get_Data()->Get_Default().is_Empty() ){
		    fprintf(stdout1,"    value = %s\n",cleanAbstract(CSG_String(choice->Get_Item(atoi(param->Get_Data()->Get_Default())))));
		  }
		  fprintf(stdout1,"   </Default>\n");
		  fprintf(stdout1,"  </LiteralData>\n");
		}
	      }
	    }
	  }
	  fprintf(stdout1," </DataInputs>\n");
	  fprintf(stdout1," <DataOutputs>\n");

	  bool hasOutput=false;

	  for(int k=0;k<pc;k++){
	    CSG_Parameter * param=params->Get_Parameter(k);
	    if(param!=NULL && param->is_Output()){
	      hasOutput=true;
	      if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("tin"))){
		char* tinOut[5]={
		  (char*)"POINTS",
		  (char*)"CENTER",
		  (char*)"EDGES",
		  (char*)"TRIANGLES",
		  (char*)"POLYGONS"
		};
		for(int l=0;l<5;l++){
		  char *ttitle=(char*)malloc((strlen(tinOut[l])+8)*sizeof(char));
		  sprintf(ttitle,"%s Shapes",tinOut[l]);
		  printBasicMetadata(stdout1,param,true,false,true,true,tinOut[l],ttitle);
		  printShapes(stdout1);
		}
	      }
	      else {
		printBasicMetadata(stdout1,param,true);

		if(CSG_String(param->Get_Type_Identifier()).Contains("points")){
		  printPoints(stdout1);
		}
		if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("shapes"))){
		  printShapes(stdout1);
		}
		if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("grid")) ||
		   CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("data_object"))){
		  printGrids(stdout1);
		}
		if(CSG_String(param->Get_Type_Identifier()).Contains(CSG_String("table"))){
		  printTables(stdout1);
		}

	      }
	    }
	  }
	  fprintf(stdout1,"</DataOutputs>\n");
	  fclose(stdout1);
	  if(!hasOutput)
	    unlink((library->Get_Library_Name()+"/"+module->Get_ID()+".zcfg").b_str());
	}
      }
    }
  }
  wxUninitialize();

  return 0;
}
