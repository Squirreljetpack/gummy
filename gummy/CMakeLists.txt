include(FetchContent)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_Declare(fmt
    URL https://github.com/fmtlib/fmt/releases/download/9.1.0/fmt-9.1.0.zip
)
FetchContent_Declare(spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.11.0.tar.gz
)
FetchContent_MakeAvailable(
    json
    fmt
    spdlog
)

add_subdirectory(gummyd)
add_subdirectory(gummy-cli)
