cmake_minimum_required(VERSION 3.20)
project(Handler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(handler
    src/main.cpp
    src/system_info.cpp
    src/temp_cleaner.cpp\n    src/environment_state.cpp\n    src/state_store.cpp\n    src/history.cpp\n    src/module.cpp\n    src/router.cpp\n    src/maintenance.cpp
)

target_include_directories(handler PRIVATE include)

if (WIN32)
    target_compile_definitions(handler PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if (MSVC)
    target_compile_options(handler PRIVATE /W4 /permissive-)
else()
    target_compile_options(handler PRIVATE -Wall -Wextra -Wpedantic)
endif()
