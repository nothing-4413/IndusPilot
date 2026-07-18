cmake_minimum_required(VERSION 3.20)

project(IndusPilot VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(INDUSPILOT_BUILD_CLIENT "Build Qt desktop client" ON)
option(INDUSPILOT_BUILD_TESTS "Build backend tests" ON)

if(INDUSPILOT_BUILD_TESTS)
  enable_testing()
endif()

add_subdirectory(backend)

if(INDUSPILOT_BUILD_CLIENT)
  add_subdirectory(client)
endif()