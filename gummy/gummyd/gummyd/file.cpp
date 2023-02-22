#include <gummyd/file.hpp>

#include <fstream>
#include <filesystem>
#include <sstream>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

namespace gummyd {

named_pipe::named_pipe(std::string filepath) : filepath_(filepath) {
    fd_ = mkfifo(filepath.c_str(), S_IFIFO | 0640);
    if (fd_ < 0) {
        spdlog::error("[named_pipe] mkfifo error");
    }
}

named_pipe::~named_pipe() {
    close(fd_);
    std::filesystem::remove(filepath_);
}

lock_file::lock_file(std::string filepath)
    : filepath_(filepath),
      fd_(open(filepath.c_str(), O_WRONLY | O_CREAT, 0640)) {

    if (fd_ < 0) {
        throw std::runtime_error("open() failed");
    }

    fl_.l_type   = F_WRLCK;
    fl_.l_whence = SEEK_SET;
    fl_.l_start  = 0;
    fl_.l_len    = 1;
    fnctl_op_    = fcntl(fd_, F_SETLK, &fl_);

    if (fnctl_op_ < 0) {
        throw std::runtime_error("F_SETLK failed");
    }
}

lock_file::~lock_file() {
    if (fnctl_op_ > 0)
        fcntl(fd_, F_UNLCK, &fl_);
    close(fd_);
    std::filesystem::remove(filepath_);
}


std::string file_read(std::string filepath) {
    std::ifstream fs(filepath);
    fs.exceptions(std::ifstream::failbit);

    std::ostringstream buf;
    buf << fs.rdbuf();

    return buf.str();
}

void file_write(std::string filepath, const std::string &data) {
    std::ofstream fs(filepath);
    fs.exceptions(std::ifstream::failbit);
    fs.write(data.c_str(), data.size());
}

std::string xdg_config_filepath(std::string filename) {
    const char *home = getenv("XDG_CONFIG_HOME");
    std::string format = "/";

    if (!home) {
        home   = getenv("HOME");
        format = "/.config/";
    }

    std::ostringstream ss;
    ss << home << format << filename;

    return ss.str();
}

std::string xdg_state_filepath(std::string filename) {

    constexpr std::array<std::array<std::string_view, 2>, 2> env_vars {{
        {"XDG_STATE_HOME", ""},
        {"HOME", "/.local/state"}
    }};

    for (const auto &var : env_vars) {
        if (auto env = getenv(var[0].data()))
            return fmt::format("{}{}/{}", env, var[1], filename);
    }

    throw std::runtime_error("HOME env not found!");
}

} // namespace gummyd;
