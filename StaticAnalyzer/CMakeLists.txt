CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (TaskDroid)
MESSAGE(STATUS "root This is SOURCE dir " ${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/include/Analyzer
  ${PROJECT_SOURCE_DIR}/include/AndroidStackMachine
  ${PROJECT_SOURCE_DIR}/include/fml/include
  ${PROJECT_SOURCE_DIR}/include/Parser
  /usr/local/include
  /opt/homebrew/include
  /usr/include
)
LINK_DIRECTORIES(
  /usr/local/lib
  /opt/homebrew/lib
  /usr/lib
)
MESSAGE ("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
IF (CMAKE_BUILD_TYPE MATCHES "DEBUG")
    MESSAGE ("Debug build.")
    SET (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/debug/bin)
ELSEIF (CMAKE_BUILD_TYPE MATCHES "RELEASE")
    MESSAGE ("Release build.")
    SET (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
ELSE ()
    MESSAGE ("Some other build type.")
    SET (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
ENDIF ()

SET (CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -w -g -std=c++17")
SET (CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -w -std=c++17")
SET (CMAKE_EXPORT_COMPILE_COMMANDS 1)

SET (CMAKE_CXX_STANDARD 17)

ADD_SUBDIRECTORY(src)
