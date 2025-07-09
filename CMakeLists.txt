cmake_minimum_required(VERSION 3.16)
project(Nut VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# 设置全局包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/ThirdParty
)

# 查找第三方库
find_package(PkgConfig QUIET)

# 添加 spdlog (如果存在)
if(EXISTS "${CMAKE_SOURCE_DIR}/ThirdParty/spdlog")
    add_subdirectory(ThirdParty/spdlog)
    # 设置 spdlog 包含路径
    include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/spdlog/include)
endif()

# 添加 tcmalloc (如果存在)
if(EXISTS "${CMAKE_SOURCE_DIR}/ThirdParty/tcmalloc")
    # tcmalloc 配置
endif()

# 查找所有子项目的 CMakeLists.txt
file(GLOB_RECURSE CMAKE_FILES "Source/*/CMakeLists.txt")

# 添加每个子项目
foreach(CMAKE_FILE ${CMAKE_FILES})
    get_filename_component(PROJECT_DIR ${CMAKE_FILE} DIRECTORY)
    add_subdirectory(${PROJECT_DIR})
endforeach()

# 打印项目信息
message(STATUS "Nut Project Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}") 