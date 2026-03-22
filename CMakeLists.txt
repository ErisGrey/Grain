cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror")
IF( NOT CMAKE_BUILD_TYPE )
   SET( CMAKE_BUILD_TYPE Release ... FORCE )
ENDIF()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(MY_PROJECT_NAME grain)
project(${MY_PROJECT_NAME})

######## Cplex Dependency
find_package(CPLEX REQUIRED)
if(CPLEX_FOUND)
    include_directories(${CPLEX_INCLUDE_DIR})
    link_directories(${CPLEX_LIBRARY})
endif()

if(CPLEX_CONCERT_FOUND)
    include_directories(${CPLEX_CONCERT_INCLUDE_DIR})
    link_directories(${CPLEX_CONCERT_LIBRARY})
endif()

if(CPLEX_ILOCPLEX_FOUND)
    include_directories(${CPLEX_ILOCPLEX_INCLUDE_DIR})
    link_directories(${CPLEX_ILOCPLEX_LIBRARY})
endif()

if(CPLEX_CP_FOUND)
    include_directories(${CPLEX_CP_INCLUDE_DIR})
    link_directories(${CPLEX_CP_LIBRARY})
endif()

set(CMAKE_CXX_STANDARD 17)

######## Set output directories.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build//bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)


file(GLOB_RECURSE source source/*.cpp)
file(GLOB_RECURSE header header/*.h)
add_executable(${PROJECT_NAME} ${source} ${header})

target_link_libraries(${PROJECT_NAME} cplex-library cplex-concert ilocplex cplex-cp ${CommonLibs})

