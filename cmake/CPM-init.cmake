include(CPM)

set(CPM_SOURCE_CACHE "${PROJECT_SOURCE_DIR}/external")
CPMUsePackageLock(package-lock.cmake)

CPMGetPackage(CPMLicenses.cmake)
CPMGetPackage(CMakeAmigaCommon)

if(NOT TARGET write-licenses)
	cpm_licenses_create_disclaimer_target(
		write-licenses "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/third_party.txt" "${CPM_PACKAGES}"
	)
endif()
