cmake_minimum_required(VERSION 3.20)
project(s7prtgsensor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Find snap7 ────────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(snap7 REQUIRED)

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/address_parser.cpp
    src/s7client.cpp
    src/value_extractor.cpp
    src/prtg_output.cpp
)

add_executable(s7sensor ${SOURCES})

# ── Include paths ─────────────────────────────────────────────────────────────
target_include_directories(s7sensor PRIVATE
    src/
    third_party/
)

# ── Link snap7 ────────────────────────────────────────────────────────────────
target_link_libraries(s7sensor PRIVATE snap7::snap7)

# ── Compiler options ──────────────────────────────────────────────────────────
if(MSVC)
    # Static CRT to minimize runtime dependencies
    set_property(TARGET s7sensor PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(s7sensor PRIVATE /W4 /WX- /utf-8)
else()
    # MinGW / GCC / Clang
    target_compile_options(s7sensor PRIVATE -Wall -Wextra)
    target_link_options(s7sensor PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()

# ── POST_BUILD: copy snap7.dll next to the exe (Windows) ─────────────────────
if(WIN32)
    get_target_property(_snap7_dll snap7::snap7 IMPORTED_LOCATION)
    if(_snap7_dll)
        add_custom_command(TARGET s7sensor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_snap7_dll}"
                "$<TARGET_FILE_DIR:s7sensor>/snap7.dll"
            COMMENT "Copying snap7.dll to output directory"
        )
    endif()
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS s7sensor RUNTIME DESTINATION .)
if(WIN32 AND _snap7_dll)
    install(FILES "${_snap7_dll}" DESTINATION .)
endif()
