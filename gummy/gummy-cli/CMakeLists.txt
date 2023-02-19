project(gummy-cli LANGUAGES CXX)

FetchContent_Declare(cli11
	GIT_REPOSITORY https://github.com/CLIUtils/CLI11
	GIT_TAG 291c58789c031208f08f4f261a858b5b7083e8e2) #v2.3.2
FetchContent_MakeAvailable(cli11)

add_subdirectory(gummy-cli)
