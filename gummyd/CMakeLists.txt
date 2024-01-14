# Copyright 2021-2024 Francesco Fusco
# SPDX-License-Identifier: GPL-3.0-or-later

project(gummyd LANGUAGES CXX)

# API
add_library(libgummyd STATIC gummyd/api.hpp gummyd/api.cpp gummyd/file.cpp gummyd/constants.cpp gummyd/utils.cpp)
set_target_properties(libgummyd PROPERTIES PREFIX "")
target_include_directories(libgummyd PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(libgummyd PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

set(CMAKE_INSTALL_DAEMON_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/gummyd")
message("-- Daemon path: " ${CMAKE_INSTALL_DAEMON_PATH})
target_compile_definitions(libgummyd PRIVATE CMAKE_INSTALL_DAEMON_PATH="${CMAKE_INSTALL_DAEMON_PATH}")
target_compile_features(libgummyd PRIVATE cxx_std_20)

add_subdirectory(gummyd)
