
add_executable(bw  main.cpp bw.cpp ${PROJECT_SOURCE_DIR}/src/util.cpp)


target_include_directories(bw PRIVATE ${PROJECT_SOURCE_DIR}/include)


add_definitions(-DLOOPS=1)

if(USE_PMEM AND LABOS)
    target_link_libraries(bw PUBLIC ${Ralloc})
    target_include_directories(bw PUBLIC /home/shaowen/ralloc/src)
elseif(USE_PMEM)
    message(FATAL_ERROR "We are not on labos so we can't use pmem")
endif()
target_link_libraries(bw PUBLIC ${Pthread})

find_package(PkgConfig REQUIRED)

if(LABOS)
    set(ENV{PKG_CONFIG_PATH} "/home/$ENV{USER}/pmdk/install/lib/pkgconfig")
    pkg_search_module(LIBPMEM REQUIRED libpmem)
    message(STATUS "${LIBPMEM_LINK_LIBRARIES}")
endif()


target_compile_options(bw PRIVATE "-march=native")
target_compile_options(bw PRIVATE "-masm=att")
