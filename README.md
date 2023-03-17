# gummy

gummy is a screen manager for Linux. It was mainly written to allow adaptive screen adjustments, but allows manual configuration as well.

It provides a CLI for adjusting:

- backlight brightness
- pixel brightness
- color temperature

These settings can be set manually or automatically, based on any of these inputs:
- **ALS**: your laptop's ambient light sensor, if available.
- **Screenlight**: screen lightness, i.e. the brightness of the contents being displayed on your screen.
- **Time**: a time range for the day (e.g. from 06:00 to 18:00).

## Installation

| Distro | Location |
|---------|--------|
| Debian/Ubuntu  | [GitHub releases](https://github.com/Fushko/gummy/releases/latest) |
| Arch    | AUR ([stable](https://aur.archlinux.org/packages/gummy/) - [latest](https://aur.archlinux.org/packages/gummy-git/)) |
| NixOS   | [nixpkgs](https://search.nixos.org/packages?channel=unstable&show=gummy&query=gummy) |

For NixOS users: you must add the following line to `configuration.nix`: `services.udev.packages = [ pkgs.gummy ];`

gummy uses the ddcutil library on your system for managing external backlights. Until ddcutil [1.4+](https://www.ddcutil.com/release_notes/#i2c-device-permissions-simplified), you must add your user to the "i2c" group for rootless operation. From ddcutil's [documentation](https://www.ddcutil.com/config_steps/#i2c-device-permissions):

`sudo usermod <user-name> -aG i2c`

Generally, a **reboot is recommended** to ensure gummy can operate your screens without elevated privileges.

## Usage

Type `gummy -h` to print all the available options.

Screen settings are applied to all screens, unless the `-s` option with a screen number (starting from 0) is provided.
All relevant settings can be increased or decreased by prefixing the value with a "+" or "-" sign.

Quick guide:
| Command | Explanation |
|---------|--------|
| `gummy start`   | Starts the background process responsible for screen adjustments. |
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

If you experience screen flickering: disable any program that might change screen gamma, such as Redshift. Some system daemons might also cause screen flickering, such as colord. Check if it's running in your system, and try disabling it. I may add functionality to disable gamma control in the future (while keeping backlight control), so that gummy can work with any other gamma-adjusting software.

If backlight adjustments don't work, make sure DDC/CI is enabled in your screen's control panel. Your screen might not support DDC/CI: in that case, you might want to use pixel brightness as a substitute.



## Building from source

System requirements:

- C++20 compiler
- CMake v3.14 or later
- libxcb (with extensions: randr, shm, image)
- libsystemd
- libsdbus-c++
- libddcutil

### Apt packages

`sudo apt install build-essential cmake libxcb1-dev libxcb-randr0-dev libxcb-shm0-dev libxcb-image0-dev libsystemd-dev libsdbus-c++-dev libddcutil-dev`

### Installation

```
git clone https://github.com/Fushko/gummy.git --depth 1 && cd gummy
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE="Release"
cmake --build . -j && sudo cmake --install .
```

## Credits
This project makes (hopefully good) use of these excellent libraries:

- JSON [[MIT](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)]
- fmt [[MIT](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)]
- spdlog [[MIT](https://github.com/gabime/spdlog/blob/v1.x/LICENSE)]
- CLI11 [[BSD-3-clause](https://github.com/CLIUtils/CLI11/blob/main/LICENSE)]
- libxcb [[MIT](https://github.com/freedesktop/xcb-libxcb/blob/master/COPYING)]
- libsystemd [[GPLv2](https://github.com/systemd/systemd/blob/main/LICENSE.GPL2)]
- libddcutil [[GPLv2](https://github.com/rockowitz/ddcutil/blob/1.4.1-release/COPYING)]

Temperature adjustments use the color ramp provided by [Ingo Thies](https://github.com/jonls/redshift/blob/master/README-colorramp) for [Redshift.](https://github.com/jonls/redshift)

## Donations

You can buy me a coffee at: https://coindrop.to/fushko

## License

Copyright (C) 2021-2023, Francesco Fusco.

Released under the terms of the GNU General Public License v3.0.
