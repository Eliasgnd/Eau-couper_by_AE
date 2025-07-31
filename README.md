# Eau-couper

Eau-couper is a Qt 6 application that drives a water cutting machine. It runs on Raspberry Pi and Windows and is built from the `machineDecoupeIHM.pro` project file.

## Features

- Draw or load shapes to create cutting paths
- Import edges of images using OpenCV
- Transfer images through Bluetooth or a local Wi‑Fi HTTP server
- Configure Wi‑Fi networks directly from the interface
- Control GPIO pins through libgpiod on Linux
- Optional AI‑generated icons (requires `OPENAI_API_KEY`)

## Build instructions

### Linux / Raspberry Pi

Install Qt 6, OpenCV and libgpiod development packages. On Debian/Ubuntu systems:

```bash
sudo apt install qt6-base-dev qt6-tools-dev qt6-tools-dev-tools libopencv-dev libgpiod-dev build-essential
```

Compile using qmake:

```bash
qmake machineDecoupeIHM.pro
make -j$(nproc)
```

The resulting binary `machineDecoupeIHM` can be run directly on the Raspberry Pi. GPIO access may require root or the `gpio` group.

### Windows

Install Qt 6 and OpenCV. The `.pro` file expects OpenCV to be located in `C:/opencv`. Open the project with Qt Creator and build in either Debug or Release mode.

## Running

Execute the built binary. On Raspberry Pi, the interface opens full screen and controls the connected hardware. On Windows, hardware functions depending on libgpiod are disabled, but you can design shapes and transfer images.
