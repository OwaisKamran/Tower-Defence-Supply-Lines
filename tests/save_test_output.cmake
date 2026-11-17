# Runs the test runner and saves its output (used by the "test_output" build target).
# Called with -DRUNNER=<exe> -DDATA_DIR=<data folder> -DOUTPUT=<file>
execute_process(
    COMMAND ${RUNNER} ${DATA_DIR}
    OUTPUT_FILE ${OUTPUT}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "test_runner reported failures (exit code ${result}); see ${OUTPUT}")
endif()
message(STATUS "Saved ${OUTPUT}")
