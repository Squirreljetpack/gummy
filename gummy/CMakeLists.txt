include(FetchContent)
include(CPack)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_Declare(fmt  URL https://github.com/fmtlib/fmt/releases/download/9.1.0/fmt-9.1.0.zip)

FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(fmt)

add_subdirectory(gummyd)
add_subdirectory(gummy-cli)
