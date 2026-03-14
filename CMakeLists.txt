cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(240217-221926-esp32s3-woofer)
# Bring in threading_alt.h to the TLS libraries
target_include_directories(mbedcrypto BEFORE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
