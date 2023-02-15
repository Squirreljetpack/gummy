project(gummyd LANGUAGES CXX)

set(HEADERS
	${INCLUDE_DIR}/channel.hpp
	${INCLUDE_DIR}/config.hpp
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
	${INCLUDE_DIR}/xcb.hpp
)


set(SOURCES
	gummyd.cpp
	display.cpp
	gamma.cpp
	sysfs_devices.cpp
	core.cpp
	config.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})

find_package(sdbus-c++ REQUIRED)
find_library(UDEV_LIB "udev" REQUIRED)

find_library(XCB_LIB "xcb" REQUIRED)
find_library(XCB_RANDR_LIB "xcb-randr" REQUIRED)
find_library(XCB_SHM_LIB "xcb-shm" REQUIRED)
find_library(XCB_IMAGE_LIB "xcb-image" REQUIRED)

set(XCB_LIBS
	${XCB_LIB}
	${XCB_RANDR_LIB}
	${XCB_SHM_LIB}
	${XCB_IMAGE_LIB}
)

set(LIBS
	-latomic
	json
	SDBusCpp::sdbus-c++
	${XCB_LIBS}
	${UDEV_LIB}
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS})

if(NOT UDEV_DIR)
	set(UDEV_DIR /usr/lib/udev/rules.d)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
install(FILES ${CMAKE_SOURCE_DIR}/config/udev/99-gummy.rules DESTINATION "${UDEV_DIR}")
