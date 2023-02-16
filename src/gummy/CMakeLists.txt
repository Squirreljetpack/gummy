project(gummy LANGUAGES CXX)

add_library(CLI11 INTERFACE)
target_include_directories(CLI11 INTERFACE ${INCLUDE_DIR}/thirdparty/CLI)

set(HEADERS
	${INCLUDE_DIR}/config.hpp
	${INCLUDE_DIR}/file.hpp
	${INCLUDE_DIR}/utils.hpp
)

set(SOURCES
	gummy.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE
	nlohmann_json::nlohmann_json
	CLI11
	config)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(${PROJECT_NAME} PRIVATE CMAKE_INSTALL_DAEMON_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/gummyd")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
