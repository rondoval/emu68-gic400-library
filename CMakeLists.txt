cmake_minimum_required(VERSION 3.14.0)
project(gic400.library VERSION 1.3.0)

string(TIMESTAMP CURRENT_DATE "(%d.%m.%Y)")
set(VERSTRING "$VER: ${PROJECT_NAME} ${PROJECT_VERSION} ${CURRENT_DATE}")
string(REGEX REPLACE "\\.library$" "" LIBRARY_BASENAME "${PROJECT_NAME}")

find_program(SFDC_EXECUTABLE
    NAMES sfdc
    HINTS /opt/m68k-amigaos/bin
    REQUIRED
)

set(GIC400_GENERATED_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/include)
set(GIC400_SFD_FILE ${CMAKE_CURRENT_SOURCE_DIR}/sfd/${LIBRARY_BASENAME}.sfd)
set(GIC400_GENERATED_HEADERS
    ${GIC400_GENERATED_INCLUDE_DIR}/clib/${LIBRARY_BASENAME}_protos.h
    ${GIC400_GENERATED_INCLUDE_DIR}/inline/${LIBRARY_BASENAME}.h
    ${GIC400_GENERATED_INCLUDE_DIR}/pragmas/${LIBRARY_BASENAME}_pragmas.h
    ${GIC400_GENERATED_INCLUDE_DIR}/proto/${LIBRARY_BASENAME}.h
)

add_custom_command(
    OUTPUT ${GIC400_GENERATED_HEADERS}
    COMMAND ${CMAKE_COMMAND}
        -DSFDC_EXECUTABLE=${SFDC_EXECUTABLE}
        -DSFD_FILE=${GIC400_SFD_FILE}
        -DOUTPUT_DIR=${GIC400_GENERATED_INCLUDE_DIR}
        -DOUTPUT_BASENAME=${LIBRARY_BASENAME}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateSfdHeaders.cmake
    DEPENDS
        ${GIC400_SFD_FILE}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateSfdHeaders.cmake
    VERBATIM
)

add_custom_target(gic400_sfd_headers ALL DEPENDS ${GIC400_GENERATED_HEADERS})

find_package(Emu68Common CONFIG REQUIRED)

# Set default install prefix to ./install subdirectory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

add_compile_definitions(PRIVATE LIBRARY_IDSTRING="${VERSTRING}" LIBRARY_VERSION=${PROJECT_VERSION_MAJOR} LIBRARY_REVISION=${PROJECT_VERSION_MINOR} LIBRARY_NAME="${PROJECT_NAME}")

add_compile_options(-Os -m68040 -fomit-frame-pointer -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wmissing-prototypes -Wstrict-prototypes -DDEBUG)
add_link_options(-ffreestanding -nostdlib -nostartfiles -Wall -Wextra -s -Wl,-e,_doNotExecute)

add_library(gic400_headers INTERFACE)
add_executable(gic400_library
    src/gic400_main.c
    src/gic400_distributor.c
    src/gic400_api.c
    src/gic400_end.c
)

target_include_directories(gic400_headers
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${GIC400_GENERATED_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(gic400_library PRIVATE include ${GIC400_GENERATED_INCLUDE_DIR})
add_dependencies(gic400_library gic400_sfd_headers)
target_link_libraries(gic400_library PRIVATE Emu68Common::common)

set_target_properties(gic400_library PROPERTIES
    OUTPUT_NAME gic400
    SUFFIX ".library"
)

install(
    DIRECTORY include/
    DESTINATION include
    PATTERN "${LIBRARY_BASENAME}_private.h" EXCLUDE
)

install(
    DIRECTORY ${GIC400_GENERATED_INCLUDE_DIR}/
    DESTINATION include
)

install(TARGETS gic400_headers gic400_library
    EXPORT GIC400Targets
    RUNTIME DESTINATION LIBS
)

install(EXPORT GIC400Targets
    NAMESPACE GIC400::
    FILE GIC400Targets.cmake
    DESTINATION lib/cmake/GIC400
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GIC400Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/GIC400Config.cmake
    INSTALL_DESTINATION lib/cmake/GIC400
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/GIC400ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/GIC400Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/GIC400ConfigVersion.cmake
    DESTINATION lib/cmake/GIC400
)
