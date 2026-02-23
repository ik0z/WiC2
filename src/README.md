# ESP32 Captive Portal Firmware

ESP32 Arduino firmware that creates a Wi-Fi access point with automatic captive portal detection and an admin dashboard for device management.

## Features

- **Smart Device Detection** — Identifies Windows, iOS, Android, macOS, and Linux clients
- **Captive Portal** — Auto-popup on all major operating systems
- **Admin Dashboard** — Password-protected management UI with device logs, credential storage, and client monitoring
- **SPIFFS Storage** — Persistent JSON storage for devices, credentials, and settings
- **DNS Redirection** — All DNS queries resolve to the AP for captive portal triggering
- **Wi-Fi Scanning** — Scan and display nearby networks from the admin panel
- **Agent Distribution** — Serve the Windows agent binary for download

## Files

| File | Description |
|------|-------------|
| `CaptivePortal.ino` | Main sketch — device detection, web server routes, captive portal logic |
| `config.h` | All configuration constants (AP settings, admin password, file paths) |
| `storage.h` | SPIFFS initialization and JSON read/write helpers |
| `pages.h` | PROGMEM HTML/CSS for captive portal pages (per-device landing pages) |
| `admin_page.h` | PROGMEM HTML/CSS/JS for the full admin dashboard |

## Dependencies

Install via Arduino Library Manager:

| Library | Author |
|---------|--------|
| ESPAsyncWebServer | me-no-dev |
| AsyncTCP | me-no-dev |
| ArduinoJson | Benoit Blanchon |

Board: **ESP32** (Arduino ESP32 Core 2.x or 3.x)

## Configuration

Edit `config.h` before uploading:

```cpp
const char* AP_SSID = "Enterprise Network";   // Network name shown to clients
const char* AP_PASSWORD = "";                  // Empty = open network
const IPAddress AP_IP(10, 22, 4, 1);          // Access point IP
const char* ADMIN_PASSWORD = "your_password";  // Admin dashboard login
```

## Upload

1. Open `CaptivePortal.ino` in Arduino IDE
2. Select your ESP32 board under **Tools > Board**
3. Click **Upload**
4. (Optional) Upload SPIFFS data via **Tools > ESP32 Sketch Data Upload** if using the `data/` folder

## Usage

1. Power on the ESP32 — it creates the Wi-Fi AP
2. Connect any device — the captive portal appears automatically
3. Access the admin dashboard at the AP IP (e.g., `http://10.22.4.1/admin`)
4. Log in with the admin password to view connected devices, credentials, and clients

## License

© 2026 Khaled M. Alshammri | [@ik0z](https://github.com/ik0z). All rights reserved.
