add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
	PRIVATE
	gamma.cpp
	gamma.hpp
	display.cpp
	display.hpp
	sysfs_devices.cpp
	sysfs_devices.hpp
	core.cpp
	core.hpp
	config.cpp
	config.hpp
    constants.hpp
	gummyd.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	nlohmann_json::nlohmann_json
	fmt::fmt
	sdbus-util
	file
	xcb
	udev
	time
	channel
	easing
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gummy/")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/gummy/gummyd")
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(${PROJECT_NAME} PRIVATE
	VERSION="${MAIN_VERSION}"
	$<$<CONFIG:Debug>:CMAKE_DEBUG>
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
