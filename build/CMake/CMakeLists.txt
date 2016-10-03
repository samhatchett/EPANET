# EPANET 2.xx LIBRARY

cmake_minimum_required (VERSION 2.6)

project (EPANET)

add_definitions(-std=c99 -DEN_API_FLOAT_TYPE=double)
SET(CMAKE_C_FLAGS "-std=c99")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

IF(APPLE)
  SET(CMAKE_INSTALL_NAME_DIR @executable_path)
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
ENDIF(APPLE)

# the library
include_directories(../../include)
file(GLOB EPANET_SOURCES ../../src/*.c)
add_library(epanet STATIC  ${EPANET_SOURCES})

install(TARGETS epanet DESTINATION lib)
install(FILES ../../include/epanet2.h DESTINATION include)