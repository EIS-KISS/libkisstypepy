cmake_minimum_required(VERSION 3.23)

project(kisstypepy VERSION "1.0.0" LANGUAGES C CXX)

set(KISSTYPEPY_VERSION "${PROJECT_VERSION}")

# The kisstype objects are linked into a python extension module, so they must be position independent
# regardless of what the upstream build flags happen to be.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(SKBUILD)
	if(NOT Python_EXECUTABLE AND PYTHON_EXECUTABLE)
		set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
	endif()
	execute_process(
		COMMAND "${Python_EXECUTABLE}" -c
		"import pybind11; print(pybind11.get_cmake_dir())"
		OUTPUT_VARIABLE _tmp_dir
		OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ECHO STDOUT)
	list(APPEND CMAKE_PREFIX_PATH "${_tmp_dir}")
endif()

find_package(pybind11 CONFIG REQUIRED)

# kisstype is vendored as a submodule in ./kisstype, if it is not checked out fall back to an
# installed libkisstype.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/kisstype/CMakeLists.txt)
	add_subdirectory(kisstype)
	set(KISSTYPE_TARGET kisstype_static)
else()
	message(STATUS "No ./kisstype checkout found, looking for an installed libkisstype instead")
	find_package(PkgConfig REQUIRED)
	pkg_search_module(KISSTYPE REQUIRED libkisstype)
	# libkisstype.pc points at <prefix>/include/kisstype, the headers are included as
	# <kisstype/type.h> though, so the parent directory is needed as well.
	get_filename_component(KISSTYPE_INCLUDE_PARENT "${KISSTYPE_INCLUDE_DIRS}" DIRECTORY)
	add_library(kisstype_imported INTERFACE IMPORTED)
	set_target_properties(kisstype_imported PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${KISSTYPE_INCLUDE_DIRS};${KISSTYPE_INCLUDE_PARENT}"
		INTERFACE_LINK_DIRECTORIES "${KISSTYPE_LIBRARY_DIRS}"
		INTERFACE_LINK_LIBRARIES "${KISSTYPE_LINK_LIBRARIES}")
	set(KISSTYPE_TARGET kisstype_imported)
endif()

pybind11_add_module(_core MODULE src/main.cpp)
target_compile_definitions(_core PRIVATE KISSTYPEPY_VERSION="${KISSTYPEPY_VERSION}")
target_link_libraries(_core PUBLIC ${KISSTYPE_TARGET})
# Keep the statically linked kisstype symbols private to this extension module. Otherwise they can
# be interposed by (or interpose) the copy of kisstype that is linked into eisgenerator._core.
set_target_properties(_core PROPERTIES
	CXX_VISIBILITY_PRESET hidden
	C_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN ON)
install(TARGETS _core DESTINATION .)
