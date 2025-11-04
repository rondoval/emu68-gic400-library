# CPM Package Lock
# This file should be committed to version control

# CPMLicenses.cmake
CPMDeclarePackage(CPMLicenses.cmake
  VERSION 0.0.4
  GITHUB_REPOSITORY TheLartians/CPMLicenses.cmake
)
# Common CMake additions for Amiga
CPMDeclarePackage(CMakeAmigaCommon
  GIT_TAG 1.0.7
  GITHUB_REPOSITORY AmigaPorts/cmake-amiga-common-library
)
# Emu68 devicetree.resource
CPMDeclarePackage(devicetree.resource
  GIT_TAG 943f3b522b1fb049ae095a1e6dee203d43a6b11b
  GITHUB_REPOSITORY michalsc/devicetree.resource
  EXCLUDE_FROM_ALL YES
)