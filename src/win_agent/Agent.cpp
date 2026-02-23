/*
 * ============================================================================
 *  Enterprise Network Agent for Windows
 *  Version: 2.0.0
 *  
 *  Features:
 *  - Forces Windows to connect to specified Wi-Fi network
 *  - Sends system data and connectivity status to router
 *  - Executes remote commands (ipconfig, netstat, etc.)
 *  - Resilient to failures with automatic reconnection
 *  - Runs as background service
 * 
 * ============================================================================
 *  © 2026 Khaled M.Alshammri | @ik0z . All rights reserved.
 *  https://github.com/ik0z
 * ============================================================================
 */

#define _WIN32_WINNT 0x0601
#define WINVER 0x0601
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <wbemidl.h>
#include <comdef.h>
#include <iwscapi.h>
#include <wscapi.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <wlanapi.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>
#include <atomic>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "wlanapi.lib")

// ============================================================================
//  Configuration
// ============================================================================
const wchar_t* ROUTER_HOST = L"10.22.4.1";
const int ROUTER_PORT = 80;
const wchar_t* TARGET_SSID = L"Enterprise Network";
const wchar_t* TARGET_PASSWORD = L"";
const int HEARTBEAT_INTERVAL_SECONDS = 30;
const int COMMAND_POLL_INTERVAL_SECONDS = 5;
const int WIFI_CHECK_INTERVAL_SECONDS = 10;
const int MAX_RETRY_ATTEMPTS = 5;
const int RETRY_DELAY_SECONDS = 3;

// Global state
std::atomic<bool> g_running(true);
std::atomic<bool> g_connected(false);
std::mutex g_mutex;
std::string g_hostname;
std::string g_clientId;

// ============================================================================
//  Utility Functions
// ============================================================================
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
    return str;
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}

std::string EscapeJson(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 32) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

std::string GetHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "Unknown";
}

std::string GenerateClientId(const std::string& ip) {
    std::string hostname = GetHostname();
    std::string ipClean = ip;
    ipClean.erase(std::remove(ipClean.begin(), ipClean.end(), '.'), ipClean.end());
    return hostname + "-" + ipClean;
}

// ============================================================================
//  OS Information
// ============================================================================
struct SystemInfo {
    std::string hostname;
    std::string osType;
    std::string osVersion;
    std::string osBuild;
    std::string osArch;
    std::string localIP;
    std::string macAddress;
    std::string username;
    bool wifiConnected;
    std::string connectedSSID;
    int signalStrength;
};

SystemInfo GetSystemInfo() {
    SystemInfo info;
    info.hostname = GetHostname();
    info.osType = "Windows";
    info.wifiConnected = false;
    info.signalStrength = 0;
    
    // Get OS version from registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        wchar_t buffer[256];
        DWORD bufferSize = sizeof(buffer);
        
        if (RegQueryValueExW(hKey, L"ProductName", nullptr, nullptr, 
            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            info.osVersion = WStringToString(buffer);
        }
        
        bufferSize = sizeof(buffer);
        if (RegQueryValueExW(hKey, L"CurrentBuild", nullptr, nullptr, 
            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            info.osBuild = WStringToString(buffer);
        }
        
        bufferSize = sizeof(buffer);
        if (RegQueryValueExW(hKey, L"DisplayVersion", nullptr, nullptr, 
            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            info.osVersion += " (" + WStringToString(buffer) + ")";
        }
        
        RegCloseKey(hKey);
    }
    
    // Get architecture
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: info.osArch = "x64"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: info.osArch = "ARM64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: info.osArch = "x86"; break;
        default: info.osArch = "Unknown";
    }
    
    // Get username
    char username[256];
    DWORD usernameLen = sizeof(username);
    if (GetUserNameA(username, &usernameLen)) {
        info.username = username;
    }
    
    // Get local IP
    char hostbuf[256];
    if (gethostname(hostbuf, sizeof(hostbuf)) == 0) {
        struct addrinfo hints = {0}, *result = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(hostbuf, nullptr, &hints, &result) == 0 && result) {
            char ipbuf[INET_ADDRSTRLEN];
            struct sockaddr_in* addr = (struct sockaddr_in*)result->ai_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ipbuf, sizeof(ipbuf));
            info.localIP = ipbuf;
            freeaddrinfo(result);
        }
    }
    
    // Get MAC address
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufLen = sizeof(adapterInfo);
    if (GetAdaptersInfo(adapterInfo, &bufLen) == ERROR_SUCCESS) {
        char mac[18];
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
            adapterInfo[0].Address[0], adapterInfo[0].Address[1],
            adapterInfo[0].Address[2], adapterInfo[0].Address[3],
            adapterInfo[0].Address[4], adapterInfo[0].Address[5]);
        info.macAddress = mac;
    }
    
    return info;
}

// ============================================================================
//  WiFi Management
// ============================================================================
bool IsConnectedToTargetNetwork() {
    HANDLE hClient = nullptr;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    
    if (WlanOpenHandle(dwMaxClient, nullptr, &dwCurVersion, &hClient) != ERROR_SUCCESS) {
        return false;
    }
    
    PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;
    if (WlanEnumInterfaces(hClient, nullptr, &pIfList) != ERROR_SUCCESS) {
        WlanCloseHandle(hClient, nullptr);
        return false;
    }
    
    bool connected = false;
    
    for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++) {
        PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];
        
        if (pIfInfo->isState == wlan_interface_state_connected) {
            PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = nullptr;
            DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
            WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
            
            if (WlanQueryInterface(hClient, &pIfInfo->InterfaceGuid,
                wlan_intf_opcode_current_connection, nullptr,
                &connectInfoSize, (PVOID*)&pConnectInfo, &opCode) == ERROR_SUCCESS) {
                
                std::string ssid((char*)pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID,
                    pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength);
                
                std::string targetSsid = WStringToString(TARGET_SSID);
                if (ssid == targetSsid) {
                    connected = true;
                }
                
                WlanFreeMemory(pConnectInfo);
            }
        }
    }
    
    WlanFreeMemory(pIfList);
    WlanCloseHandle(hClient, nullptr);
    
    return connected;
}

std::string GetCurrentSSID() {
    HANDLE hClient = nullptr;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    std::string ssid = "";
    
    if (WlanOpenHandle(dwMaxClient, nullptr, &dwCurVersion, &hClient) != ERROR_SUCCESS) {
        return ssid;
    }
    
    PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;
    if (WlanEnumInterfaces(hClient, nullptr, &pIfList) != ERROR_SUCCESS) {
        WlanCloseHandle(hClient, nullptr);
        return ssid;
    }
    
    for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++) {
        PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];
        
        if (pIfInfo->isState == wlan_interface_state_connected) {
            PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = nullptr;
            DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
            WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
            
            if (WlanQueryInterface(hClient, &pIfInfo->InterfaceGuid,
                wlan_intf_opcode_current_connection, nullptr,
                &connectInfoSize, (PVOID*)&pConnectInfo, &opCode) == ERROR_SUCCESS) {
                
                ssid = std::string((char*)pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID,
                    pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength);
                
                WlanFreeMemory(pConnectInfo);
                break;
            }
        }
    }
    
    WlanFreeMemory(pIfList);
    WlanCloseHandle(hClient, nullptr);
    
    return ssid;
}

bool ForceConnectToNetwork() {
    std::cout << "[WIFI] Attempting to connect to: " << WStringToString(TARGET_SSID) << std::endl;
    
    HANDLE hClient = nullptr;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    
    if (WlanOpenHandle(dwMaxClient, nullptr, &dwCurVersion, &hClient) != ERROR_SUCCESS) {
        std::cerr << "[WIFI] Failed to open WLAN handle" << std::endl;
        return false;
    }
    
    PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;
    if (WlanEnumInterfaces(hClient, nullptr, &pIfList) != ERROR_SUCCESS) {
        std::cerr << "[WIFI] Failed to enumerate interfaces" << std::endl;
        WlanCloseHandle(hClient, nullptr);
        return false;
    }
    
    if (pIfList->dwNumberOfItems == 0) {
        std::cerr << "[WIFI] No wireless interfaces found" << std::endl;
        WlanFreeMemory(pIfList);
        WlanCloseHandle(hClient, nullptr);
        return false;
    }
    
    GUID interfaceGuid = pIfList->InterfaceInfo[0].InterfaceGuid;
    
    // Create profile XML
    std::wstring ssid = TARGET_SSID;
    std::wstring password = TARGET_PASSWORD;
    
    std::wostringstream profileXml;
    profileXml << L"<?xml version=\"1.0\"?>";
    profileXml << L"<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">";
    profileXml << L"<name>" << ssid << L"</name>";
    profileXml << L"<SSIDConfig><SSID><name>" << ssid << L"</name></SSID></SSIDConfig>";
    profileXml << L"<connectionType>ESS</connectionType>";
    profileXml << L"<connectionMode>auto</connectionMode>";
    profileXml << L"<MSM><security>";
    
    if (password.empty()) {
        profileXml << L"<authEncryption><authentication>open</authentication>";
        profileXml << L"<encryption>none</encryption><useOneX>false</useOneX></authEncryption>";
    } else {
        profileXml << L"<authEncryption><authentication>WPA2PSK</authentication>";
        profileXml << L"<encryption>AES</encryption><useOneX>false</useOneX></authEncryption>";
        profileXml << L"<sharedKey><keyType>passPhrase</keyType><protected>false</protected>";
        profileXml << L"<keyMaterial>" << password << L"</keyMaterial></sharedKey>";
    }
    
    profileXml << L"</security></MSM></WLANProfile>";
    
    std::wstring profile = profileXml.str();
    
    // Set profile
    DWORD dwReasonCode = 0;
    DWORD result = WlanSetProfile(hClient, &interfaceGuid, 0, profile.c_str(), 
        nullptr, TRUE, nullptr, &dwReasonCode);
    
    if (result != ERROR_SUCCESS) {
        std::cerr << "[WIFI] Failed to set profile: " << result << std::endl;
    }
    
    // Connect
    WLAN_CONNECTION_PARAMETERS connParams;
    ZeroMemory(&connParams, sizeof(connParams));
    connParams.wlanConnectionMode = wlan_connection_mode_profile;
    connParams.strProfile = ssid.c_str();
    connParams.pDot11Ssid = nullptr;
    connParams.pDesiredBssidList = nullptr;
    connParams.dot11BssType = dot11_BSS_type_any;
    connParams.dwFlags = 0;
    
    result = WlanConnect(hClient, &interfaceGuid, &connParams, nullptr);
    
    WlanFreeMemory(pIfList);
    WlanCloseHandle(hClient, nullptr);
    
    if (result == ERROR_SUCCESS) {
        std::cout << "[WIFI] Connection initiated successfully" << std::endl;
        // Wait for connection
        for (int i = 0; i < 10; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (IsConnectedToTargetNetwork()) {
                std::cout << "[WIFI] Connected to target network!" << std::endl;
                return true;
            }
        }
    }
    
    std::cerr << "[WIFI] Failed to connect: " << result << std::endl;
    return false;
}

// ============================================================================
//  Security Products Detection
// ============================================================================
struct SecurityProduct {
    std::string name;
    std::string state;
    std::string type;
};

std::vector<SecurityProduct> GetSecurityProducts() {
    std::vector<SecurityProduct> products;
    
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return products;
    }
    
    IWSCProductList* pProdList = nullptr;
    hr = CoCreateInstance(__uuidof(WSCProductList), nullptr, CLSCTX_INPROC_SERVER,
        __uuidof(IWSCProductList), (void**)&pProdList);
    
    if (SUCCEEDED(hr) && pProdList) {
        // Get Antivirus products
        hr = pProdList->Initialize(WSC_SECURITY_PROVIDER_ANTIVIRUS);
        if (SUCCEEDED(hr)) {
            LONG count = 0;
            pProdList->get_Count(&count);
            
            for (LONG i = 0; i < count; i++) {
                IWscProduct* pProduct = nullptr;
                hr = pProdList->get_Item(i, &pProduct);
                if (SUCCEEDED(hr) && pProduct) {
                    SecurityProduct sp;
                    sp.type = "Antivirus";
                    
                    BSTR name = nullptr;
                    if (SUCCEEDED(pProduct->get_ProductName(&name)) && name) {
                        sp.name = WStringToString(name);
                        SysFreeString(name);
                    }
                    
                    WSC_SECURITY_PRODUCT_STATE state;
                    if (SUCCEEDED(pProduct->get_ProductState(&state))) {
                        switch (state) {
                            case WSC_SECURITY_PRODUCT_STATE_ON: sp.state = "Active"; break;
                            case WSC_SECURITY_PRODUCT_STATE_OFF: sp.state = "Inactive"; break;
                            case WSC_SECURITY_PRODUCT_STATE_SNOOZED: sp.state = "Snoozed"; break;
                            default: sp.state = "Unknown";
                        }
                    }
                    
                    if (!sp.name.empty()) products.push_back(sp);
                    pProduct->Release();
                }
            }
        }
        pProdList->Release();
    }
    
    CoUninitialize();
    return products;
}

// ============================================================================
//  Command Execution
// ============================================================================
std::string ExecuteCommand(const std::string& command) {
    std::string result;
    
    // Security check - only allow specific commands
    std::vector<std::string> allowedCommands = {
        "ipconfig", "netstat", "systeminfo", "tasklist", "hostname",
        "whoami", "dir", "type", "ping", "tracert", "nslookup",
        "arp", "route", "netsh", "wmic", "getmac"
    };
    
    std::string cmdLower = command;
    std::transform(cmdLower.begin(), cmdLower.end(), cmdLower.begin(), ::tolower);
    
    bool allowed = false;
    for (const auto& ac : allowedCommands) {
        if (cmdLower.find(ac) == 0) {
            allowed = true;
            break;
        }
    }
    
    if (!allowed) {
        return "Error: Command not allowed for security reasons";
    }
    
    // Execute command
    std::string fullCmd = "cmd /c " + command + " 2>&1";
    
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;
    
    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return "Error: Failed to create pipe";
    }
    
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
    
    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    PROCESS_INFORMATION pi = {0};
    
    if (CreateProcessA(nullptr, (LPSTR)fullCmd.c_str(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        
        CloseHandle(hWritePipe);
        
        char buffer[4096];
        DWORD bytesRead;
        
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            result += buffer;
        }
        
        WaitForSingleObject(pi.hProcess, 30000); // 30 second timeout
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        CloseHandle(hWritePipe);
        result = "Error: Failed to execute command";
    }
    
    CloseHandle(hReadPipe);
    
    // Limit output size
    if (result.length() > 32000) {
        result = result.substr(0, 32000) + "\n... (output truncated)";
    }
    
    return result;
}

// ============================================================================
//  HTTP Communication
// ============================================================================
std::string HttpRequest(const std::wstring& method, const std::wstring& endpoint, 
                        const std::string& body = "") {
    std::string response;
    
    HINTERNET hSession = WinHttpOpen(L"EnterpriseAgent/2.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return "";
    
    HINTERNET hConnect = WinHttpConnect(hSession, ROUTER_HOST, ROUTER_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), endpoint.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    const wchar_t* headers = L"Content-Type: application/json\r\n";
    
    BOOL result;
    if (body.empty()) {
        result = WinHttpSendRequest(hRequest, headers, -1, nullptr, 0, 0, 0);
    } else {
        result = WinHttpSendRequest(hRequest, headers, -1,
            (LPVOID)body.c_str(), (DWORD)body.length(),
            (DWORD)body.length(), 0);
    }
    
    if (result) {
        result = WinHttpReceiveResponse(hRequest, nullptr);
    }
    
    if (result) {
        DWORD bytesAvailable = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
            std::vector<char> buffer(bytesAvailable + 1);
            DWORD bytesRead = 0;
            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                buffer[bytesRead] = '\0';
                response += buffer.data();
            }
        }
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return response;
}

bool SendHeartbeat(const SystemInfo& info, const std::vector<SecurityProduct>& security) {
    std::ostringstream json;
    json << "{";
    json << "\"clientId\":\"" << EscapeJson(g_clientId) << "\",";
    json << "\"hostname\":\"" << EscapeJson(info.hostname) << "\",";
    json << "\"ip\":\"" << EscapeJson(info.localIP) << "\",";
    json << "\"mac\":\"" << EscapeJson(info.macAddress) << "\",";
    json << "\"username\":\"" << EscapeJson(info.username) << "\",";
    json << "\"osType\":\"" << EscapeJson(info.osType) << "\",";
    json << "\"osVersion\":\"" << EscapeJson(info.osVersion) << "\",";
    json << "\"osBuild\":\"" << EscapeJson(info.osBuild) << "\",";
    json << "\"osArch\":\"" << EscapeJson(info.osArch) << "\",";
    json << "\"wifiConnected\":" << (info.wifiConnected ? "true" : "false") << ",";
    json << "\"connectedSSID\":\"" << EscapeJson(info.connectedSSID) << "\",";
    json << "\"status\":\"online\",";
    json << "\"security\":[";
    
    for (size_t i = 0; i < security.size(); i++) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"name\":\"" << EscapeJson(security[i].name) << "\",";
        json << "\"type\":\"" << EscapeJson(security[i].type) << "\",";
        json << "\"state\":\"" << EscapeJson(security[i].state) << "\"";
        json << "}";
    }
    
    json << "],";
    json << "\"timestamp\":" << std::time(nullptr);
    json << "}";
    
    std::string response = HttpRequest(L"POST", L"/api/agent-heartbeat", json.str());
    return !response.empty() && response.find("\"success\":true") != std::string::npos;
}

std::string CheckForCommands() {
    std::string endpoint = "/api/agent-command?clientId=" + g_clientId;
    return HttpRequest(L"GET", StringToWString(endpoint));
}

bool SendCommandResult(const std::string& commandId, const std::string& result) {
    std::ostringstream json;
    json << "{";
    json << "\"clientId\":\"" << EscapeJson(g_clientId) << "\",";
    json << "\"commandId\":\"" << EscapeJson(commandId) << "\",";
    json << "\"result\":\"" << EscapeJson(result) << "\",";
    json << "\"timestamp\":" << std::time(nullptr);
    json << "}";
    
    std::string response = HttpRequest(L"POST", L"/api/agent-command-result", json.str());
    return !response.empty() && response.find("\"success\":true") != std::string::npos;
}

// ============================================================================
//  Worker Threads
// ============================================================================
void WiFiMonitorThread() {
    std::cout << "[THREAD] WiFi monitor started" << std::endl;
    
    while (g_running) {
        try {
            bool connected = IsConnectedToTargetNetwork();
            g_connected = connected;
            
            if (!connected) {
                std::cout << "[WIFI] Not connected to target network, attempting connection..." << std::endl;
                ForceConnectToNetwork();
            }
        } catch (...) {
            std::cerr << "[WIFI] Error in WiFi monitor" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(WIFI_CHECK_INTERVAL_SECONDS));
    }
}

void HeartbeatThread() {
    std::cout << "[THREAD] Heartbeat started" << std::endl;
    
    int failCount = 0;
    
    while (g_running) {
        try {
            SystemInfo info = GetSystemInfo();
            info.wifiConnected = g_connected;
            info.connectedSSID = GetCurrentSSID();
            
            std::vector<SecurityProduct> security = GetSecurityProducts();
            
            if (SendHeartbeat(info, security)) {
                std::cout << "[HEARTBEAT] Sent successfully" << std::endl;
                failCount = 0;
            } else {
                failCount++;
                std::cerr << "[HEARTBEAT] Failed (attempt " << failCount << ")" << std::endl;
                
                if (failCount >= MAX_RETRY_ATTEMPTS) {
                    std::cerr << "[HEARTBEAT] Max retries reached, will keep trying..." << std::endl;
                }
            }
        } catch (...) {
            std::cerr << "[HEARTBEAT] Error in heartbeat" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_INTERVAL_SECONDS));
    }
}

void CommandPollerThread() {
    std::cout << "[THREAD] Command poller started" << std::endl;
    
    while (g_running) {
        try {
            std::string response = CheckForCommands();
            
            // Check if there's an actual command (not null)
            if (!response.empty() && response.find("\"command\":\"") != std::string::npos) {
                // Parse command from response (simple parsing)
                size_t cmdStart = response.find("\"command\":\"");
                size_t idStart = response.find("\"commandId\":\"");
                
                if (cmdStart != std::string::npos && idStart != std::string::npos) {
                    cmdStart += 11;
                    size_t cmdEnd = response.find("\"", cmdStart);
                    
                    idStart += 14;
                    size_t idEnd = response.find("\"", idStart);
                    
                    if (cmdEnd != std::string::npos && idEnd != std::string::npos) {
                        std::string command = response.substr(cmdStart, cmdEnd - cmdStart);
                        std::string commandId = response.substr(idStart, idEnd - idStart);
                        
                        if (!command.empty()) {
                            std::cout << "[COMMAND] Received: " << command << std::endl;
                            
                            std::string result = ExecuteCommand(command);
                            
                            if (SendCommandResult(commandId, result)) {
                                std::cout << "[COMMAND] Result sent successfully" << std::endl;
                            } else {
                                std::cerr << "[COMMAND] Failed to send result" << std::endl;
                            }
                        }
                    }
                }
            }
        } catch (...) {
            std::cerr << "[COMMAND] Error in command poller" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(COMMAND_POLL_INTERVAL_SECONDS));
    }
}

// ============================================================================
//  Console Banner
// ============================================================================
void PrintBanner() {
    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "  Enterprise Network Agent v2.0.0" << std::endl;
    std::cout << "  Target Network: " << WStringToString(TARGET_SSID) << std::endl;
    std::cout << "  Router: " << WStringToString(ROUTER_HOST) << ":" << ROUTER_PORT << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "  (c) 2026 Khaled M.Alshammri | @ik0z . All rights reserved." << std::endl;
    std::cout << "  https://github.com/ik0z" << std::endl;
    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << std::endl;
}

// ============================================================================
//  Main Entry Point
// ============================================================================
int main(int argc, char* argv[]) {
    // Check for silent mode
    bool silentMode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-silent") == 0 || strcmp(argv[i], "/silent") == 0) {
            silentMode = true;
        }
    }
    
    if (!silentMode) {
        PrintBanner();
    }
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        if (!silentMode) std::cerr << "[ERROR] Failed to initialize Winsock" << std::endl;
        return 1;
    }
    
    // Get system info and generate client ID
    g_hostname = GetHostname();
    SystemInfo initInfo = GetSystemInfo();
    g_clientId = GenerateClientId(initInfo.localIP);
    
    if (!silentMode) {
        std::cout << "[INFO] Hostname: " << g_hostname << std::endl;
        std::cout << "[INFO] Client ID: " << g_clientId << std::endl;
        std::cout << std::endl;
    }
    
    // Start worker threads
    std::thread wifiThread(WiFiMonitorThread);
    std::thread heartbeatThread(HeartbeatThread);
    std::thread commandThread(CommandPollerThread);
    
    if (!silentMode) {
        std::cout << "[INFO] Agent running. Press Ctrl+C to stop." << std::endl;
        std::cout << std::endl;
    }
    
    // Wait for threads
    wifiThread.join();
    heartbeatThread.join();
    commandThread.join();
    
    WSACleanup();
    return 0;
}

/*
 * ============================================================================
 *  Build Instructions:
 *  
 *  Using MSVC (Visual Studio):
 *    cl /EHsc /O2 /Fe:agent.exe Agent.cpp
 *  
 *  For silent/background operation, run with:
 *    agent.exe -silent
 * 
 * ============================================================================
 *  © 2026 Khaled M.Alshammri | @ik0z . All rights reserved.
 *  https://github.com/ik0z
 * ============================================================================
 */
