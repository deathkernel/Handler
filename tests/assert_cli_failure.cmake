if(NOT DEFINED HANDLER OR NOT DEFINED ARGS OR NOT DEFINED EXPECTED)
    message(FATAL_ERROR "HANDLER, ARGS, and EXPECTED are required")
endif()

execute_process(
    COMMAND "${HANDLER}" ${ARGS}
    RESULT_VARIABLE exit_code
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if("${exit_code}" STREQUAL "0")
    message(FATAL_ERROR "Expected command to fail, but it exited successfully. Output: ${stdout}${stderr}")
endif()

string(CONCAT output "${stdout}" "${stderr}")
if(NOT output MATCHES "${EXPECTED}")
    message(FATAL_ERROR "Command failed, but output did not match '${EXPECTED}'. Output: ${output}")
endif()
