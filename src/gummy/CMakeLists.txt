project(gummy LANGUAGES CXX)

set(SOURCES gummy.cpp ../gummyd/cfg.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include/third-party)

add_compile_definitions(
	CMAKE_INSTALL_DAEMON_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/gummyd")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
