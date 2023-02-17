project(gummy LANGUAGES CXX)

set(HEADERS
	${INCLUDE_DIR}/config.hpp
	${INCLUDE_DIR}/file.hpp
	${INCLUDE_DIR}/utils.hpp
)

set(SOURCES
	gummy.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

FetchContent_Declare(cli11
	GIT_REPOSITORY https://github.com/CLIUtils/CLI11
	GIT_TAG 291c58789c031208f08f4f261a858b5b7083e8e2) #v2.3.2
FetchContent_MakeAvailable(cli11)

target_link_libraries(${PROJECT_NAME} PRIVATE
	nlohmann_json::nlohmann_json
	CLI11::CLI11
	fmt::fmt
	config)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	VERSION="${MAIN_VERSION}"
	CMAKE_INSTALL_DAEMON_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/gummyd"
	$<$<CONFIG:Debug>:CMAKE_DEBUG>
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
