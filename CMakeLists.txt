cmake_minimum_required(VERSION 3.5)

project(graph_benchmark)

SET(TBB_CONFIG_PATH "/home/lsw/oneTBB/install/lib/cmake")

find_package(TBB REQUIRED 
    PATHS ${TBB_CONFIG_PATH} 
    NO_DEFAULT_PATH
    )

message(STATUS "TBB version:  ${TBB_VERSION}")

FILE(GLOB GRAPH_LIB_SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp)

message(STATUS ${GRAPH_LIB_SRCS})
add_library(graph_lib STATIC ${GRAPH_LIB_SRCS})
target_include_directories(graph_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(graph_lib PUBLIC TBB::tbb)

add_executable(benchmark main.cpp)

target_link_libraries(benchmark PRIVATE graph_lib)



