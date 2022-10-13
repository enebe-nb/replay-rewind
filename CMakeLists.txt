cmake_minimum_required(VERSION 3.16)

string(TIMESTAMP PLP_VER_DAY "%m%d" UTC)
string(TIMESTAMP PLP_VER_YEAR "%Y" UTC)
project(replay-rewind
    VERSION 0.2.${PLP_VER_YEAR}.${PLP_VER_DAY}
    DESCRIPTION "Mod for a specific fighting game.")
set(PLP_VERSION ${PROJECT_VERSION})
string(REPLACE . , PLP_VERSION_COMMA ${PLP_VERSION})

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BASE_INSTALL_PATH replay-rewind)

add_subdirectory(${PROJECT_SOURCE_DIR}/thirdparty/SokuLib)
add_library(replay-rewind MODULE
  src/main.hpp
  src/main.cpp
  src/serializer.hpp
  src/serializer.cpp
)
target_link_libraries(replay-rewind SokuLib)
set_target_properties(replay-rewind PROPERTIES PREFIX "")
