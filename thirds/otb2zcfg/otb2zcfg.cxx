#include "otbWrapperApplicationRegistry.h"
#include "otbWrapperApplication.h"
#include "otbImage.h" 
#include <iostream> 

using namespace otb::Wrapper;

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

void printDefaultOutput(){
  std::cout << "  [Result]" << std::endl;
  std::cout << "   Title = the result message" << std::endl;
  std::cout << "   Abstract = the result message" << std::endl;
  std::cout << "   <LiteralData>" << std::endl;
  std::cout << "    dataType = string" << std::endl;
  std::cout << "    <Default />" << std::endl;
  std::cout << "   </LiteralData>" << std::endl;
}

void printAscii(){
  std::cout << "    <Default>" << std::endl;
  std::cout << "     mimeType = text/plain" << std::endl;
  std::cout << "     encoding = ascii" << std::endl;
  std::cout << "    </Default>" << std::endl;
}

void printXml(){
  std::cout << "    <Default>" << std::endl;
  std::cout << "     mimeType = text/xml" << std::endl;
  std::cout << "     encoding = utf-8" << std::endl;
  std::cout << "    </Default>" << std::endl;
}

void printGeoid(){
  std::cout << "    <Default>" << std::endl;
  std::cout << "     mimeType = application/octet-stream" << std::endl;
  std::cout << "    </Default>" << std::endl;
}

void printCSV(){
  std::cout << "    <Default>" << std::endl;
  std::cout << "     mimeType = text/csv" << std::endl;
  std::cout << "     encoding = utf-8" << std::endl;
  std::cout << "    </Default>" << std::endl;
}

void printUnknown(){
  std::cout << "    <Default>" << std::endl;
  std::cout << "     mimeType = text/xml" << std::endl;
  std::cout << "     encoding = utf-8" << std::endl;
  std::cout << "    </Default>" << std::endl;
  std::cout << "    <Supported>" << std::endl;
  std::cout << "     mimeType = text/plain" << std::endl;
  std::cout << "     encoding = utf-8" << std::endl;
  std::cout << "    </Supported>" << std::endl;
}

void printImages(){
  std::cout << "    <Default>" << std::endl;
  std::cout << "     mimeType = image/tiff" << std::endl;
  std::cout << "    </Default>" << std::endl;
#if defined(OTB_USE_JPEG2000)
  std::cout << "    <Supported>" << std::endl;
  std::cout << "     mimeType = image/jp2" << std::endl;
  std::cout << "    </Supported>" << std::endl;
#endif
  std::cout << "    <Supported>" << std::endl;
  std::cout << "     mimeType = image/jpeg" << std::endl;
  std::cout << "    </Supported>" << std::endl;
  std::cout << "    <Supported>" << std::endl;
  std::cout << "     mimeType = image/png" << std::endl;
  std::cout << "    </Supported>" << std::endl;
}

void printKmz(){
  std::cout << "    <Default>" << std::endl;
  std::cout << "     mimeType = application/vnd.google-earth.kmz" << std::endl;
  std::cout << "     extension = kmz" << std::endl;
  std::cout << "    </Default>" << std::endl;
}

void printVector(){
  std::cout << "    <Default>" << std::endl;
  std::cout << "     mimeType = text/xml" << std::endl;
  std::cout << "     encoding = utf-8" << std::endl;
  std::cout << "    </Default>" << std::endl;
  std::cout << "    <Supported>" << std::endl;
  std::cout << "     mimeType = application/vnd.google-earth.kml+xml" << std::endl;
  std::cout << "     encoding = utf-8" << std::endl;
  std::cout << "    </Supported>" << std::endl;
  std::cout << "    <Supported>" << std::endl;
  std::cout << "     mimeType = application/json" << std::endl;
  std::cout << "     encoding = utf-8" << std::endl;
  std::cout << "    </Supported>" << std::endl;
  std::cout << "    <Supported>" << std::endl;
  std::cout << "     mimeType = application/zip" << std::endl;
  std::cout << "    </Supported>" << std::endl;
}

void printOutputImage(ImagePixelType pt){
  std::cout << "   <LiteralData>" << std::endl;
  std::cout << "    dataType = string" << std::endl;
  std::cout << "    <Default>" << std::endl;
  // Check for the default pixel type
  switch(pt){
  case ImagePixelType_uint8:
    std::cout << "     value = uint8" << std::endl;
    break;
  case ImagePixelType_int16:
    std::cout << "     value = int16" << std::endl;
    break;
  case ImagePixelType_uint16:
    std::cout << "     value = uint16" << std::endl;
    break;
  case ImagePixelType_int32:
    std::cout << "     value = int32" << std::endl;
    break;
  case ImagePixelType_uint32:
    std::cout << "     value = uint8" << std::endl;
    break;
  case ImagePixelType_double:
    std::cout << "     value = uint8" << std::endl;
    break;
  default:
    std::cout << "     value = float" << std::endl;
    break;
  }
  std::cout << "     AllowedValues = uint8,uint16,int16,int32,int32,float,double" << std::endl;
  std::cout << "    </Default>" << std::endl;
  std::cout << "   </LiteralData>" << std::endl;
}

void printOutputComplexImage(ComplexImagePixelType pt){
  std::cout << "   <LiteralData>" << std::endl;
  std::cout << "    dataType = string" << std::endl;
  std::cout << "    <Default>" << std::endl;
  // Check for the default pixel type
  switch(pt){
  case ComplexImagePixelType_double:
    std::cout << "     value = cdouble" << std::endl;
    break;
  default:
    std::cout << "     value = cfloat" << std::endl;
    break;
  }
  std::cout << "     AllowedValues = cfloat,cdouble" << std::endl;
  std::cout << "    </Default>" << std::endl;
  std::cout << "   </LiteralData>" << std::endl;
}

int main(int itkNotUsed(argc), char * itkNotUsed(argv)[]) 
{ 
  typedef otb::Image<unsigned short, 2> ImageType; 
 
  ImageType::Pointer image = ImageType::New(); 
 
  const char * ITK_AUTOLOAD_PATH = itksys::SystemTools::GetEnv("ITK_AUTOLOAD_PATH");
  std::cerr << "INFO: Module search path: " << (ITK_AUTOLOAD_PATH ? ITK_AUTOLOAD_PATH : "none (check ITK_AUTOLOAD_PATH)") << std::endl;

  std::vector<std::string> list = ApplicationRegistry::GetAvailableApplications();
  if (list.size() == 0)
    std::cerr << "ERROR: no module found." << std::endl;
  else{
    std::cerr << "INFO: Available modules :" << std::endl;
    for (std::vector<std::string>::const_iterator it = list.begin(); it != list.end(); ++it){
      std::string filename= *it + ".zcfg";
      std::ofstream out(filename.c_str(),std::ofstream::out);
      std::streambuf *coutbuf = std::cout.rdbuf();
      std::cout.rdbuf(out.rdbuf());

      std::cerr << *it << std::endl;
      std::cout << "[" << *it << "]" << std::endl;
      Application::Pointer m_Application=ApplicationRegistry::CreateApplication(*it);
      std::string s0 = m_Application->GetDescription();
      s0=ReplaceAll(ReplaceAll(s0,std::string("\n"),std::string("")),std::string("\t"),std::string(""));
      std::cout << " Title = " << s0 << std::endl;
      s0 = m_Application->GetDocLongDescription();
      s0=ReplaceAll(ReplaceAll(s0,std::string("\n"),std::string("")),std::string("\t"),std::string(""));
      std::cout << " Abstract = " << s0 << std::endl;
      const std::vector<std::string> appKeyList = m_Application->GetParametersKeys(true);
      std::cout << " storeSupported = true"<< std::endl;
      std::cout << " statusSupported = true" << std::endl;
      std::cout << " serviceProvider = " << *it << std::endl;
      std::cout << " serviceType = OTB" << std::endl;
      std::cout << " <DataInputs>" << std::endl;
      for (unsigned int i = 0; i < appKeyList.size(); i++){
	const std::string paramKey(appKeyList[i]);
	Parameter::Pointer param = m_Application->GetParameterByKey(paramKey);
	ParameterType type = m_Application->GetParameterType(paramKey);
	Role role = m_Application->GetParameterRole(paramKey);
      
	if(paramKey!="inxml" && paramKey!="outxml" && role==0 && type!=17 
	   && type!=ParameterType_OutputFilename && type!=ParameterType_OutputVectorData){
	  std::vector<std::string> values;
	  std::string s = m_Application->GetParameterDescription(paramKey);
	  s=ReplaceAll(ReplaceAll(ReplaceAll(s,std::string("\n"),std::string("")),std::string("\t"),std::string("")),std::string("<"),std::string("&lt;"));
	  std::cout << "  [" << paramKey << "]" << std::endl;
	  if(s.length()>0){
	    std::cout << "   Title = " << s << std::endl;
	    std::cout << "   Abstract = " << s << std::endl;
	  }else{
	    std::cout << "   Title = " << paramKey << std::endl;
	    std::cout << "   Abstract = " << paramKey << std::endl;
	  }
	  std::cout << "   minOccurs = " << m_Application->IsMandatory(paramKey) << std::endl;
	  /* Bounded and unbounded parameters */
	  if(type == ParameterType_StringList || type == ParameterType_InputImageList
	     || type == ParameterType_InputVectorDataList || type == ParameterType_InputFilenameList
	     || type == ParameterType_ListView)
	    std::cout << "   maxOccurs = 1024" << std::endl;
	  else
	    std::cout << "   maxOccurs = 1" << std::endl;
	  std::replace( s.begin(), s.end(), '\n', ' ');
	  
	  if(type == ParameterType_StringList || type == ParameterType_String || type == ParameterType_Float
	     || type == ParameterType_Int || type == ParameterType_Choice || type == ParameterType_ListView
	     || type == ParameterType_RAM || type == ParameterType_Empty || type == ParameterType_Directory){
	    std::cout << "   <LiteralData>" << std::endl;
	    std::string lt;
	    if(type == ParameterType_Int || type == ParameterType_RAM)
	      lt="integer";
	    if(type == ParameterType_Float)
	      lt="float";
	    if(type == ParameterType_String || type == ParameterType_StringList
	       || type == ParameterType_Choice || type == ParameterType_Directory
	       || type == ParameterType_ListView)
	      lt="string";
	    if(type == ParameterType_Empty)
	      lt="boolean";
	    std::cout << "    dataType = " << lt << std::endl;
	    if(type == ParameterType_Choice || type == ParameterType_ListView){
	      const std::vector<std::string> nList = m_Application->GetChoiceNames(paramKey);
	      const std::vector<std::string> keysList = m_Application->GetChoiceKeys(paramKey);
	      if(keysList.size()==0){
		std::cout << "    <Default />" << std::endl;
	      }
	      for (unsigned int j = 0; j < keysList.size(); j++){
		const std::string key(keysList[j]);
		if(j==0){
		  std::cout << "    <Default>" << std::endl;
		  if(m_Application->HasValue(paramKey))
		    std::cout << "    value = " << m_Application->GetParameterAsString(paramKey) << std::endl;
		  else
		    std::cout << "    value = " << key << std::endl;
		}
		else{
		  if(j==1){
		    std::cout << "    AllowedValues = "+keysList[0]+",";
		  }
		  std::cout << keysList[j];
		  if(j+1>=keysList.size()){
		    std::cout << std::endl;
		    std::cout << "    </Default>" << std::endl;
		  }
		  else
		    std::cout << ",";
		}
	      }
	    }
	    else{
	      if(type!=17 && m_Application->HasValue(paramKey)){
		std::cout << "    <Default>" << std::endl;
		std::cout << "     value = " << m_Application->GetParameterAsString(paramKey) << std::endl;
		std::cout << "    </Default>" << std::endl;
	      }
	      else
		std::cout << "    <Default />" << std::endl;
	    }
	    std::cout << "   </LiteralData>" << std::endl;
	  }
	  else{
	    if(type == ParameterType_OutputImage){
	      printOutputImage(m_Application->GetParameterOutputImagePixelType(paramKey));
	    }
	    else{
	      if(type == ParameterType_ComplexOutputImage){
		printOutputComplexImage(m_Application->GetParameterComplexOutputImagePixelType(paramKey));
	      }else{
		std::cout << "   <ComplexData>" << std::endl;
		if(type == ParameterType_InputImage || type == ParameterType_InputImageList || type == ParameterType_ComplexInputImage){
		  printImages();
		}
		else
		  if(type == ParameterType_InputVectorData || type == ParameterType_InputVectorDataList){
		    printVector();
		  }
		  else
		    if(type == ParameterType_InputFilename || type == ParameterType_OutputFilename){
		      
		      std::string geoid("geoid");
		      if(paramKey.find(geoid)!= std::string::npos)
			printGeoid();
		      else{
			std::string dtype("vector");
			std::string descr(m_Application->GetParameterDescription(paramKey));
			if(descr.find(dtype)!= std::string::npos)
			  printVector();
			else{
			  std::string dtype1("ASCII");
			  if(descr.find(dtype1)!= std::string::npos)
			    printAscii();
			  else{
			    std::string dtype2("XML");
			    std::string dtype3("xml");
			    if(descr.find(dtype2)!= std::string::npos || descr.find(dtype3)!= std::string::npos)
			      printXml();
			    else
			      printImages();
			  }
			}
		      }
		    }
		std::cout << "   </ComplexData>" << std::endl;
	      }
	    }
	  }


	}
      }
      std::cout << " </DataInputs>" << std::endl;
      std::cout << " <DataOutputs>" << std::endl;
      int hasOutput=-1;
      for (unsigned int i = 0; i < appKeyList.size(); i++){
	const std::string paramKey(appKeyList[i]);
	std::vector<std::string> values;
	Parameter::Pointer param = m_Application->GetParameterByKey(paramKey);
	ParameterType type = m_Application->GetParameterType(paramKey);
	Role role = m_Application->GetParameterRole(paramKey);
	
	if(paramKey!="inxml" && paramKey!="outxml" &&
	   ((type == ParameterType_OutputVectorData || type == ParameterType_OutputImage
	     || type == ParameterType_OutputImage || type == ParameterType_ComplexOutputImage
	     || type == ParameterType_OutputFilename) || role==1) && type != ParameterType_Group){
	  hasOutput=1;
	  std::vector<std::string> values;
	  Parameter::Pointer param = m_Application->GetParameterByKey(paramKey);
	  ParameterType type = m_Application->GetParameterType(paramKey);
	  Role role = m_Application->GetParameterRole(paramKey);
	  std::cout << "  [" << paramKey << "]" << std::endl;
	  std::string s=m_Application->GetParameterDescription(paramKey);
	  if(s.length()>0){
	    s=ReplaceAll(ReplaceAll(s,std::string("\n"),std::string("")),std::string("\t"),std::string(""));
	    std::cout << "   Title = " << s << std::endl;
	    std::cout << "   Abstract = " << s << std::endl;
	  }else{
	    std::cout << "   Title = " << paramKey << std::endl;
	    std::cout << "   Abstract = " << paramKey << std::endl;
	  }

	  if(type == ParameterType_OutputImage || type == ParameterType_ComplexOutputImage){
	    std::cout << "   <ComplexData>" << std::endl;
	    printImages();
	    std::cout << "   </ComplexData>" << std::endl;
	  }
	  else
	    if(type == ParameterType_OutputVectorData || type == ParameterType_OutputImage){
	      std::cout << "   <ComplexData>" << std::endl;
	      if(type == ParameterType_OutputImage)
		printImages();
	      else
		printVector();
	      std::cout << "   </ComplexData>" << std::endl;
	    }
	    else
	      if(type == ParameterType_String || type == ParameterType_StringList
		 || type == ParameterType_Float || type == ParameterType_Int){
		std::cout << "   <LiteralData>" << std::endl;
		std::string lt;
		if(type == ParameterType_Int)
		  lt="integer";
		if(type == ParameterType_Float)
		  lt="float";
		if(type == ParameterType_String || type == ParameterType_StringList)
		  lt="string";
		std::cout << "    dataType = " << lt << std::endl;
		std::cout << "    <Default />" << std::endl;
		std::cout << "   </LiteralData>" << std::endl;
	      }
	      else
		if(type == ParameterType_OutputFilename){
		  std::cout << "   <ComplexData>" << std::endl;
		  std::string descr(m_Application->GetParameterDescription(paramKey));
		  std::string dtype("csv");
		  std::string dtype1("CSV");
		  if(descr.find(dtype)!= std::string::npos || descr.find(dtype1)!= std::string::npos)
		    printCSV();
		  else{
		    std::string dtype2("text file");
		    if(descr.find(dtype2)!= std::string::npos)
		      printAscii();
		    else{
		      std::string dtype2("XML");
		      std::string dtype3("xml");
		      if(descr.find(dtype2)!= std::string::npos || descr.find(dtype3)!= std::string::npos)
			printXml();
		      else{
			std::string dtype4("vector");
			std::string dtype5("Vector");
			if(descr.find(dtype4)!= std::string::npos || descr.find(dtype5)!= std::string::npos)
			  printVector();
		       	else{
			  std::string dtype6("kmz");
			  std::string dtype7("Kmz");
			  if(descr.find(dtype6)!= std::string::npos || descr.find(dtype6)!= std::string::npos)
			    printKmz();
			  else
			    printUnknown();
			}
		      }
		    }
		  }
		  std::cout << "   </ComplexData>" << std::endl;
		}
	}
      }
      if(hasOutput<0)
	printDefaultOutput();
      std::cout << " </DataOutputs>" << std::endl;
      std::cout.rdbuf(coutbuf);
    }
  }
 
  return EXIT_SUCCESS; 
}

