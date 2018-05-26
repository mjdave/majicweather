# Specify the minimum version for CMake

cmake_minimum_required(VERSION 3.5)

# Project's name

project(majicweather)

set(CMAKE_BUILD_TYPE Debug)

set(SOURCES 
    main.cpp
    ../thirdparty/json11-master/json11.cpp
    src/GribLoader.cpp
)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set (CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-deprecated")
endif ( CMAKE_COMPILER_IS_GNUCC )


include_directories(
    ../thirdparty/eccodesBuild/include/
    ../thirdparty/
    ../thirdparty/json11-master
    ../thirdparty/glm/glm
    src
)

add_executable(majicweather ${SOURCES})

set (CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")

target_link_libraries(majicweather "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/eccodesBuild/lib/libeccodes.a")
target_link_libraries(majicweather "/usr/local/lib/libopenjp2.so")

