cmake_minimum_required(VERSION 3.14)
project(gummy VERSION 0.4 LANGUAGES CXX)
include(GNUInstallDirs)
include(FetchContent)

set(JSON_ImplicitConversions OFF)
set(CLI11_PRECOMPILED ON)
set(SPDLOG_FMT_EXTERNAL ON)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_Declare(fmt
    URL https://github.com/fmtlib/fmt/releases/download/9.1.0/fmt-9.1.0.zip
)
FetchContent_Declare(spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz
)
FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    GIT_TAG 291c58789c031208f08f4f261a858b5b7083e8e2 #v2.3.2
)
FetchContent_MakeAvailable(
    json
    fmt
    spdlog
    cli11
)

set(
    CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

set(CPACK_GENERATOR "DEB")
set(CPACK_STRIP_FILES YES)

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "X11 screen manager")
set(CPACK_PACKAGE_DESCRIPTION "adaptive screen brightness/temperature manager")
set(CPACK_PACKAGE_CONTACT "fushkodev@outlook.it")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

set(CPACK_DEBIAN_PACKAGE_NAME "gummy")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Francesco Fusco <fushkodev@outlook.it>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/Fushko/gummy")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
#set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
#set(CPACK_SOURCE_IGNORE_FILES
#    /.git
#    /.*build.*
#    .swp
#    .*~
#    .user
#    )

include(CPack)

add_subdirectory(gummy)
