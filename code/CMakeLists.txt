cmake_minimum_required(VERSION 4.0)
project(FYP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(project_compilation_options INTERFACE)
target_include_directories(project_compilation_options INTERFACE ${PROJECT_SOURCE_DIR})
target_compile_options(project_compilation_options INTERFACE -Wall -Wextra -Wpedantic -Werror)
# shared compilation options to be used by everyone

add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(forwarding_tables)
add_subdirectory(generator)

enable_testing()
add_subdirectory(tests)

add_executable(forwarding main.cpp)
target_link_libraries(forwarding PRIVATE project_compilation_options core)