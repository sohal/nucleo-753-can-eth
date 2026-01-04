# Fetch hal753 library package based on toolchain variant
# Checksums from: https://github.com/sohal/hal753/releases/download/v1.0.1/SHA256SUMS.txt

# Set default toolchain variant if not specified
if(NOT DEFINED TOOLCHAIN_VARIANT)
    set(TOOLCHAIN_VARIANT "gnuarm14.3" CACHE STRING "Toolchain variant (gnuarm14.3 or atfe21.1)")
endif()

# hal753 package version
set(HAL753_VERSION "1.12.1")

# Select package URL and checksum based on toolchain
if(TOOLCHAIN_VARIANT STREQUAL "gnuarm14.3")
    set(HAL753_PACKAGE_URL "https://github.com/sohal/hal753/releases/download/v1.0.1/nucleo-hal-${HAL753_VERSION}-gnuarm14.3.tar.gz")
    set(HAL753_PACKAGE_SHA256 "b7763477739c3dc7180293e3fe0d049ff653cb05e1ab6ac69152fe94a5142945")
elseif(TOOLCHAIN_VARIANT STREQUAL "atfe21.1")
    set(HAL753_PACKAGE_URL "https://github.com/sohal/hal753/releases/download/v1.0.1/nucleo-hal-${HAL753_VERSION}-atfe21.1.tar.gz")
    # TODO: Add SHA256 checksum when atfe21.1 package is available
    set(HAL753_PACKAGE_SHA256 "")
    message(WARNING "atfe21.1 package checksum not yet available")
else()
    message(FATAL_ERROR "Invalid TOOLCHAIN_VARIANT: ${TOOLCHAIN_VARIANT}. Must be 'gnuarm14.3' or 'atfe21.1'")
endif()

message(STATUS "Fetching hal753 v${HAL753_VERSION} for ${TOOLCHAIN_VARIANT}")

# Fetch the hal753 package
CPMAddPackage(
    NAME nucleo-hal
    VERSION ${HAL753_VERSION}
    URL ${HAL753_PACKAGE_URL}
    URL_HASH SHA256=${HAL753_PACKAGE_SHA256}
    DOWNLOAD_ONLY YES
)

# Add the package to CMake prefix path for find_package()
if(nucleo-hal_ADDED)
    list(APPEND CMAKE_PREFIX_PATH "${nucleo-hal_SOURCE_DIR}")
    message(STATUS "hal753 package extracted to: ${nucleo-hal_SOURCE_DIR}")
endif()
