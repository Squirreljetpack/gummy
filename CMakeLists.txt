cmake_minimum_required(VERSION 3.14)
project(gummy VERSION 0.4 LANGUAGES CXX)

include(GNUInstallDirs)
include(FetchContent)
include(CPack)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_Declare(fmt  URL https://github.com/fmtlib/fmt/releases/download/9.1.0/fmt-9.1.0.zip)

FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(fmt)

add_library(config STATIC src/gummyd/config.cpp include/config.hpp include/file.hpp)
target_include_directories(config PRIVATE ${INCLUDE_DIR})
target_link_libraries(config PRIVATE nlohmann_json::nlohmann_json)
target_compile_features(config PRIVATE cxx_std_20)

set(MAIN_VERSION "${PROJECT_VERSION}")
add_subdirectory(src/gummyd)
add_subdirectory(src/gummy)

set(
    CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

set(CPACK_GENERATOR "DEB")
set(CPACK_STRIP_FILES YES)
set(CPACK_PACKAGE_CONTACT "fushkodev@outlook.it")

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_PACKAGE_DESCRIPTION "automatic and manual adjustment of screen brightness and temperature.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "X11 screen manager")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Francesco Fusco <fushkodev@outlook.it>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/Fushko/gummy")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxext-dev libxcb-randr0-dev libsdbus-c++-dev")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)


