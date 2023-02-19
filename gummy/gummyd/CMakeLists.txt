project(gummyd LANGUAGES CXX)

find_package(sdbus-c++ REQUIRED)
find_library(LIBUDEV "udev" REQUIRED)

# file
add_library(file INTERFACE file.hpp)

# XCB
find_library(LIBXCB "xcb" REQUIRED)
find_library(LIBXCB-RANDR "xcb-randr" REQUIRED)
find_library(LIBXCB-SHM "xcb-shm" REQUIRED)
find_library(LIBXCB-IMAGE "xcb-image" REQUIRED)
add_library(xcb INTERFACE xcb.hpp)
target_link_libraries(xcb INTERFACE ${LIBXCB} ${LIBXCB-RANDR} ${LIBXCB-SHM} ${LIBXCB-IMAGE})

# channel
add_library(channel INTERFACE channel.hpp)
target_compile_features(channel INTERFACE cxx_std_20)
target_link_libraries(channel INTERFACE -latomic)

# time
add_library(time INTERFACE time.hpp)

# sdbus
find_package(sdbus-c++ REQUIRED)
add_library(sdbus-util INTERFACE sdbus-util.hpp)
target_link_libraries(sdbus-util INTERFACE SDBusCpp::sdbus-c++)

# easing
add_library(easing INTERFACE easing.hpp)

# udev
find_library(LIBUDEV "udev" REQUIRED)
add_library(udev INTERFACE udev.hpp)
target_link_libraries(udev INTERFACE ${LIBUDEV})
set(GUMMYD_UDEV_RULE "${CMAKE_CURRENT_SOURCE_DIR}/data/udev/99-gummy.rules")
if(NOT UDEV_RULES_DIR)
    set(UDEV_RULES_DIR /usr/lib/udev/rules.d)
endif()
install(FILES ${GUMMYD_UDEV_RULE} DESTINATION "${UDEV_RULES_DIR}")

add_subdirectory(gummyd)
