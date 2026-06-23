cmake_minimum_required(VERSION 3.14.0)
project(gic400.library VERSION 1.5)

# Version string
string(TIMESTAMP CURRENT_DATE "(%d.%m.%Y)")
set(VERSTRING "$VER: ${PROJECT_NAME} ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR} ${CURRENT_DATE}")
string(REGEX REPLACE "\\.library$" "" LIBRARY_BASENAME "${PROJECT_NAME}")

# Find SFDC tool
find_program(SFDC_EXECUTABLE
    NAMES sfdc
    HINTS /opt/m68k-amigaos/bin
    REQUIRED
)

# SFD header generation
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

# Dependencies
find_package(Emu68Common CONFIG REQUIRED)

# Set default install prefix to ./install subdirectory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

# Compile definitions and options
add_compile_definitions(
    PRIVATE
        LIBRARY_IDSTRING="${VERSTRING}"
        LIBRARY_VERSION=${PROJECT_VERSION_MAJOR}
        LIBRARY_REVISION=${PROJECT_VERSION_MINOR}
        LIBRARY_NAME="${PROJECT_NAME}"
)

add_compile_options(
    -O3
    -m68040
    -fomit-frame-pointer
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wmissing-prototypes
    -Wstrict-prototypes
)

# Debug-output backend defines (EMU68_DEBUG_BACKEND, see emu68-common).
emu68_debug_backend_definitions()

add_link_options(
    -ffreestanding
    -nostdlib
    -nostartfiles
    -Wall
    -Wextra
    -s
    -Wl,-e,_doNotExecute
)

# Targets
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

target_include_directories(gic400_library
    PRIVATE
        include
        ${GIC400_GENERATED_INCLUDE_DIR}
)

add_dependencies(gic400_library gic400_sfd_headers)

target_link_libraries(gic400_library
    PRIVATE
        Emu68Common::common
)

# ROM-able: no writable .data/.bss may sneak in (see driver-stack TODO.md). For the
# serial backend this instead links debug.lib (+ __divsi3 glue) and skips the check.
emu68_debug_backend_finalize(gic400_library ROMABLE)

set_target_properties(gic400_library PROPERTIES
    OUTPUT_NAME gic400
    SUFFIX ".library"
)

# Install headers
install(
    DIRECTORY include/
    DESTINATION include
    PATTERN "${LIBRARY_BASENAME}_private.h" EXCLUDE
)

install(
    DIRECTORY ${GIC400_GENERATED_INCLUDE_DIR}/
    DESTINATION include
)

# Install targets
install(
    TARGETS gic400_headers gic400_library
    EXPORT GIC400Targets
    RUNTIME DESTINATION LIBS
)

install(
    EXPORT GIC400Targets
    NAMESPACE GIC400::
    FILE GIC400Targets.cmake
    DESTINATION lib/cmake/GIC400
)

# CMake package config
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

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/GIC400Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/GIC400ConfigVersion.cmake
    DESTINATION lib/cmake/GIC400
)
