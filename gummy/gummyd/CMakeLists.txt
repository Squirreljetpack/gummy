project(gummyd LANGUAGES CXX)

# API
add_library(libgummyd STATIC gummyd/api.cpp gummyd/file.cpp gummyd/constants.cpp gummyd/utils.cpp)
set_target_properties(libgummyd PROPERTIES PREFIX "")
target_include_directories(libgummyd PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(libgummyd PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)
target_compile_definitions(libgummyd PRIVATE CMAKE_INSTALL_DAEMON_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}/gummyd")

add_subdirectory(gummyd)
