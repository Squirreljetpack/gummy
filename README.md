# gummy

gummy is a screen manager for Linux. It was mainly written to allow adaptive screen adjustments, but allows manual configuration as well.

It provides a CLI for adjusting:

- backlight brightness*
- pixel brightness
- color temperature

These settings can be set manually or automatically, based on any of these inputs:
- **ALS**: your laptop's ambient light sensor, if available.
- **Screenlight**: screen lightness, i.e. the brightness of the contents being displayed on your screen.
- **Time**: a time range for the day (e.g. from 06:00 to 18:00).


**currently only for laptops. Backlight control for external monitors is WIP. You may want to use pixel brightness as a substitute.*
## Installation

DEB packages:
- provided in each new [release.](https://github.com/Fushko/gummy/releases)

AUR packages:
- [stable](https://aur.archlinux.org/packages/gummy/)
- [bleeding edge](https://aur.archlinux.org/packages/gummy-git/)

NixOS packages:
- [stable](https://search.nixos.org/packages?channel=unstable&show=gummy&query=gummy). 
- **Note**: You have to add the following line to `configuration.nix`: `services.udev.packages = [ pkgs.gummy ];`

On laptops, a reboot is needed after the installation to ensure udev rules for backlight adjustments are loaded.

## Usage

Type `gummy -h` to print all the available options.

Screen settings are applied to all screens, unless the `-s` option with a screen number (starting from 0) is provided.
All relevant settings can be increased or decreased by prefixing the value with a "+" or "-" sign.

Quick guide:
- `gummy start` starts the background process responsible for screen adjustments.
- `gummy -b 50` sets backlight brightness to 50%.
- `gummy -b +50` increases backlight brightness by 50%.
- `gummy -t 3400` sets the color temperature to 3400K.
- `gummy -B 1` adjusts backlight brightness based on screen lightness.
- `gummy -P 1` as above, but with pixel brightness.
- `gummy -T 1` as above, but with color temperature.
- `gummy -B 2` adjusts backlight brightness based on ALS data. (If you have an ambient light sensor.)
- `gummy --als-scale 1.5` multiplies the ALS signal by 1.5. This can be useful for calibration.
- `gummy -T 3 -y 06:00 -u 18:00 --temperature-max 6500 --temperature-min 3400` enables time-based temperature. Temperature will be at 6500K from 06:00, and transition to 3400K as the end̶ time̴s ap̶p̴r̵o̶a̷c̶h̷

## Building from source

System requirements:

- C++20 compiler
- CMake
- libxcb (with extensions: randr, shm, image)
- libsystemd
- libsdbus-c++

### Apt packages

`sudo apt install build-essential cmake libxcb1-dev libxcb-randr0-dev libxcb-shm0-dev libxcb-image0-dev libsystemd-dev libsdbus-c++-dev`

### Installation

```
git clone https://github.com/Fushko/gummy.git --depth 1 && cd gummy
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE="Release"
cmake --build . && sudo cmake --install .
```

## Credits
This project makes (hopefully good) use of these excellent libraries:

- JSON [[MIT](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)]
- fmt [[MIT](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)]
- spdlog [[MIT](https://github.com/gabime/spdlog/blob/v1.x/LICENSE)]
- CLI11 [[BSD-3-clause](https://github.com/CLIUtils/CLI11/blob/main/LICENSE)]
- libxcb [[MIT](https://github.com/freedesktop/xcb-libxcb/blob/master/COPYING)]
- libsystemd [[GPLv2](https://github.com/systemd/systemd/blob/main/LICENSE.GPL2)]

Temperature adjustments use the color ramp provided by [Ingo Thies](https://github.com/jonls/redshift/blob/master/README-colorramp) for [Redshift.](https://github.com/jonls/redshift)

## Donations

You can buy me a coffee at: https://coindrop.to/fushko

## License

Copyright (C) 2021-2023, Francesco Fusco.
Released under the terms of the GNU General Public License v3.0.
