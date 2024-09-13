# Copyright (c) 2022 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19)

# Set default path for Mbed OS library (no force-write for override)
set(MBED_OS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/mbed-os CACHE INTERNAL "")

# Set default path for mbed_app.json5 (no force-write for override)
set(MBED_APP_JSON_PATH mbed_app.json5 CACHE INTERNAL "")

# Set default path for custom_targets.json5 (no force-write for override)
set(CUSTOM_TARGETS_PATH custom_targets CACHE INTERNAL "")
set(CUSTOM_TARGETS_JSON_PATH ${CUSTOM_TARGETS_PATH}/custom_targets.json5 CACHE INTERNAL "")

# Include Mbed toolchain setup file
include(mbed-os/tools/cmake/mbed_toolchain_setup.cmake)

set(APP_PROJECT MBED_CE_MCUBOOT_DEMO)
set(APP_TARGET mbed-ce-mcuboot-demo)

# Set up project name
project(${APP_PROJECT})

# Include Mbed project setup file
include(mbed_project_setup)

# Add for custom targets
add_subdirectory(${CUSTOM_TARGETS_PATH})

# Add Mbed OS library
add_subdirectory(${MBED_OS_PATH})

# Add other libraries
add_subdirectory(mcuboot/boot/bootutil)
add_subdirectory(mcuboot/boot/mbed)
if("COMPONENT_NUSD=1" IN_LIST MBED_TARGET_DEFINITIONS)
    add_subdirectory(drivers/COMPONENT_NUSD)
endif()

# User-provided MBEDTLS_USER_CONFIG_FILE for mbedtls
target_include_directories(mbed-mbedtls
    PUBLIC
        .
)

add_executable(${APP_TARGET})

target_sources(${APP_TARGET}
    PRIVATE
        secondary_bd.cpp
        enc_key.c
        signing_keys.c
        shared_data.c
)

target_link_libraries(${APP_TARGET}
    PRIVATE
        mbed-baremetal        
        mbed-mcuboot
        mbed-mbedtls
        $<$<IN_LIST:COMPONENT_DATAFLASH=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-dataflash>
        $<$<IN_LIST:DEVICE_FLASH=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-flashiap>
        $<$<IN_LIST:COMPONENT_I2CEE=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-i2cee>
        $<$<IN_LIST:COMPONENT_OSPIF=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-ospif>
        $<$<IN_LIST:COMPONENT_QSPIF=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-qspif>
        $<$<IN_LIST:COMPONENT_SD=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-sd>
        $<$<IN_LIST:COMPONENT_SPIF=1,${MBED_TARGET_DEFINITIONS}>:mbed-storage-spif>
        $<$<IN_LIST:COMPONENT_NUSD=1,${MBED_TARGET_DEFINITIONS}>:nusd>
)

# Must call this for each target to set up bin file creation, code upload, etc
mbed_set_post_build(${APP_TARGET})
