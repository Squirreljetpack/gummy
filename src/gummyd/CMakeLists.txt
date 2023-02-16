project(gummyd LANGUAGES CXX)

set(HEADERS
	${INCLUDE_DIR}/core.hpp
	${INCLUDE_DIR}/dbus.hpp
	${INCLUDE_DIR}/display.hpp
	${INCLUDE_DIR}/easing.hpp
	${INCLUDE_DIR}/file.hpp
	${INCLUDE_DIR}/gamma.hpp
	${INCLUDE_DIR}/sysfs_devices.hpp
	${INCLUDE_DIR}/sysfs.hpp
	${INCLUDE_DIR}/time.hpp
	${INCLUDE_DIR}/utils.hpp
)

set(SOURCES
	gummyd.cpp
	display.cpp
	gamma.cpp
	sysfs_devices.cpp
	core.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

find_package(sdbus-c++ REQUIRED)
find_library(LIBUDEV "udev" REQUIRED)

find_library(LIBXCB "xcb" REQUIRED)
find_library(LIBXCB-RANDR "xcb-randr" REQUIRED)
find_library(LIBXCB-SHM "xcb-shm" REQUIRED)
find_library(LIBXCB-IMAGE "xcb-image" REQUIRED)

add_library(xcb INTERFACE ${INCLUDE_DIR}/xcb.hpp)
target_link_libraries(xcb INTERFACE ${LIBXCB} ${LIBXCB-RANDR} ${LIBXCB-SHM} ${LIBXCB-IMAGE})

add_library(channel INTERFACE ${INCLUDE_DIR}/channel.hpp)
target_link_libraries(channel INTERFACE -latomic)

set(LIBS
	nlohmann_json::nlohmann_json
	SDBusCpp::sdbus-c++
	config
	xcb
	channel
	${LIBUDEV}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(${PROJECT_NAME} PRIVATE CMAKE_DEBUG)
endif()

if(NOT UDEV_DIR)
	set(UDEV_DIR /usr/lib/udev/rules.d)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
install(FILES ${CMAKE_SOURCE_DIR}/config/udev/99-gummy.rules DESTINATION "${UDEV_DIR}")
