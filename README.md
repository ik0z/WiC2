# ESP32 WiC2 ( Command and Control via WIFI ) 

This project is intended for educational purposes only.

The concept of the project is a command-and-control system for devices using a simple ESP32S module.

The following components are not included:

#### iOS/Android agent

#### macOS/Linux agent

This is to prevent any potential misuse of the tool.

## Overview

The system consists of two components:

- **ESP32 Firmware** -- Creates a Wi-Fi access point with captive portal auto-detection, device logging, credential storage, and a full admin dashboard.
- **Windows Agent** -- A background client that connects to the ESP32 AP, collects system and security information, and reports it back to the router.

## Project Structure

```
ESP32/                    -- ESP32 Arduino firmware
  CaptivePortal.ino       -- Main sketch (device detection, web routes, portal logic)
  config.h                -- Configuration (AP settings, admin password, paths)
  storage.h               -- SPIFFS storage (JSON persistence for devices/credentials)
  pages.h                 -- Captive portal HTML pages (per-device landing pages)
  admin_page.h            -- Admin dashboard UI (management interface)

Windows Agent/            -- Windows client application
  Agent.cpp               -- Full agent source (Wi-Fi control, data collection, remote commands)
  SecurityMonitor.cpp     -- Security monitor source (AV/EDR/Firewall detection)
  agent_test.exe           -- Pre-built agent binary
```

## ESP32 Firmware

### Features

- Device detection for Windows, iOS, Android, macOS, and Linux
- Captive portal auto-popup on all major operating systems
- Password-protected admin dashboard with device logs
- Credential storage for mobile devices
- Windows agent download and tracking
- DNS server for portal redirection
- SPIFFS-based persistent storage

### Dependencies

- ESPAsyncWebServer -- https://github.com/me-no-dev/ESPAsyncWebServer
- AsyncTCP -- https://github.com/me-no-dev/AsyncTCP
- ArduinoJson -- https://github.com/bblanchon/ArduinoJson
- ESP32 Arduino Core

### Upload

1. Install Arduino IDE with ESP32 board support
2. Install the libraries listed above
3. Open `ESP32/CaptivePortal.ino`
4. Edit `ESP32/config.h` to set your AP name, password, and admin credentials
5. Select your ESP32 board and upload

## Windows Agent

### Features

- Collects OS version, build number, and architecture
- Detects installed antivirus, EDR, and firewall products
- Sends periodic status reports to the ESP32 router
- Runs in the background with minimal resource usage

### Detected Security Products

| Type | Examples |
|------|----------|
| Antivirus | Windows Defender, Norton, McAfee, Kaspersky, Bitdefender |
| EDR | CrowdStrike Falcon, SentinelOne, Carbon Black, FireEye, Tanium |
| Firewall | Windows Firewall |

### Build from Source

MSVC:

```cmd
cl /EHsc /O2 Agent.cpp /link ws2_32.lib winhttp.lib wbemuuid.lib ole32.lib oleaut32.lib iphlpapi.lib wlanapi.lib advapi32.lib
```

MinGW-w64:

```cmd
g++ -o agent_new.exe Agent.cpp -lws2_32 -lwinhttp -lwbemuuid -lole32 -loleaut32 -liphlpapi -lwlanapi -ladvapi32 -static
```

### Configuration

Edit the constants at the top of `Agent.cpp` to match your ESP32 setup:

```cpp
const wchar_t* ROUTER_HOST = L"10.22.4.1";      // ESP32 AP IP
const int ROUTER_PORT = 80;                      // Web server port
const wchar_t* REPORT_ENDPOINT = L"/api/client-report";
const int REPORT_INTERVAL_SECONDS = 60;
```

## Quick Start

1. Flash the ESP32 firmware (see ESP32 section above)
2. Power on the ESP32 -- it creates the Wi-Fi access point
3. Connect any device -- the captive portal appears automatically
4. Open the admin dashboard at the AP IP (e.g. `http://10.22.4.1/admin`) - Password : Khaled@WiC2
5. Deploy `agent_new.exe` on Windows machines to monitor them from the dashboard

## Requirements

- **ESP32** -- Any ESP32 development board (WROOM, ESP32-S, etc.)
- **Windows Agent** -- Windows 7 or later (10/11 recommended)
- **Arduino IDE** -- 1.8.x or 2.x with ESP32 board support

## Author

Khaled M. Alshammri | [@ik0z](https://github.com/ik0z)

## License

All rights reserved. (c) 2026 Khaled M. Alshammri.
