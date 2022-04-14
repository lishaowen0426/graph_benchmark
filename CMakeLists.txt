cmake_minimum_required(VERSION 3.5)

project(graph_benchmark)

SET(COMPILER_FLAGS "-o3 -ggdb")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")


option(USE_PMEM "Allocate memory on pmem" OFF)

SET(LABOS FALSE)
if("$ENV{USER}" STREQUAL "shaowen" )
    set(LABOS TRUE)
    add_definitions(-DLABOS)
    message(STATUS "on labos")
endif()




SET(TBB_CONFIG_PATH "/home/$ENV{USER}/oneTBB/install/lib/cmake")

find_package(TBB REQUIRED 
    PATHS ${TBB_CONFIG_PATH} 
    NO_DEFAULT_PATH
    )

message(STATUS "TBB version:  ${TBB_VERSION}")

FILE(GLOB GRAPH_LIB_SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_library(graph_lib STATIC ${GRAPH_LIB_SRCS})
target_include_directories(graph_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(graph_lib PUBLIC TBB::tbb)

if(USE_PMEM AND LABOS)
    message(STATUS "Allocate memory on pmem")
    add_definitions(-DPMEM)
    find_library(Ralloc ralloc /home/shaowen/ralloc/test)
    target_link_libraries(graph_lib PUBLIC ${Ralloc})
    target_include_directories(graph_lib PUBLIC /home/shaowen/ralloc/src)
elseif(USE_PMEM)
    message(FATAL_ERROR "We are not on labos so we can't use pmem")
endif()

find_library(Pthread pthread)

add_executable(benchmark main.cpp)
target_link_libraries(graph_lib PUBLIC ${Pthread})

target_link_libraries(benchmark PRIVATE graph_lib)

add_executable(reorder reorder.cpp)
target_link_libraries(reorder PUBLIC TBB::tbb)



