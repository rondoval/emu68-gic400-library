cmake_minimum_required(VERSION 3.14.0)
project(gic400.library VERSION 1.0.0)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CPM-init)

get_verstring(VERSTRING)

if(NOT TARGET devicetree)
    CPMGetPackage(devicetree.resource)
endif()

# Set default install prefix to ./install subdirectory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

add_subdirectory(common)

sfdc(sfd/gic400_lib.sfd)

add_executable(gic400.library
    src/gic400_main.c
    src/gic400_distributor.c
    src/gic400_api.c
    src/gic400_end.c
)

target_compile_definitions(gic400.library PRIVATE LIBRARY_IDSTRING="${VERSTRING}" LIBRARY_VERSION=${PROJECT_VERSION_MAJOR} LIBRARY_REVISION=${PROJECT_VERSION_MINOR} LIBRARY_NAME="${PROJECT_NAME}")
add_dependencies(gic400.library devicetree gic400)
target_link_libraries(gic400.library common gic400 devicetree)
target_link_options(gic400.library PRIVATE -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -s -Wl,-e,_doNotExecute)
target_include_directories(gic400.library PRIVATE include)
target_compile_options(gic400.library PUBLIC $<$<COMPILE_LANGUAGE:C>:-O3 -m68040 -fomit-frame-pointer -Wall -Wextra -DDEBUG>)

# install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gic400.library DESTINATION GIC400/)
