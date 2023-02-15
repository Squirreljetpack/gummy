project(gummy LANGUAGES CXX)

set(HEADERS
	${INCLUDE_DIR}/config.hpp
	${INCLUDE_DIR}/file.hpp
	${INCLUDE_DIR}/utils.hpp
)

set(SOURCES
	gummy.cpp
	../gummyd/config.cpp
)

add_library(CLI11 INTERFACE)
target_include_directories(CLI11 INTERFACE ${INCLUDE_DIR}/thirdparty/CLI)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE json CLI11)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})

add_compile_definitions(
	CMAKE_INSTALL_DAEMON_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/gummyd")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
