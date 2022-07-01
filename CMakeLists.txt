cmake_minimum_required(VERSION 3.0)

project(TaskCloudCore VERSION 0.0.1)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILE_FEATURES "${CMAKE_CXX_COMPILE_FEATURES} /MT /GR- /utf-8")
add_definitions(-DUNICODE -D_UNICODE -DYASIO_HEADER_ONLY -D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_SWPRINTFS -DCRYPTOPP_ENABLE_NAMESPACE_WEAK)
add_definitions(-DLUAJIT_ENABLE_LUA52COMPAT)

find_package(Python3 COMPONENTS Development REQUIRED)

include_directories(
    modules
    includes
    third_party
    third_party/luabridge/Source
    third_party/yasio
    ${Python3_INCLUDE_DIRS})

# build luajit static library
set(LUAJIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/luajit)
include_directories(build/third_party/luajit-cmake)
add_subdirectory(third_party/luajit-cmake)

# build quickjs static library
add_subdirectory(third_party/quickjs-cmake)

# build cryptopp static library
set(CRYPTOPP_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cryptopp)
set(CRYPTOPP_DATA_DIR "")
set(CMAKE_SYSTEM_VERSION_BACK ${CMAKE_SYSTEM_VERSION})
set(CMAKE_SYSTEM_VERSION "1145141919810") # To make sure the CMake subdirectory does not add compile definition "_WIN32_WINNT=0x0A00"
include_directories(build/third_party/cryptopp-cmake)
add_subdirectory(third_party/cryptopp-cmake)
set(CMAKE_SYSTEM_VERSION ${CMAKE_SYSTEM_VERSION_BACK})

# build curl static library
set(BUILD_SHARED_LIBS OFF)
set(BUILD_CURL_EXE OFF)
add_subdirectory(third_party/curl)

# scan common source files
file(GLOB_RECURSE COMMONSRC src/common/*.cc)

# build taskcloud core program
add_executable(core src/core/main.cc src/core/service.cc ${COMMONSRC})
target_link_libraries(core libluajit quickjs cryptopp-static libcurl ${Python3_LIBRARIES})

# build taskcloud local program
add_executable(local src/local/main.cc src/local/service.cc ${COMMONSRC})
target_link_libraries(local libluajit quickjs cryptopp-static libcurl ${Python3_LIBRARIES})