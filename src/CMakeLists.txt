
include_directories(${JSON-C_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
file( GLOB_RECURSE SOURCES *.h *.cpp)
add_library(bsiren ${SOURCES})
target_link_libraries(bsiren blis opus fftw3f android_cutils android_hardware r2ad3 r2vt4 r2ssp ztvad ${JSON-C_LIBRARIES} ${CURL_LIBRARIES})
