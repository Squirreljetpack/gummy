project(gummyd LANGUAGES CXX)

find_package(sdbus-c++ REQUIRED)
find_library(LIBUDEV "udev" REQUIRED)

# file
add_library(file INTERFACE gummyd/file.hpp)

# XCB
find_library(LIBXCB "xcb" REQUIRED)
find_library(LIBXCB-RANDR "xcb-randr" REQUIRED)
find_library(LIBXCB-SHM "xcb-shm" REQUIRED)
find_library(LIBXCB-IMAGE "xcb-image" REQUIRED)
add_library(xcb INTERFACE gummyd/xcb.hpp)
target_link_libraries(xcb INTERFACE ${LIBXCB} ${LIBXCB-RANDR} ${LIBXCB-SHM} ${LIBXCB-IMAGE})

# channel
add_library(channel INTERFACE gummyd/channel.hpp)
target_compile_features(channel INTERFACE cxx_std_20)
target_link_libraries(channel INTERFACE -latomic)

# time
add_library(time INTERFACE gummyd/time.hpp)

# sdbus
find_package(sdbus-c++ REQUIRED)
add_library(sdbus-util INTERFACE gummyd/sdbus-util.hpp)
target_link_libraries(sdbus-util INTERFACE SDBusCpp::sdbus-c++)

# easing
add_library(easing INTERFACE gummyd/easing.hpp)

# udev
find_library(LIBUDEV "udev" REQUIRED)
add_library(udev INTERFACE gummyd/udev.hpp)
target_link_libraries(udev INTERFACE ${LIBUDEV})
if(NOT UDEV_RULES_DIR)
    set(UDEV_RULES_DIR /usr/lib/udev/rules.d)
endif()
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/udev/99-gummy.rules" DESTINATION "${UDEV_RULES_DIR}")

# API
add_library(libgummyd STATIC gummyd/api.cpp gummyd/api.hpp)
set_target_properties(libgummyd PROPERTIES PREFIX "")
target_include_directories(libgummyd PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/gummyd")
target_link_libraries(libgummyd PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    file
)
target_compile_definitions(libgummyd PRIVATE CMAKE_INSTALL_DAEMON_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/gummyd")

add_subdirectory(gummyd)
