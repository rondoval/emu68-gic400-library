cmake_minimum_required(VERSION 3.14.0)
project(gic400.library VERSION 1.1.0)

string(TIMESTAMP CURRENT_DATE "(%d.%m.%Y)")
set(VERSTRING "$VER: ${PROJECT_NAME} ${PROJECT_VERSION} ${CURRENT_DATE}")

# Depend on shared Emu68 common library headers/implementation
find_package(Emu68Common REQUIRED)

# Set default install prefix to ./install subdirectory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

add_compile_definitions(PRIVATE LIBRARY_IDSTRING="${VERSTRING}" LIBRARY_VERSION=${PROJECT_VERSION_MAJOR} LIBRARY_REVISION=${PROJECT_VERSION_MINOR} LIBRARY_NAME="${PROJECT_NAME}")

add_compile_options(-Os -m68040 -fomit-frame-pointer -Wall -Wextra -DDEBUG)
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
        $<INSTALL_INTERFACE:include>
)
target_include_directories(gic400_library PRIVATE include)
target_link_libraries(gic400_library
    PRIVATE
        Emu68Common::common
)

set_target_properties(gic400_library PROPERTIES
    OUTPUT_NAME gic400
    SUFFIX ".library"
)

# install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gic400.library DESTINATION GIC400/)

install(
    DIRECTORY include/
    DESTINATION include
    PATTERN "gic400_private.h" EXCLUDE
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
