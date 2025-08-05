PROJECT(otb2zcfg)

cmake_minimum_required(VERSION 3.1.0)

find_package(Boost 1.65.0 COMPONENTS serialization filesystem REQUIRED)

find_package(CURL)
find_package(OTB)
if(OTB_FOUND)
  include(${OTB_USE_FILE})
else(OTB_FOUND)
  message(FATAL_ERROR "Cannot build OTB project without OTB.  Please set OTB_DIR.")
endif(OTB_FOUND)

ADD_EXECUTABLE(otb2zcfg otb2zcfg.cxx )  
TARGET_LINK_LIBRARIES(otb2zcfg ${OTB_LIBRARIES}) 
