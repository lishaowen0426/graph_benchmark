
add_executable(chasing main.cpp ${PROJECT_SOURCE_DIR}/src/util.cpp)


option(RANDOM_SEED "Set random seed before shuffling" OFF)

if(RANDOM_SEED)
    add_definitions(-DRANDOM)
endif()

target_include_directories(chasing PRIVATE ${PROJECT_SOURCE_DIR}/include)
