PROJECT(otb2zcfg)  
 
cmake_minimum_required(VERSION 2.6)  
 
FIND_PACKAGE(OTB)  
IF(OTB_FOUND)  
  INCLUDE(${OTB_USE_FILE})  
ELSE(OTB_FOUND)  
  MESSAGE(FATAL_ERROR  
      "Cannot build OTB project without OTB.  Please set OTB_DIR.")  
ENDIF(OTB_FOUND)  
 
ADD_EXECUTABLE(otb2zcfg otb2zcfg.cxx )  
TARGET_LINK_LIBRARIES(otb2zcfg OTBCommon OTBApplicationEngine OTBIO ${ITK_LIBRARIES}) 
