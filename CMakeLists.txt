cmake_minimum_required(VERSION 2.8)
project(pattern-mining)

set(CMAKE_VERBOSE_MAKEFILE OFF)
add_definitions(-DHAVE_CONFIG_H -DSQLITE_THREADSAFE=1)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
	message("-- pkgconfig found")
endif(PKG_CONFIG_FOUND)

set(CMAKE_CXX_FLAGS  "-O2 -Wall -g --std=gnu++14 -fPIC -fdiagnostics-color=always")
set(LINK_FLAGS "-O2 -g")

if(UNIX)
	add_definitions(-DUNIX)
elseif(WIN32)
	add_definitions(-DWIN32)
endif(UNIX)

# boost
set (BOOST_ROOT ${CMAKE_PREFIX_PATH})
set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_MULTITHREADED ON)

add_definitions(-DBOOST_THREAD_USE_LIB)

find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(3rdparty)

#include_directories("/usr/include/cairo")
#include_directories("/usr/include/freetype2")

set(sources
	log.cpp

	3rdparty/lodepng/lodepng.cpp

	model/quotes.cpp
	miners/ttminer.cpp
	miners/minmaxminer.cpp
	miner.cpp

	report/textreportbuilder.cpp
#	report/htmlreportbuilder.cpp
	)
	
add_executable(pattern-mining main.cpp ${sources})
target_link_libraries(pattern-mining  ${Boost_LIBRARIES})
if(UNIX)
target_link_libraries(pattern-mining -lpthread -ldl)
endif(UNIX)
#target_link_libraries(pattern-mining -lcairo -lfreetype)

