cmake_minimum_required(VERSION 3.1)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(MENOH_MAJOR_VERSION 1)
set(MENOH_MINOR_VERSION 2)
set(MENOH_PATCH_VERSION 0)

# Options
option(USE_OLD_GLIBCXX_ABI "Generate binaries for the old libstdc++ ABI" OFF)
option(LINK_STATIC_LIBPROTOBUF "Link static libprotobuf to libmenoh" OFF)

option(ENABLE_TEST "Build test" OFF)
option(ENABLE_BENCHMARK "Build benchmark" ON)
option(ENABLE_EXAMPLE "Build example" ON)

option(BUILD_SHARED_LIBS "Build shared libs" ON)

option(SHOW_ALL_VARIABLES "Debug: show all variables" OFF)

## C++ setup
set(CMAKE_CXX_STANDARD 14)

## Build type
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    message(STATUS "CMAKE_BUILD_TYPE is unset, defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

## Compiler dependent options
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") # check Clang and AppleClang
    # using Clang
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -pg -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -pg -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s -DNDEBUG")
endif()

## Configure to use the new `libstdc++` ABI
if(USE_OLD_GLIBCXX_ABI)
    message(STATUS "Set _GLIBCXX_USE_CXX11_ABI macro to 0")
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif()

set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(DOWNLOAD_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/downloads CACHE PATH "A directory to save the downloaded files.")
mark_as_advanced(DOWNLOAD_LOCATION)

## Setup protobuf (it is used in ONNX and Menoh)
if(LINK_STATIC_LIBPROTOBUF)
    # Note: We can't use `set(PROTOBUF_BUILD_SHARED_LIBS OFF)` in `FindProtobuf` module
    # because `libprotobuf.a` produced by the package manager is not PIC. So we need to
    # build it by ourselves.
    if(NOT BUILD_SHARED_LIBS)
        message(FATAL_ERROR "LINK_STATIC_LIBPROTOBUF=ON is supported only when BUILD_SHARED_LIBS=ON")
    endif()
    if(UNIX OR MINGW)
        include(BuildProtobuf)
    else()
        message(FATAL_ERROR "LINK_STATIC_LIBPROTOBUF is supported only in UNIX-like environments")
    endif()
else()
    # Note: It may conflict with the loading mechanism in onnx's CMake configuration.
    # See external/onnx/CMakeLists.txt for more details.
    include(FindProtobuf)
    find_package(Protobuf ${PROTOBUF_VERSION} REQUIRED)

    # Note: `protobuf::protoc` is not supported in CMake 3.9.4 or older
    if(NOT TARGET protobuf::protoc)
        find_program(Protobuf_PROTOC_EXECUTABLE
            NAMES protoc
            DOC "The Google Protocol Buffers Compiler")
        add_executable(protobuf::protoc IMPORTED)
        if(EXISTS "${Protobuf_PROTOC_EXECUTABLE}")
            set_target_properties(protobuf::protoc PROPERTIES
                IMPORTED_LOCATION "${Protobuf_PROTOC_EXECUTABLE}")
        endif()
    endif()
endif()

## Setup git
find_package(Git)
if(NOT GIT_FOUND)
    message(FATAL_ERROR "git not found")
endif()

## Build libonnx.a
message(STATUS "Adding external/onnx")

set(ONNX_SRC_DIR ${EXTERNAL_DIR}/onnx)
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init -- ${ONNX_SRC_DIR} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# see https://github.com/onnx/onnx/pull/1466
message(STATUS "Patching to external/onnx")
configure_file(${EXTERNAL_DIR}/onnx-v1.3.0-patch_CMakeLists.txt ${EXTERNAL_DIR}/onnx/CMakeLists.txt COPYONLY)

# TODO: enable the following option when it is ready for migrating to onnx-ml
#set(ONNX_ML 1)

set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${EXTERNAL_DIR}/onnx EXCLUDE_FROM_ALL) # Note: BUILD_SHARED_LIBS must be OFF in this place
set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

if(${ENABLE_TEST})
    message(STATUS "Adding test")
    enable_testing()
    add_subdirectory(test)
endif()

if(${ENABLE_BENCHMARK})
    message(STATUS "Adding benchmark")
    add_subdirectory(benchmark)
endif()

if(${ENABLE_EXAMPLE})
    message(STATUS "Adding example")
    add_subdirectory(example)
endif()

## Build libmenoh
message(STATUS "Adding menoh")
add_subdirectory(menoh)

message(STATUS "Adding include")
add_subdirectory(include)

if(SHOW_ALL_VARIABLES)
    get_cmake_property(_variableNames VARIABLES)
    list(SORT _variableNames)
    foreach(_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endif()
