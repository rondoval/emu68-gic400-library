if(NOT DEFINED SFDC_EXECUTABLE)
    message(FATAL_ERROR "SFDC_EXECUTABLE is required")
endif()

if(NOT DEFINED SFD_FILE)
    message(FATAL_ERROR "SFD_FILE is required")
endif()

if(NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "OUTPUT_DIR is required")
endif()

if(NOT DEFINED OUTPUT_BASENAME)
    message(FATAL_ERROR "OUTPUT_BASENAME is required")
endif()

file(MAKE_DIRECTORY ${OUTPUT_DIR}/clib)
file(MAKE_DIRECTORY ${OUTPUT_DIR}/inline)
file(MAKE_DIRECTORY ${OUTPUT_DIR}/pragmas)
file(MAKE_DIRECTORY ${OUTPUT_DIR}/proto)

function(generate_sfd_header mode output_file)
    execute_process(
        COMMAND ${SFDC_EXECUTABLE} --addvectors none --target m68k-amigaos --mode ${mode} ${SFD_FILE}
        OUTPUT_FILE ${output_file}
        RESULT_VARIABLE result
    )

    if(NOT result EQUAL 0)
        message(FATAL_ERROR "sfdc failed for mode ${mode} with exit code ${result}")
    endif()
endfunction()

generate_sfd_header(clib ${OUTPUT_DIR}/clib/${OUTPUT_BASENAME}_protos.h)
generate_sfd_header(macros ${OUTPUT_DIR}/inline/${OUTPUT_BASENAME}.h)
generate_sfd_header(pragmas ${OUTPUT_DIR}/pragmas/${OUTPUT_BASENAME}_pragmas.h)
generate_sfd_header(proto ${OUTPUT_DIR}/proto/${OUTPUT_BASENAME}.h)