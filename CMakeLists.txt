cmake_minimum_required(VERSION 3.20)
project(Handler VERSION 0.9.0 LANGUAGES CXX)

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
    src/transaction_lock.cpp
    src/recovery_journal.cpp
    src/command_engine.cpp
    src/action_engine.cpp
    src/executable_trust.cpp
    src/state_paths.cpp
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
    src/auto_recovery.cpp
    src/artifact_backup.cpp
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
    src/dependency_manager.cpp
    src/policy.cpp
    src/command_engine.cpp
    src/risky_command.cpp
    src/snapshot.cpp
    src/environment_state.cpp
    src/path_guardian.cpp
    src/environment_guardian.cpp
    src/toolchain_doctor.cpp
    src/component_discovery.cpp
    src/action_engine.cpp
    src/executable_trust.cpp
    src/history.cpp
    src/artifact_backup.cpp
    src/state_paths.cpp
    src/system_info.cpp
    src/transaction.cpp
    src/transaction_lock.cpp
    src/recovery_journal.cpp
    src/uninstall.cpp
    src/project_context.cpp
    src/verification.cpp
    src/repair.cpp
    src/temp_cleaner.cpp
)

target_include_directories(handler_tests PRIVATE include)

if (WIN32)
    target_compile_definitions(handler_tests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(handler_tests PRIVATE Ws2_32)
endif()

add_test(NAME handler_core_tests COMMAND handler_tests)

# CLI smoke/integration coverage: exercise the built executable and its public routing.
add_test(NAME handler_cli_version COMMAND handler version)
set_tests_properties(handler_cli_version PROPERTIES
    PASS_REGULAR_EXPRESSION "Handler 0\\.9\\.0")

add_test(NAME handler_cli_help COMMAND handler help)
set_tests_properties(handler_cli_help PROPERTIES
    PASS_REGULAR_EXPRESSION "handler self-check")

add_test(NAME handler_cli_no_arguments COMMAND handler)
set_tests_properties(handler_cli_no_arguments PROPERTIES
    PASS_REGULAR_EXPRESSION "Handler - Developer Environment Guardian")

add_test(NAME handler_cli_safe_risk_check COMMAND handler risk "python --version")
set_tests_properties(handler_cli_safe_risk_check PROPERTIES
    PASS_REGULAR_EXPRESSION "Risk: SAFE")

add_test(NAME handler_cli_unknown_command
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=handler-test-unknown-command"
        "-DEXPECTED=Unknown Handler command"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

# CLI regression checks for project inspection and guarded failure paths.
add_test(NAME handler_cli_dependency_inspection COMMAND handler deps)
set_tests_properties(handler_cli_dependency_inspection PROPERTIES
    PASS_REGULAR_EXPRESSION "Dependency inspection")

add_test(NAME handler_cli_missing_dependency_upgrade
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=deps;--upgrade;handler-nonexistent-package-for-test"
        "-DEXPECTED=Dependency upgrade blocked: package is not declared"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

add_test(NAME handler_cli_unknown_snapshot_rollback
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=rollback;handler-missing-snapshot-for-test"
        "-DEXPECTED=Rollback blocked: snapshot not found"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

add_test(NAME handler_cli_invalid_repair_target
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=repair;invalid-target;handler-test-package"
        "-DEXPECTED=Unknown repair target"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

add_test(NAME handler_cli_repair_missing_arguments
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=repair"
        "-DEXPECTED=Usage: handler repair"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

add_test(NAME handler_cli_rollback_missing_snapshot_id
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=rollback"
        "-DEXPECTED=Usage: handler rollback"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

add_test(NAME handler_cli_dependency_upgrade_missing_package
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=dependency-upgrade"
        "-DEXPECTED=Usage: handler dependency-upgrade"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

add_test(NAME handler_cli_command_missing_tool
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=command"
        "-DEXPECTED=Usage: handler command"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

add_test(NAME handler_cli_recovery_missing_error
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=recover"
        "-DEXPECTED=Usage: handler recover"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

add_test(NAME handler_cli_detect_error_missing_text
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=detect-error"
        "-DEXPECTED=Usage: handler detect-error"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")

add_test(NAME handler_cli_blocked_risk_check
    COMMAND ${CMAKE_COMMAND}
        -DHANDLER=$<TARGET_FILE:handler>
        "-DARGS=risk;diskpart"
        "-DEXPECTED=Risk: BLOCKED"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/assert_cli_failure.cmake")
