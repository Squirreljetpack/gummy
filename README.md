# gummy

## Description

Simple screen manager daemon for Linux. It comes with a straightforward CLI for tweaking:
- backlight brightness
- pixel brightness
- color temperature

Automatic adjustments are also available, based on any of these inputs:

- ALS: your laptop's ambient light sensor, if available.
- Screenlight: screen lightness, i.e. the brightness of the contents being displayed on your screen.
- Time: a time range for the day (e.g. from 06:00 to 18:00).

## How it works

It mostly strings together ddcutil (external backlights), xrandr (brightness, temperature, screen contents) and udev (internal backlights, sensors), in a simple, unified interface.

## Building from source

CMake 3.14+ and a compiler with support for the C++20 revision are required. Building has been tested on GCC and Clang so far.

The following C++ libraries are externally linked by default. You may also fetch them from GitHub with some CMake options. For example: `-DGUMMY_EXTERNAL_FMT=OFF`. Check the CMakeLists.txt files for details.

- libfmt
- libspdlog
- libcli11
- libsdbus-c++
- nlohmann_json

The following C libraries are RAII-wrapped and utilized in this project. They are widely available on most environments. No configuration for internet downloads is provided for these.

- libddcutil
- libsystemd
- libxcb
- libxcb-randr
- libxcb-shm
- libxcb-image

Below are some package manager one-liners for your convenience. These may likely work on derivatives and future versions.

**Ubuntu 22.04**:

```
sudo apt install build-essential \
cmake \
libatomic1 \
libxcb1-dev \
libxcb-randr0-dev \
libxcb-shm0-dev \
libxcb-image0-dev \
libsystemd-dev \
libddcutil-dev \
libsdbus-c++-dev \
libcli11-dev \
libfmt-dev \
libspdlog-dev \
nlohmann-json3-dev
```

**Fedora 39**: 
```
sudo dnf install cmake \
libddcutil-devel.x86_64 \
systemd-devel.x86_64 \
libxcb-devel.x86_64 \
xcb-util-image-devel.x86_64 \
sdbus-cpp-devel.x86_64 \
fmt-devel.x86_64 \
spdlog-devel.x86_64 \
cli11-devel.noarch
```

On Fedora 39, there is no package for the json library. Add `-DGUMMY_EXTERNAL_JSON=0` to the CMake configuration to fetch it from GitHub.

### Installation

```
git clone https://github.com/f-fusco/gummy.git && cd gummy
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE="Release"
cmake --build . -j && sudo cmake --install .
```

## Packages

| Distro | Location |
|---------|--------|
| Ubuntu | [GitHub releases](https://github.com/Fushko/gummy/releases/latest) |
| Arch    | AUR ([stable](https://aur.archlinux.org/packages/gummy/) - [latest](https://aur.archlinux.org/packages/gummy-git/)) |
| NixOS   | [nixpkgs](https://search.nixos.org/packages?channel=unstable&show=gummy&query=gummy) |

The DEB package is provided by me for convenience. It's built on Ubuntu 22.04 and will not work on all Debian-based distros. Maintainers welcome!

**NixOS users**: you must add the following line to `configuration.nix`: `services.udev.packages = [ pkgs.gummy ];`

## Usage

Type `gummy -h` to print all the available options.

Screen settings are applied to all screens, unless the `-s` option with a screen number starting from 0 is provided.
All relevant settings can be increased or decreased by prefixing the value with a plus (+) or minus (-) sign.

Here are some common commands:

| Command | Explanation |
|---------|--------|
| `gummy start`   | Starts the background process responsible for screen adjustments. |
| `gummy status`   | Lists current screen settings. |
| `gummy -b 50`   | Sets backlight brightness to 50%. |
| `gummy -b +50`  | Increases backlight brightness by 50%. |
| `gummy -t 3400` | Sets the color temperature to 3400K. |
| `gummy -B 1`    | Adjusts backlight brightness based on screen lightness. |
| `gummy -P 1`    | As above, but with pixel brightness. |
| `gummy -T 1`    | As above, but with color temperature. |
| `gummy -B 2`    | Adjusts backlight brightness based on ALS data. (If you have an ambient light sensor.) |
| `gummy --als-scale 1.5` | Multiplies the ALS signal by 1.5. This can be useful for calibration. |
| `gummy -T 3 -y 06:00 -u 18:00` | Enables time-based temperature. Temperature will gradually transition from 6500K to 3200K (by default), one hour before 18:00. |

## Troubleshooting

First, reboot after installing to ensure gummy can operate on your screens without root privileges.

The kernel module **i2c-dev** is required for managing external backlights. i2c-dev is generally built-in in most distributions. This [guide from ddcutil's documentation](https://www.ddcutil.com/kernel_module/) explains how to see if it's built-in, and load it if it's not.

If your `ddcutil --version` is older than [1.4](https://www.ddcutil.com/release_notes/#i2c-device-permissions-simplified), you must add your user to the `i2c` group to manage backlights without root privileges:

`sudo usermod <user-name> -aG i2c`

If backlight adjustments still don't work, make sure that DDC/CI is enabled in your screen's control panel. Your screen might not support DDC/CI: in that case, you might want to use pixel brightness (gamma) as a substitute.

Gamma is a global resource and may be abused by many programs at once. You can disable gamma functionality with `gummy --gamma-enable 0` to use gummy's backlight control alongside other gamma-adjusting software, e.g. Redshift, Night Light, etc. 

Of course, if your monitors don't support DDC/CI, and you disable gamma, gummy will just sit there among other processes, wasting precious megabytes of RAM.

### Wayland
gummy runs on Wayland as of version 0.5.2. However, the following features are currently unavailable:

- pixel brightness
- color temperature
- screenlight

Unfortunately, there is no compositor-agnostic way of doing those things right now (as far as I know). For color temperature, you may use built-in programs that come with some desktop environments.

## Credits
This project makes (hopefully good) use of these libraries:

- JSON [[MIT](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)]
- fmt [[MIT](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)]
- spdlog [[MIT](https://github.com/gabime/spdlog/blob/v1.x/LICENSE)]
- CLI11 [[BSD-3-clause](https://github.com/CLIUtils/CLI11/blob/main/LICENSE)]
- libxcb [[MIT](https://github.com/freedesktop/xcb-libxcb/blob/master/COPYING)]
- libsystemd [[GPLv2](https://github.com/systemd/systemd/blob/main/LICENSE.GPL2)]
- ddcutil [[GPLv2](https://github.com/rockowitz/ddcutil/blob/1.4.1-release/COPYING)]
- sdbus-c++ [[GPLv2](https://github.com/Kistler-Group/sdbus-cpp/blob/master/COPYING)]

Temperature adjustments use the color ramp provided by [Ingo Thies](https://github.com/jonls/redshift/blob/master/README-colorramp) for [Redshift.](https://github.com/jonls/redshift)

## Donations

You can buy me a coffee at: https://coindrop.to/fusco

## License

Copyright 2021-2024 Francesco Fusco

Released under the terms of the GNU General Public License v3.0.
