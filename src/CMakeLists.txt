
cmake_minimum_required( VERSION 2.8.8 )
project(rokid.blacksiren.src)

include_directories(${JSON-C_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
set(TARGET bsiren)
file( GLOB_RECURSE SOURCES *.h *.cpp)
add_library(${TARGET} ${SOURCES})
target_link_libraries(${TARGET} ${JSON-C_LIBRARIES} ${CURL_LIBRARIES})

