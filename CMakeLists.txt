cmake_minimum_required(VERSION 3.25)

# Extract version from git tags
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_TAG MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        set(VERSION_MAJOR ${CMAKE_MATCH_1})
        set(VERSION_MINOR ${CMAKE_MATCH_2})
        set(VERSION_PATCH ${CMAKE_MATCH_3})
    else()
        set(VERSION_MAJOR 0)
        set(VERSION_MINOR 1)
        set(VERSION_PATCH 0)
    endif()
else()
    set(VERSION_MAJOR 0)
    set(VERSION_MINOR 1)
    set(VERSION_PATCH 0)
endif()

# Set the location of all fetched dependencies
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/_deps")

# Download CPM.cmake v0.42.0
file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/${GITHUB_BRANCH_cpm}/CPM.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake"
    EXPECTED_HASH SHA256=${GITHUB_BRANCH_cpm_SHA256}
)

include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# Include toolchain dependency (downloads cmake_scripts with toolchain files)
include(cmake/deps.toolchain.cmake)

# Include hal753 dependency (must be before project() to set toolchain)
include(cmake/deps.nucleo-hal.cmake)

project(nucleo-753-can-eth
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    LANGUAGES C ASM
)

# Find the hal753 package (version set in cmake/deps.nucleo-hal.cmake)
find_package(nucleo ${HAL753_CMAKE_VERSION} REQUIRED)

# Create the firmware executable
add_executable(${PROJECT_NAME}
    # Active Objects
    ao/blinky/blinky.c
    ao/mongoose/mongoose_ao.c
    ao/stack_monitor/stack_monitor_ao.c
    
    # QP/C Framework Adapter (provides application_init entry point)
    frameworks/qpc/qpc-adapter.c
    
    # Mongoose Network Framework
    frameworks/mongoose/mongoose.c
    frameworks/mongoose/mongoose_glue.c
    frameworks/mongoose/mongoose_impl.c
    frameworks/mongoose/mongoose_fs.c
    
    # System calls (newlib stubs)
    ao/common/syscalls.c
    ao/common/sysmem.c
    
    # Startup code
    linker/startup_stm32h753xx.s
)

# add external libraries
add_library (qpc STATIC IMPORTED)
set_target_properties (qpc PROPERTIES
    IMPORTED_LOCATION "${nucleo-hal_SOURCE_DIR}/lib/libqpc.a"
)
target_include_directories(qpc INTERFACE
    "${nucleo-hal_SOURCE_DIR}/include/qpc/include"
    "${nucleo-hal_SOURCE_DIR}/include/qpc/config"
    "${nucleo-hal_SOURCE_DIR}/include/qpc/port"
)

add_library (h753 STATIC IMPORTED)
set_target_properties (h753 PROPERTIES
    IMPORTED_LOCATION "${nucleo-hal_SOURCE_DIR}/lib/libnucleo-h753.a"
)
target_include_directories(h753 INTERFACE
        "${nucleo-hal_SOURCE_DIR}/include/stm32cubeh7/Drivers/STM32H7xx_HAL_Driver/Inc"
        "${nucleo-hal_SOURCE_DIR}/include/stm32cubeh7/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy"
        "${nucleo-hal_SOURCE_DIR}/include/stm32cubeh7/Drivers/CMSIS/Device/ST/STM32H7xx/Include"
        "${nucleo-hal_SOURCE_DIR}/include/stm32cubeh7/Drivers/CMSIS/Include"
        "${nucleo-hal_SOURCE_DIR}/include/stm32cubeh7/Drivers/CMSIS/Core/Include"
        "${nucleo-hal_SOURCE_DIR}/include/nucleo"
)

# Link with nucleo HAL and QPC framework
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        qpc
        h753
)

# Add include directories for Active Objects and frameworks
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/common
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/blinky
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/mongoose
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/stack_monitor
        ${CMAKE_CURRENT_SOURCE_DIR}/frameworks/qpc
        ${CMAKE_CURRENT_SOURCE_DIR}/frameworks/mongoose
)

# Add compile definitions for STM32 device and USE_HAL_DRIVER
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        STM32H753xx
        USE_HAL_DRIVER
)


# Set compile options

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -mcpu=cortex-m7
        -mthumb
        -mfpu=fpv5-d16
        -mfloat-abi=hard
        -Os
        -g3
        -Wall
        -Wextra
        #-Wpedantic
        #-Werror

)
# Set linker script
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32H753XX_FLASH.ld)
target_link_options(${PROJECT_NAME}
    PRIVATE
        -T${LINKER_SCRIPT}
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
        -Wl,--print-memory-usage
        --specs=nano.specs   # Use newlib-nano for smaller code size
)

# Ensure linker script is in the dependencies
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# Post-build: Generate hex and bin files, display size
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} -B $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin, displaying size"
)

# Print build information
message(STATUS "")
message(STATUS "===========================================")
message(STATUS "  Project: ${PROJECT_NAME}")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Toolchain: ${TOOLCHAIN_VARIANT}")
message(STATUS "  hal753 version: 1.12.1")
message(STATUS "===========================================")
message(STATUS "")
