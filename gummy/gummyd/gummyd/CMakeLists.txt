add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
	PRIVATE
	gamma.cpp
	gamma.hpp
	display.cpp
	display.hpp
	sysfs.hpp
	sysfs.cpp
	sysfs_devices.cpp
	sysfs_devices.hpp
	core.cpp
	core.hpp
	config.cpp
	config.hpp
    utils.hpp
    utils.cpp
    sdbus-util.hpp
    sdbus-util.cpp
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
    xcb.hpp
    xcb.cpp
    ddc.hpp
    ddc.cpp
    gummyd.cpp
)

find_package(sdbus-c++ REQUIRED)
find_library(LIBDDCUTIL "ddcutil" REQUIRED)

# udev
find_library(LIBSYSTEMD "systemd" REQUIRED)

# XCB
find_library(LIBXCB "xcb" REQUIRED)
find_library(LIBXCB-RANDR "xcb-randr" REQUIRED)
find_library(LIBXCB-SHM "xcb-shm" REQUIRED)
find_library(LIBXCB-IMAGE "xcb-image" REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBXCB} ${LIBXCB-RANDR} ${LIBXCB-SHM} ${LIBXCB-IMAGE})

target_link_libraries(${PROJECT_NAME} PRIVATE
	nlohmann_json::nlohmann_json
	fmt::fmt
    spdlog::spdlog
    SDBusCpp::sdbus-c++
    ${LIBSYSTEMD}
    ${LIBDDCUTIL}
    -latomic
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gummy/gummyd/")
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
install(FILES "${CMAKE_SOURCE_DIR}/gummy/gummyd/data/udev/99-gummy.rules" DESTINATION "${UDEV_RULES_DIR}")
