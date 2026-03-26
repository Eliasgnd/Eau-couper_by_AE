# External Integrations

**Analysis Date:** 2026-03-26

## APIs & External Services

**OpenAI DALL-E 3 Image Generation:**
- Service: OpenAI API (https://api.openai.com/v1/images/generations)
- What it's used for: Generate vector-style images from user text prompts for water-cutting designs
- SDK/Client: Qt6 QNetworkAccessManager (built-in HTTP client)
- Auth: Bearer token via `OPENAI_API_KEY` environment variable
  - Location: `infrastructure/network/OpenAIService.cpp` lines 56-62
  - Required: Yes (application prompts user if missing)
- Endpoints:
  - `POST https://api.openai.com/v1/images/generations` - Image generation with DALL-E 3
- Models supported:
  - dall-e-3 (default)
  - Configurable via UI dialog parameters
- Image parameters:
  - size: 1024x1024 (default, configurable)
  - quality: standard (default, configurable)
  - count: 1 per request
- Response handling:
  - Generated image downloaded and saved to local filesystem
  - Saved to: `ImagePaths::iaDir()` as PNG with timestamp naming
  - Signal on completion: `OpenAIService::generationFinished(bool success, QString filePath)`

## Data Storage

**Local File System (Primary):**
- Custom shapes storage:
  - File: Determined by `InventoryRepository::customShapesFilePath()`
  - Function in: `infrastructure/persistence/InventoryRepository.cpp`
  - Contains: Custom shape definitions, layouts, folders

- Inventory data (JSON or structured format):
  - Base shapes metadata (usage count, last used date, order)
  - Custom shapes with geometry
  - Layout definitions
  - Folder structure
  - Loaded/saved via: `InventoryStorage` namespace (`application/InventoryStorage.h`)

- Generated images:
  - Location: `ImagePaths::iaDir()` - Determined at runtime
  - Format: PNG files with timestamp naming pattern `YYYY-MM-DD_HH-mm_sanitized-name.png`
  - Purpose: AI-generated images available for import into designs

- Project files and resources:
  - Location: `resources.qrc` - Bundled Qt resource file
  - Contents: Icons, images, stylesheets used by UI

**No database system is integrated:**
- All persistence is file-based (JSON, PNG, custom binary formats)
- No SQL database (SQLite, PostgreSQL, MySQL)
- No cloud storage backends (AWS S3, Google Drive, OneDrive)

## Authentication & Identity

**Internal Authentication:**
- None detected - Application appears to run without user accounts or login
- No OAuth, JWT, or standard auth framework

**Hardware-Level Access Control:**
- GPIO pin access on Raspberry Pi
  - Implemented via libgpiod library
  - Requires appropriate `/dev/gpiochip*` permissions
  - No user-level authentication (system-level only)

## Hardware & Physical Integrations

**Raspberry Pi GPIO Hardware Interface:**
- Library: libgpiod (via `infrastructure/hardware/Raspberry.h`)
- Purpose: Control stepper motors and water-cutting jet
- Pin configuration in `infrastructure/hardware/Raspberry.h`:
  - **Motor Control Pins:**
    - STEP_PIN (13) - Step pulse for stepper motor
    - DIR_PIN (6) - Direction control
    - EN1_PIN, EN2_PIN, EN3_PIN (7, 8, 25) - Enable/disable motor channels
  - **Jet Control:**
    - BIN1_PIN, BIN2_PIN (17, 27) - Jet bidirectional control
    - BEMF_PIN (18) - Back-EMF sensing
  - **System Control:**
    - PIN_SCS (20) - Chip Select for SPI
    - PIN_RESET (21) - System reset
    - PIN_SLEEPn (26) - Sleep mode control
  - **Feedback Pins:**
    - STALLN_PIN (23) - Stall detection
    - FAULTN_PIN (24) - Fault detection
  - **SPI Hardware Pins (auto-managed):**
    - SDATI_PIN (10) - MOSI (Master Out, Slave In)
    - SDATAO_PIN (9) - MISO (Master In, Slave Out)
    - SCLK_PIN (11) - SPI Clock

- Motor control via: `infrastructure/hardware/MotorControl.cpp`
  - Jet control: `startJet()`, `stopJet()` methods
  - Movement: `moveCut()` (with jet on), `moveRapid()` (rapid positioning)
  - Speeds: Vcut (10 mm/s default), Vtrav (200 mm/s default)
  - Step tracking: X/Y step counters

**Touchscreen Input:**
- Framework: Qt6 touch event handling via MouseInteractionHandler
- Gesture support: Via GestureHandler and TouchGestureReader in `drawing/tools/TouchGestureReader.h`
- Virtual keyboards: KeyboardDialog, NumericKeyboardDialog for touchscreen text input

**Bluetooth Connectivity:**
- Framework: Qt6 Bluetooth module (QT += bluetooth)
- Purpose: Wireless file transfer and device discovery
- Implementation: `ui/dialogs/BluetoothReceiverDialog.h` and `.cpp`
- Features:
  - Service initialization via OS Bluetooth daemon
  - File reception monitoring
  - Status updates with flashing indicators
  - Process-based integration (QProcess for bluetooth management)

## Network & Connectivity

**WiFi Management:**
- Framework: nmcli command-line interface wrapper
- Implementation: `infrastructure/network/WifiNmcliClient.h`, `WifiNmcliClient.cpp`
- Features:
  - Network scanning via `nmcli dev wifi list`
  - Connection management via NetworkManager
  - WiFi device detection
  - WiFi radio on/off control
  - Network profile management (save/forget networks)
- Configuration UI: `ui/dialogs/WifiConfigDialog.h`
  - SSID selection and hidden network support
  - Credential entry (WPA2/WPA3)
  - Connection status monitoring
  - Auto-scan with configurable intervals
  - Diagnostic information display

**HTTP File Transfer:**
- Framework: Qt6 HttpServer module (QT += httpserver)
- Purpose: Local network file transfer from PC to Raspberry Pi
- Widget: `ui/widgets/WifiTransferWidget.h`
- Uses: Built-in HTTP server to receive files via browser

**Network Stack:**
- Qt6 Network module handles all socket and HTTP operations
- No custom networking implementation

## Monitoring & Observability

**Error Tracking:**
- No external error tracking service (Sentry, Bugsnag, etc.)
- Errors communicated via:
  - UI status messages and dialogs
  - Qt debug output via qDebug(), qWarning()
  - Signal/slot based error reporting

**Logging:**
- Console logging via Qt's qDebug(), qWarning(), qCritical()
- No file-based logging system detected
- Status messages emitted via signals (e.g., `OpenAIService::statusUpdate(QString)`)

## Image Processing & Vision

**OpenCV 4 Integration:**
- Library: libopencv-dev (system library via pkg-config)
- Purpose: Logo importing, image edge detection, shape skeletonization
- Components:
  - `infrastructure/imaging/LogoImporter.h` - Import external images as shapes
  - `infrastructure/imaging/ImageEdgeImporter.h` - Extract edges from photos
  - `infrastructure/imaging/skeletonizer.h` - Convert binary images to vector paths
- Key algorithms:
  - Zhang-Suen skeletonization for thinning binary images to 1-pixel width
  - Path smoothing via Chaikin algorithm
  - Bitmap to QPainterPath vector conversion

**QR Code Generation:**
- Library: Custom qrcodegen (included in codebase)
- Files: `drawing/tools/qrcodegen.hpp`, `drawing/tools/qrcodegen.cpp`
- Purpose: Generate QR codes for machine configuration or data export

## External Configuration & Profiles

**WiFi Profiles:**
- Storage: NetworkManager config files (via `WifiProfileService`)
- Persistence: System-level WiFi network profiles
- Access method: nmcli command interface

**Language/Localization:**
- Files:
  - `translations/machineDecoupeIHM_fr_FR.ts` - French UI translation
  - `translations/machineDecoupeIHM_en_US.ts` - English UI translation
- Framework: Qt Linguist (lupdate/lrelease tools)
- Runtime selection via `shared/Language.h` enum

## Webhooks & Callbacks

**Incoming:**
- None detected - Application is not exposed as a service to external webhooks

**Outgoing:**
- OpenAI API calls only (request/response model, not webhook-based)
- Bluetooth file reception monitoring (local only)
- No external callbacks or event notifications to remote services

## API Contracts & Communication

**OpenAI Request Format:**
```json
{
  "model": "dall-e-3",
  "prompt": "A minimal flat icon of a [user-provided-object]...",
  "n": 1,
  "size": "1024x1024",
  "quality": "standard"
}
```
- Prompt auto-generated based on color mode (flat icon vs outline drawing)
- Implementation: `infrastructure/network/OpenAIService.cpp` lines 64-70

**OpenAI Response Format:**
```json
{
  "data": [
    {
      "url": "https://..."
    }
  ]
}
```
- Image URL extracted and downloaded
- Saved to filesystem with sanitized naming

---

*Integration audit: 2026-03-26*
