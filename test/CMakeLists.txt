cmake_minimum_required(VERSION 3.16)
project(dsp_embedded_host C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
set(ROOT_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(COMPONENTS_DIR "${ROOT_DIR}/components")
set(STUBS_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/stubs")
set(UNITY_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/unity")

# ---------------------------------------------------------------------------
# Sanity check
# ---------------------------------------------------------------------------
if(NOT EXISTS "${UNITY_DIR}/src/unity.c")
    message(FATAL_ERROR
        "Unity not found at ${UNITY_DIR}.\n"
        "Run: git submodule update --init test/unity")
endif()

# ---------------------------------------------------------------------------
# Common compile options
# ---------------------------------------------------------------------------
add_compile_options(
    -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter
)

# ---------------------------------------------------------------------------
# Host-build preprocessor definitions
# ESP_PLATFORM intentionally absent so dsp_config.h skips sdkconfig.h
# ---------------------------------------------------------------------------
add_compile_definitions(DSP_HOST_BUILD=1)

# ---------------------------------------------------------------------------
# Collect component include dirs automatically
# Convention: every components/*/include/ is included.
# New components need no changes here.
# ---------------------------------------------------------------------------
file(GLOB COMPONENT_INCLUDE_DIRS LIST_DIRECTORIES true
    "${COMPONENTS_DIR}/*/include"
)

# ---------------------------------------------------------------------------
# Unity static library
# ---------------------------------------------------------------------------
add_library(unity STATIC "${UNITY_DIR}/src/unity.c")

target_include_directories(unity PUBLIC "${UNITY_DIR}/src")

# Unity's default output via putchar is fine for host; no config overrides needed.

# ---------------------------------------------------------------------------
# dsp_embedded_host – accumulates all platform-agnostic DSP sources
# Components add themselves via target_sources() as they are implemented.
# Currently header-only (dsp_config), so a placeholder keeps CMake happy.
# ---------------------------------------------------------------------------
set(DSP_HOST_PLACEHOLDER "${CMAKE_CURRENT_BINARY_DIR}/dsp_host_placeholder.c")
file(WRITE "${DSP_HOST_PLACEHOLDER}" "/* placeholder removed once real sources are added */\n")

add_library(dsp_embedded_host STATIC "${DSP_HOST_PLACEHOLDER}")

target_include_directories(dsp_embedded_host PUBLIC
    ${COMPONENT_INCLUDE_DIRS}
    ${STUBS_DIR}
)

# ---------------------------------------------------------------------------
# Test runner executable
#
# test_main.c       – Unity runner: main(), UNITY_BEGIN/END, RUN_TEST calls
# test_smoke.c      – Smoke tests (always pass, verify pipeline)
#
# Add new test_*.c files here as components are implemented.
# ---------------------------------------------------------------------------
add_executable(dsp_test_runner
    test_main.c
    test_smoke.c
    test_dsp_tls.c
    test_dsp_tls_tickets.c
    "${ROOT_DIR}/components/dsp_mbedtls/dsp_tls.c"
    test_dsp_http.c
    "${ROOT_DIR}/components/dsp_http/dsp_http.c"
    test_dsp_mem.c
    "${ROOT_DIR}/components/dsp_mem/dsp_mem.c"
    test_dsp_wifi.c
    "${ROOT_DIR}/components/dsp_wifi/dsp_wifi.c"
    test_dsp_identity.c
    "${ROOT_DIR}/components/dsp_identity/dsp_identity.c"
    test_dsp_jwt.c
    "${ROOT_DIR}/components/dsp_jwt/dsp_jwt.c"
    test_dsp_psk.c
    "${ROOT_DIR}/components/dsp_psk/dsp_psk.c"
    test_dsp_daps.c
    "${ROOT_DIR}/components/dsp_daps/dsp_daps.c"
    test_dsp_json.c
    "${ROOT_DIR}/components/dsp_json/dsp_json.c"
    "${ROOT_DIR}/components/dsp_json/cjson/cJSON.c"
    test_dsp_jsonld.c
    test_dsp_msg.c
    "${ROOT_DIR}/components/dsp_msg/dsp_msg.c"
    test_dsp_build.c
    "${ROOT_DIR}/components/dsp_build/dsp_build.c"
    test_dsp_catalog.c
    "${ROOT_DIR}/components/dsp_catalog/dsp_catalog.c"
    test_dsp_neg.c
    "${ROOT_DIR}/components/dsp_neg/dsp_neg.c"
)

target_link_libraries(dsp_test_runner PRIVATE
    dsp_embedded_host
    unity
)

target_include_directories(dsp_test_runner PRIVATE
    ${COMPONENT_INCLUDE_DIRS}
    "${COMPONENTS_DIR}/dsp_json/cjson"
    ${STUBS_DIR}
    ${UNITY_DIR}/src
)

# ---------------------------------------------------------------------------
# dsp_test_no_tickets – session tickets DISABLED path
#
# Compiled with -DCONFIG_DSP_TLS_SESSION_TICKETS=0, overriding the
# default-1 fallback in dsp_config.h.  Exercises the #else branches in
# dsp_tls.c (no ticket_ctx member, no ticket_free call in deinit).
# ---------------------------------------------------------------------------
add_executable(dsp_test_no_tickets
    test_tickets_off_main.c
    test_tickets_off.c
    "${ROOT_DIR}/components/dsp_mbedtls/dsp_tls.c"
)

target_compile_definitions(dsp_test_no_tickets PRIVATE
    CONFIG_DSP_TLS_SESSION_TICKETS=0
)

target_link_libraries(dsp_test_no_tickets PRIVATE
    dsp_embedded_host
    unity
)

target_include_directories(dsp_test_no_tickets PRIVATE
    ${COMPONENT_INCLUDE_DIRS}
    ${STUBS_DIR}
    ${UNITY_DIR}/src
)

# ---------------------------------------------------------------------------
# dsp_test_terminate_on – negotiate-terminate ENABLED path
#
# Compiled with -DCONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE=1, overriding the
# default-0 fallback in dsp_config.h.  Exercises the #if block that registers
# POST /negotiations/terminate and its host stub in dsp_neg.c.
# ---------------------------------------------------------------------------
add_executable(dsp_test_terminate_on
    test_terminate_on_main.c
    test_terminate_on.c
    "${ROOT_DIR}/components/dsp_neg/dsp_neg.c"
    "${ROOT_DIR}/components/dsp_http/dsp_http.c"
)

target_compile_definitions(dsp_test_terminate_on PRIVATE
    CONFIG_DSP_ENABLE_NEGOTIATE_TERMINATE=1
)

target_link_libraries(dsp_test_terminate_on PRIVATE
    dsp_embedded_host
    unity
)

target_include_directories(dsp_test_terminate_on PRIVATE
    ${COMPONENT_INCLUDE_DIRS}
    ${STUBS_DIR}
    ${UNITY_DIR}/src
)

# ---------------------------------------------------------------------------
# CTest
# ---------------------------------------------------------------------------
enable_testing()
add_test(NAME dsp_test_runner      COMMAND dsp_test_runner)
add_test(NAME dsp_test_no_tickets  COMMAND dsp_test_no_tickets)
add_test(NAME dsp_test_terminate_on COMMAND dsp_test_terminate_on)

# ---------------------------------------------------------------------------
# Build summary
# ---------------------------------------------------------------------------
message(STATUS "DSP Embedded host build")
message(STATUS "  Components : ${COMPONENT_INCLUDE_DIRS}")
message(STATUS "  Stubs      : ${STUBS_DIR}")
message(STATUS "  Unity      : ${UNITY_DIR} (v2.6.0)")
message(STATUS "  C standard : C${CMAKE_C_STANDARD}")
