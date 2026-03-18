# Findsnap7.cmake
# Locates the snap7 library in the project-local snap7/ directory.
#
# Imported target: snap7::snap7
# Variables set:
#   snap7_FOUND
#   snap7_INCLUDE_DIRS
#   snap7_LIBRARIES
#   snap7_DLL  (Windows only – path to snap7.dll for POST_BUILD copy)

cmake_minimum_required(VERSION 3.20)

set(_snap7_root "${CMAKE_SOURCE_DIR}/snap7")

find_path(snap7_INCLUDE_DIR
    NAMES snap7.h
    PATHS "${_snap7_root}/include"
    NO_DEFAULT_PATH
)

if(WIN32)
    find_library(snap7_LIBRARY
        NAMES snap7
        PATHS "${_snap7_root}/lib"
        NO_DEFAULT_PATH
    )
    find_file(snap7_DLL
        NAMES snap7.dll
        PATHS "${_snap7_root}/lib"
              "${_snap7_root}/bin"
        NO_DEFAULT_PATH
    )
else()
    find_library(snap7_LIBRARY
        NAMES snap7
        PATHS "${_snap7_root}/lib"
        NO_DEFAULT_PATH
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(snap7
    REQUIRED_VARS snap7_INCLUDE_DIR snap7_LIBRARY
)

if(snap7_FOUND AND NOT TARGET snap7::snap7)
    add_library(snap7::snap7 SHARED IMPORTED)
    set_target_properties(snap7::snap7 PROPERTIES
        IMPORTED_LOCATION   "${snap7_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${snap7_INCLUDE_DIR}"
    )
    if(WIN32 AND snap7_LIBRARY)
        set_target_properties(snap7::snap7 PROPERTIES
            IMPORTED_IMPLIB "${snap7_LIBRARY}"
        )
        if(snap7_DLL)
            set_target_properties(snap7::snap7 PROPERTIES
                IMPORTED_LOCATION "${snap7_DLL}"
            )
        endif()
    endif()
    set(snap7_INCLUDE_DIRS "${snap7_INCLUDE_DIR}")
    set(snap7_LIBRARIES    "${snap7_LIBRARY}")
endif()

mark_as_advanced(snap7_INCLUDE_DIR snap7_LIBRARY snap7_DLL)
