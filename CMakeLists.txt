# Copyright 2021-2024 Francesco Fusco
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.14)
project(gummy VERSION 0.5.7 LANGUAGES CXX)
set(GUMMY_VERSION ${PROJECT_VERSION})
include(GNUInstallDirs)
include(FetchContent)

# Fix obnoxious warnings when using FetchContent on new CMake versions
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
cmake_policy(SET CMP0135 NEW)
endif()

set(JSON_ImplicitConversions OFF)
set(SPDLOG_FMT_EXTERNAL ON)

option(GUMMY_EXTERNAL_JSON   "" ON)
option(GUMMY_EXTERNAL_FMT    "" ON)
option(GUMMY_EXTERNAL_SPDLOG "" ON)

if (GUMMY_EXTERNAL_JSON)
find_package(nlohmann_json 3.10.5 REQUIRED)
else()
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)
endif()

if (GUMMY_EXTERNAL_FMT)
find_package(fmt 8.1.1 REQUIRED)
else()
FetchContent_Declare(fmt
    URL https://github.com/fmtlib/fmt/releases/download/10.1.1/fmt-10.1.1.zip
)
FetchContent_MakeAvailable(fmt)
endif()

if (GUMMY_EXTERNAL_SPDLOG)
find_package(spdlog 1.9.2 REQUIRED)
else()
FetchContent_Declare(spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.tar.gz
)
FetchContent_MakeAvailable(spdlog)
endif()

set(
    CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

set(CPACK_GENERATOR "DEB")
set(CPACK_STRIP_FILES YES)

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "screen manager")
set(CPACK_PACKAGE_DESCRIPTION "adaptive screen brightness/temperature manager")
set(CPACK_PACKAGE_CONTACT "f.fusco@pm.me")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

set(CPACK_DEBIAN_PACKAGE_NAME "gummy")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Francesco Fusco <f.fusco@pm.me>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/f-fusco/gummy")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /.*build.*
    .swp
    .*~
    .user
    )

include(CPack)

add_subdirectory(gummyd)
add_subdirectory(gummy-cli)
