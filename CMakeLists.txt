cmake_minimum_required(VERSION 3.14.0)
project(gic400.library VERSION 1.0.0)

# Set default install prefix to ./install subdirectory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install path" FORCE)
endif()

add_subdirectory(common)

add_compile_options(-ffreestanding -nostdlib -nostartfiles -Os -m68040 -fomit-frame-pointer -Wall -Wno-unused-function -Wno-shift-count-overflow -DDEBUG)
file(GLOB_RECURSE gic400_sources src/*.c include/*.h)
add_library(gic400 STATIC ${gic400_sources})

target_include_directories(gic400 PUBLIC include)
target_link_libraries(gic400 PRIVATE common)

# install(FILES ${CMAKE_SOURCE_DIR}/Install DESTINATION .)
# install(FILES ${CMAKE_SOURCE_DIR}/Install.info DESTINATION .)
