
cmake_minimum_required(VERSION 3.0.0)
project(test VERSION 0.1.0)

PROJECT(taro)

# include directories
INCLUDE_DIRECTORIES(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/../co_taro/include
)

IF (CMAKE_SYSTEM_NAME MATCHES "Windows")
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
	ADD_DEFINITIONS(-Wall)
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Windows")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
LINK_DIRECTORIES(${PROJECT_BINARY_DIR}/lib)

# compile all code
ADD_SUBDIRECTORY(build_lib)

# test
ADD_SUBDIRECTORY(test)