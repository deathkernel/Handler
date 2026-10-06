cmake_minimum_required(VERSION 3.20)
project(Handler VERSION 0.7.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(handler
    src/main.cpp
    src/system_info.cpp
    src/temp_cleaner.cpp
    src/environment_state.cpp
    src/state_store.cpp
    src/history.cpp
    src/module.cpp
    src/router.cpp
    src/maintenance.cpp
    src/error_detection.cpp
    src/component_discovery.cpp
    src/project_context.cpp
    src/dependency_manager.cpp
    src/dependency_graph.cpp
    src/verification.cpp
    src/policy.cpp
    src/snapshot.cpp
    src/transaction.cpp
    src/recovery_journal.cpp
    src/command_engine.cpp
    src/action_engine.cpp
    src/decision_engine.cpp
    src/recovery.cpp
    src/circuit_breaker.cpp
    src/environment_health.cpp
    src/path_guardian.cpp
    src/environment_guardian.cpp
    src/resource_guardian.cpp
    src/toolchain_doctor.cpp
    src/tool_updates.cpp
    src/risky_command.cpp
    src/safe_mode.cpp
    src/deep_cleanup.cpp
    src/uninstall.cpp
    src/repair.cpp
)

target_include_directories(handler PRIVATE include)

if (WIN32)
    target_compile_definitions(handler PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(handler PRIVATE Ws2_32)
endif()

if (MSVC)
    target_compile_options(handler PRIVATE /W4 /permissive-)
else()
    target_compile_options(handler PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(handler_tests
    tests/test_core.cpp
    src/error_detection.cpp
    src/dependency_graph.cpp
    src/policy.cpp
    src/command_engine.cpp
)

target_include_directories(handler_tests PRIVATE include)

add_test(NAME handler_core_tests COMMAND handler_tests)
