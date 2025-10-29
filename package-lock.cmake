# CPM Package Lock
# This file should be committed to version control

# CPMLicenses.cmake
CPMDeclarePackage(CPMLicenses.cmake
  VERSION 0.0.4
  GITHUB_REPOSITORY TheLartians/CPMLicenses.cmake
)
# Common CMake additions for Amiga
CPMDeclarePackage(CMakeAmigaCommon
  GIT_TAG 1.0.5
  GITHUB_REPOSITORY AmigaPorts/cmake-amiga-common-library
)
# Emu68 devicetree.resource
CPMDeclarePackage(devicetree.resource
  GIT_TAG 54a2e8d068d1cedad00714d5ea45f5ac18f9b0b1
  GITHUB_REPOSITORY michalsc/devicetree.resource
  EXCLUDE_FROM_ALL
)