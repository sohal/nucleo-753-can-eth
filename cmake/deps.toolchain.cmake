# dependency on the toolchain is brought by CPM as a package
CPMAddPackage(
  NAME cmake_scripts
  URL https://github.com/kodezine/cmake_scripts/archive/refs/tags/${GITHUB_BRANCH_toolchain}.tar.gz
  URL_HASH SHA256=${GITHUB_BRANCH_toolchain_SHA256}
  OPTIONS
    DOWNLOAD_ONLY TRUE
)
