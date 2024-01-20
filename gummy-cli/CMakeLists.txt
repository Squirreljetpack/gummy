# Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
# SPDX-License-Identifier: GPL-3.0-or-later

project(gummy-cli LANGUAGES CXX)

option(GUMMY_EXTERNAL_CLI11  "" ON)

if (GUMMY_EXTERNAL_CLI11)
find_package(CLI11 2.1.2 REQUIRED)
else()
set(CLI11_PRECOMPILED ON)
FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    GIT_TAG 291c58789c031208f08f4f261a858b5b7083e8e2 #v2.3.2
)
FetchContent_MakeAvailable(cli11)
endif()

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE gummy-cli/gummy-cli.cpp ../gummyd/gummyd/file.hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    fmt::fmt
    spdlog::spdlog
    libgummyd
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    VERSION="${GUMMY_VERSION}"
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME gummy)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
