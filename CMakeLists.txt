CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)

PROJECT(rokid.blacksiren)
SET(VERSION 0.1)
SET(CMAKE_MACOSX_RPATH 1)

macro(CheckExitCodeAndExitIfError)
  if(NOT ${ExitCode} EQUAL 0)
    return(${ExitCode})
  endif()
endmacro(CheckExitCodeAndExitIfError)

option(BUILD_DEMO "Build the demo" OFF)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/.lib)

find_package(CURL REQUIRED)
# For FindJSON-C
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(JSON-C REQUIRED)

set(MYPREPROCESS_CMD1 ${CMAKE_MAKE_PROGRAM} -f siren_config.mk -f siren_enable_flags.mk -pn)
set(MYPREPROCESS_CMD2 grep L_CFLAGS)
set(MYPREPROCESS_CMD3 sed -e "s#^.*:=##g")
#set(MYPREPROCESS_CMD3 sed -e "s#^.*:=##g" -e "s#\\\\\"##g")

execute_process(COMMAND ${MYPREPROCESS_CMD1} COMMAND ${MYPREPROCESS_CMD2} COMMAND ${MYPREPROCESS_CMD3} ERROR_VARIABLE myto_skip OUTPUT_VARIABLE COMMON_FLAGS RESULT_VARIABLE ExitCode OUTPUT_STRIP_TRAILING_WHITESPACE)
CheckExitCodeAndExitIfError()

message("=====>>${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -Wall -Wextra -Wunused-parameter -std=gnu++11")
message("=====>>${CMAKE_CXX_FLAGS}")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

include (TargetArch)
target_architecture(DETECTED_ARCH)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/thirdparty/support/include)
if (DETECTED_ARCH MATCHES "armv8")
	include_directories(${CMAKE_CURRENT_BINARY_DIR}/thirdparty/support/arm64/include)
elseif (DETECTED_ARCH MATCHES "arm.*")
	include_directories(${CMAKE_CURRENT_BINARY_DIR}/thirdparty/support/arm/include)
endif()

#
# blacksiren sdk source dir.
#
add_subdirectory(src)

