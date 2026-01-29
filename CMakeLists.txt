cmake_minimum_required(VERSION 4.0.1)
project(macos-tilde)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT APPLE)
    message(FATAL_ERROR "This project only supports macOS")
endif()

find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
find_library(CARBON Carbon REQUIRED)

add_executable(keyremap
    keyremap.cpp
)

target_link_libraries(keyremap
    ${APPLICATION_SERVICES}
    ${CARBON}
)

install(TARGETS keyremap DESTINATION bin)
