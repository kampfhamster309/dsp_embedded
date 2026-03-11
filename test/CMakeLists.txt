cmake_minimum_required(VERSION 3.16)
project(dsp_embedded_host C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(COMPONENTS_DIR "${ROOT_DIR}/components")
set(STUBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stubs")

# ---------------------------------------------------------------------------
# Common compile options (mirrors ESP-IDF defaults where relevant)
# ---------------------------------------------------------------------------
add_compile_options(
    -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter   # common in stub implementations
)

# ---------------------------------------------------------------------------
# Host-build preprocessor definitions
#
# DSP_HOST_BUILD=1  – signals to any component that it is running on the host
# ESP_PLATFORM is intentionally NOT defined so dsp_config.h skips sdkconfig.h
# ---------------------------------------------------------------------------
add_compile_definitions(DSP_HOST_BUILD=1)

# ---------------------------------------------------------------------------
# Collect all component include directories
#
# Convention: every component under components/ that exposes an include/
# directory is automatically visible to the host build.  When new components
# are added (DSP-101 onwards) they just need to follow this layout.
# ---------------------------------------------------------------------------
file(GLOB COMPONENT_INCLUDE_DIRS LIST_DIRECTORIES true
    "${COMPONENTS_DIR}/*/include"
)

# ---------------------------------------------------------------------------
# dsp_embedded_host – static library
#
# This library will accumulate all platform-agnostic DSP source files as they
# are added in subsequent milestones.  Components register themselves by
# appending to DSP_HOST_SOURCES via cmake's target_sources() or by editing
# this file.
#
# Currently only the header-only dsp_config component exists, so the library
# has no source files yet.  An empty object file is used as a placeholder so
# CMake can produce a valid archive.
# ---------------------------------------------------------------------------
set(DSP_HOST_PLACEHOLDER "${CMAKE_CURRENT_BINARY_DIR}/dsp_host_placeholder.c")
file(WRITE "${DSP_HOST_PLACEHOLDER}" "/* placeholder – removed once real sources are added */\n")

add_library(dsp_embedded_host STATIC "${DSP_HOST_PLACEHOLDER}")

target_include_directories(dsp_embedded_host PUBLIC
    ${COMPONENT_INCLUDE_DIRS}
    ${STUBS_DIR}
)

# ---------------------------------------------------------------------------
# Test runner executable (populated in DSP-004 with Unity tests)
# ---------------------------------------------------------------------------
add_executable(dsp_test_runner
    test_main.c   # replaced in DSP-004 with Unity runner
)

target_link_libraries(dsp_test_runner PRIVATE dsp_embedded_host)

target_include_directories(dsp_test_runner PRIVATE
    ${COMPONENT_INCLUDE_DIRS}
    ${STUBS_DIR}
)

# ---------------------------------------------------------------------------
# CTest integration (tests registered in DSP-004)
# ---------------------------------------------------------------------------
enable_testing()
add_test(NAME dsp_test_runner COMMAND dsp_test_runner)

# ---------------------------------------------------------------------------
# Build summary
# ---------------------------------------------------------------------------
message(STATUS "DSP Embedded host build")
message(STATUS "  Components : ${COMPONENT_INCLUDE_DIRS}")
message(STATUS "  Stubs      : ${STUBS_DIR}")
message(STATUS "  C standard : C${CMAKE_C_STANDARD}")
