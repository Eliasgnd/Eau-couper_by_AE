# Eau-Couper by AE

Eau-Couper is a Qt based graphical interface for a small water‑jet cutting machine. It lets you draw or import shapes, plan the cutting path and drive the motors. The application also offers Wi‑Fi and Bluetooth file transfers, AI generated images, an inventory of saved layouts and simple controls to start or pause a cut.

## Building

The project uses a `qmake` build file (`machineDecoupeIHM.pro`). Install Qt 6 and the required libraries before building:

- Qt 6 (Core, GUI, Widgets, Network, SVG, Bluetooth, HttpServer, OpenGL widgets)
- OpenCV 4
- `libgpiod` (Linux only)

Then run the following commands in the repository root:

```bash
qmake machineDecoupeIHM.pro
make
```

The resulting executable is `machineDecoupeIHM`.

## Basic usage

Launch the application:

```bash
./machineDecoupeIHM
```

The main window allows you to:

1. Create or import shapes in the preview area.
2. Manage saved layouts through the **Inventaire** page.
3. Transfer images over Wi‑Fi or Bluetooth.
4. Start, pause or stop the cutting process from the control panel.

Use the on‑screen interface or a keyboard/touch screen to operate the machine. The menu also lets you switch language and adjust settings.

