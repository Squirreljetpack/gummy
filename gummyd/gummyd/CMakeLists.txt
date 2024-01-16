# Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
	PRIVATE
	gamma.cpp
	gamma.hpp
    sd-dbus.hpp
    sd-dbus.cpp
    sd-sysfs.hpp
    sd-sysfs.cpp
    sd-sysfs-devices.cpp
    sd-sysfs-devices.hpp
	core.cpp
	core.hpp
	config.cpp
	config.hpp
    utils.hpp
    utils.cpp
    constants.hpp
    constants.cpp
    file.hpp
    file.cpp
    easing.hpp
    easing.cpp
    channel.hpp
    channel.cpp
    time.hpp
    time.cpp
    x11-xcb.hpp
    x11-xcb.cpp
    ddc.hpp
    ddc.cpp
    gummyd.cpp
)

find_package(sdbus-c++ 1.1.0 REQUIRED)

# C Libraries
find_library(LIBDDCUTIL   "ddcutil"   REQUIRED)
find_library(LIBSYSTEMD   "systemd"   REQUIRED)
find_library(LIBXCB       "xcb"       REQUIRED)
find_library(LIBXCB-RANDR "xcb-randr" REQUIRED)
find_library(LIBXCB-SHM   "xcb-shm"   REQUIRED)
find_library(LIBXCB-IMAGE "xcb-image" REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
	nlohmann_json::nlohmann_json
	fmt::fmt
    spdlog::spdlog
    SDBusCpp::sdbus-c++
    ${LIBDDCUTIL}
    ${LIBSYSTEMD}
    ${LIBXCB}
    ${LIBXCB-RANDR}
    ${LIBXCB-SHM}
    ${LIBXCB-IMAGE}
    -latomic
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gummyd")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    VERSION="${GUMMY_VERSION}"
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=0>
)

if(NOT UDEV_RULES_DIR)
    set(UDEV_RULES_DIR /usr/lib/udev/rules.d)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
install(FILES "${CMAKE_SOURCE_DIR}/gummyd/data/udev/99-gummy.rules" DESTINATION "${UDEV_RULES_DIR}")
