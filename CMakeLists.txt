cmake_minimum_required(VERSION 3.20)
project(Nut VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找所有服务
file(GLOB_RECURSE SERVICE_DIRS "MicroServices/*/")
foreach(SERVICE_DIR ${SERVICE_DIRS})
    if(EXISTS "${SERVICE_DIR}/Meta")
        get_filename_component(SERVICE_NAME ${SERVICE_DIR} NAME)
        message(STATUS "Found service: ${SERVICE_NAME}")
    endif()
endforeach()

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/Include)
include_directories(${CMAKE_SOURCE_DIR}/MicroServices/*/Sources)
