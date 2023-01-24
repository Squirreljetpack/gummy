project(gummyd LANGUAGES CXX)

set(SOURCES
	gummyd.cpp
	xorg.cpp
	cfg.cpp
	gamma.cpp
	sysfs_devices.cpp
	screenctl.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

#find_package(X11 REQUIRED)
find_package(Threads REQUIRED)
find_package(sdbus-c++ REQUIRED)
find_library(XCB_LIB "xcb" REQUIRED)
find_library(XCB_IMAGE_LIB "xcb-image" REQUIRED)
find_library(XCB_RANDR_LIB "xcb-randr" REQUIRED)
find_library(XCB_SHM_LIB "xcb-shm" REQUIRED)
find_library(UDEV_LIB "udev" REQUIRED)

target_link_libraries(
	${PROJECT_NAME} PRIVATE
	#${X11_LIBRARIES}
	Threads::Threads
	SDBusCpp::sdbus-c++
	${XCB_LIB}
	${XCB_RANDR_LIB}
	${XCB_SHM_LIB}
	${XCB_IMAGE_LIB}
	${UDEV_LIB}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include/third-party)

if(NOT UDEV_DIR)
	set(UDEV_DIR /usr/lib/udev/rules.d)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
install(FILES ${CMAKE_SOURCE_DIR}/config/udev/99-gummy.rules DESTINATION "${UDEV_DIR}")
