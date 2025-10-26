cmake_minimum_required(VERSION 3.14.0)
project(gic400.library VERSION 1.0.0)

string(TIMESTAMP CURRENT_DATE "(%d.%m.%Y)")
set(VERSTRING "$VER: ${PROJECT_NAME} ${PROJECT_VERSION} ${CURRENT_DATE}")

# Set default install prefix to ./install subdirectory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

add_compile_definitions(PRIVATE LIBRARY_IDSTRING="${VERSTRING}" LIBRARY_VERSION=${PROJECT_VERSION_MAJOR} LIBRARY_REVISION=${PROJECT_VERSION_MINOR} LIBRARY_NAME="${PROJECT_NAME}")

add_subdirectory(common)

add_compile_options(-Os -m68040 -fomit-frame-pointer -Wall -Wextra -DDEBUG)
add_link_options(-ffreestanding -nostdlib -nostartfiles -Wall -Wextra -s -Wl,-e,_doNotExecute)

add_library(gic400 INTERFACE)
add_executable(gic400.library
    src/gic400_main.c
    src/gic400_distributor.c
    src/gic400_api.c
    src/gic400_end.c
)

target_include_directories(gic400 INTERFACE include)
target_include_directories(gic400.library PRIVATE include)
target_link_libraries(gic400.library common)

# install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gic400.library DESTINATION GIC400/)
