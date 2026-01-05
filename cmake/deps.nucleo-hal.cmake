# Fetch hal753 library package based on toolchain variant Checksums from:
# https://github.com/sohal/hal753/releases/download/v1.0.1/SHA256SUMS.txt

# Set default toolchain variant if not specified
if(NOT DEFINED TOOLCHAIN_VARIANT)
    set(TOOLCHAIN_VARIANT
        "gnuarm14.3"
        CACHE STRING "Toolchain variant (gnuarm14.3 or atfe21.1)")
endif()

# Use GITHUB_BRANCH_hal753 from cache (set in VersionPresets.json)
if(NOT DEFINED GITHUB_BRANCH_hal753)
    set(GITHUB_BRANCH_hal753
        "v1.0.4"
        CACHE STRING "hal753 release tag")
endif()

# hal753 package version (STM32CubeH7 version, e.g., 1.12.1) This is the version in the package
# filename, not the release tag
if(NOT DEFINED GITHUB_BRANCH_hal753_PACKAGE_VERSION)
    set(GITHUB_BRANCH_hal753_PACKAGE_VERSION
        "1.12.1"
        CACHE STRING "hal753 package version")
endif()

# Extract the CMake package version from GITHUB_BRANCH_hal753 (strip 'v' prefix) This is used for
# find_package() and must match the version in nucleoConfigVersion.cmake
string(REGEX REPLACE "^v" "" HAL753_CMAKE_VERSION "${GITHUB_BRANCH_hal753}")

# The package filename uses the STM32CubeH7 version
set(HAL753_PACKAGE_VERSION "${GITHUB_BRANCH_hal753_PACKAGE_VERSION}")

# Select package URL and checksum based on toolchain
if(TOOLCHAIN_VARIANT STREQUAL "gnuarm14.3")
    set(HAL753_PACKAGE_URL
        "https://github.com/sohal/hal753/releases/download/${GITHUB_BRANCH_hal753}/nucleo-hal-${HAL753_PACKAGE_VERSION}-gnuarm14.3.tar.gz"
    )
    if(DEFINED GITHUB_BRANCH_hal753_SHA256)
        set(HAL753_PACKAGE_SHA256 "${GITHUB_BRANCH_hal753_SHA256}")
    else()
        set(HAL753_PACKAGE_SHA256 "")
    endif()
elseif(TOOLCHAIN_VARIANT STREQUAL "atfe21.1")
    # Use gnuarm14.3 package for atfe21.1 builds (atfe21.1 packages not yet available)
    set(HAL753_PACKAGE_URL
        "https://github.com/sohal/hal753/releases/download/${GITHUB_BRANCH_hal753}/nucleo-hal-${HAL753_PACKAGE_VERSION}-gnuarm14.3.tar.gz"
    )
    if(DEFINED GITHUB_BRANCH_hal753_SHA256)
        set(HAL753_PACKAGE_SHA256 "${GITHUB_BRANCH_hal753_SHA256}")
    else()
        set(HAL753_PACKAGE_SHA256 "")
    endif()
else()
    message(
        FATAL_ERROR
            "Invalid TOOLCHAIN_VARIANT: ${TOOLCHAIN_VARIANT}. Must be 'gnuarm14.3' or 'atfe21.1'")
endif()

message(STATUS "Fetching hal753 v${HAL753_PACKAGE_VERSION} for ${TOOLCHAIN_VARIANT}")

# Fetch the hal753 package
cpmaddpackage(
    NAME
    nucleo-hal
    VERSION
    ${HAL753_CMAKE_VERSION}
    URL
    ${HAL753_PACKAGE_URL}
    DOWNLOAD_ONLY
    YES)

# Add the package to CMake prefix path for find_package()
if(nucleo-hal_ADDED)
    list(APPEND CMAKE_PREFIX_PATH "${nucleo-hal_SOURCE_DIR}")
    message(STATUS "hal753 package extracted to: ${nucleo-hal_SOURCE_DIR}")
endif()
