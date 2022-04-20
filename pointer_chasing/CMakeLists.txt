
add_executable(chasing main.cpp ${PROJECT_SOURCE_DIR}/src/util.cpp)


option(RANDOM_SEED "Set random seed before shuffling" OFF)

if(RANDOM_SEED)
    add_definitions(-DRANDOM)
endif()

target_include_directories(chasing PRIVATE ${PROJECT_SOURCE_DIR}/include)


if(USE_PMEM AND LABOS)
    target_link_libraries(chasing PUBLIC ${Ralloc})
    target_include_directories(chasing PUBLIC /home/shaowen/ralloc/src)
elseif(USE_PMEM)
    message(FATAL_ERROR "We are not on labos so we can't use pmem")
endif()
