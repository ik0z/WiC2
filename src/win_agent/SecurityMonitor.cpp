/*
 * ============================================================================
 *  Security Monitor Client for Windows
 *  Version: 1.0.0
 *  
 *  Collects device security information and reports to the ESP32 router:
 *  - OS Type and Version
 *  - Installed Antivirus/EDR solutions
 *  - Online/Offline status
 *  - Device hostname and IP
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
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

// Configuration
const wchar_t* ROUTER_HOST = L"10.22.4.1";
const int ROUTER_PORT = 80;
const wchar_t* REPORT_ENDPOINT = L"/api/client-report";
const int REPORT_INTERVAL_SECONDS = 60;

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
            default: result += c; break;
        }
    }
    return result;
}

// ============================================================================
//  OS Information
// ============================================================================
struct OSInfo {
    std::string osType;
    std::string osVersion;
    std::string osBuild;
    std::string osArchitecture;
};

OSInfo GetOSInfo() {
    OSInfo info;
    info.osType = "Windows";
    
    // Get OS version from registry (more reliable for Windows 10/11)
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        wchar_t buffer[256];
        DWORD bufferSize = sizeof(buffer);
        
        // Product Name (e.g., "Windows 11 Pro")
        if (RegQueryValueExW(hKey, L"ProductName", nullptr, nullptr, 
            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            info.osVersion = WStringToString(buffer);
        }
        
        // Build number
        bufferSize = sizeof(buffer);
        if (RegQueryValueExW(hKey, L"CurrentBuild", nullptr, nullptr, 
            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            info.osBuild = WStringToString(buffer);
        }
        
        // Display Version (e.g., "23H2")
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
        case PROCESSOR_ARCHITECTURE_AMD64:
            info.osArchitecture = "x64";
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            info.osArchitecture = "ARM64";
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            info.osArchitecture = "x86";
            break;
        default:
            info.osArchitecture = "Unknown";
    }
    
    return info;
}

// ============================================================================
//  Hostname and IP
// ============================================================================
std::string GetHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "Unknown";
}

std::string GetLocalIP() {
    // Simple method to get local IP
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return "Unknown";
    }
    
    struct hostent* host = gethostbyname(hostname);
    if (host == nullptr) {
        return "Unknown";
    }
    
    struct in_addr addr;
    memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
    return inet_ntoa(addr);
}

// ============================================================================
//  Antivirus/EDR Detection using Windows Security Center
// ============================================================================
struct SecurityProduct {
    std::string name;
    std::string state;
    std::string type;
};

std::vector<SecurityProduct> GetSecurityProducts() {
    std::vector<SecurityProduct> products;
    
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return products;
    }
    
    // Try Windows Security Center API first
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
                            case WSC_SECURITY_PRODUCT_STATE_ON:
                                sp.state = "Active";
                                break;
                            case WSC_SECURITY_PRODUCT_STATE_OFF:
                                sp.state = "Inactive";
                                break;
                            case WSC_SECURITY_PRODUCT_STATE_SNOOZED:
                                sp.state = "Snoozed";
                                break;
                            default:
                                sp.state = "Unknown";
                        }
                    }
                    
                    if (!sp.name.empty()) {
                        products.push_back(sp);
                    }
                    pProduct->Release();
                }
            }
        }
        
        // Get Firewall products
        hr = pProdList->Initialize(WSC_SECURITY_PROVIDER_FIREWALL);
        if (SUCCEEDED(hr)) {
            LONG count = 0;
            pProdList->get_Count(&count);
            
            for (LONG i = 0; i < count; i++) {
                IWscProduct* pProduct = nullptr;
                hr = pProdList->get_Item(i, &pProduct);
                if (SUCCEEDED(hr) && pProduct) {
                    SecurityProduct sp;
                    sp.type = "Firewall";
                    
                    BSTR name = nullptr;
                    if (SUCCEEDED(pProduct->get_ProductName(&name)) && name) {
                        sp.name = WStringToString(name);
                        SysFreeString(name);
                    }
                    
                    WSC_SECURITY_PRODUCT_STATE state;
                    if (SUCCEEDED(pProduct->get_ProductState(&state))) {
                        switch (state) {
                            case WSC_SECURITY_PRODUCT_STATE_ON:
                                sp.state = "Active";
                                break;
                            case WSC_SECURITY_PRODUCT_STATE_OFF:
                                sp.state = "Inactive";
                                break;
                            default:
                                sp.state = "Unknown";
                        }
                    }
                    
                    if (!sp.name.empty()) {
                        products.push_back(sp);
                    }
                    pProduct->Release();
                }
            }
        }
        
        pProdList->Release();
    }
    
    // Fallback: Check for common EDR/AV processes
    const char* edrProcesses[] = {
        "MsMpEng.exe",      // Windows Defender
        "cb.exe",           // Carbon Black
        "CylanceSvc.exe",   // Cylance
        "csfalconservice.exe", // CrowdStrike Falcon
        "SentinelAgent.exe", // SentinelOne
        "xagt.exe",         // FireEye
        "taniumclient.exe", // Tanium
        "ossec-agent.exe",  // OSSEC
        "elastic-agent.exe", // Elastic
        "qualysagent.exe",  // Qualys
        nullptr
    };
    
    const char* edrNames[] = {
        "Windows Defender",
        "Carbon Black",
        "Cylance",
        "CrowdStrike Falcon",
        "SentinelOne",
        "FireEye",
        "Tanium",
        "OSSEC",
        "Elastic Agent",
        "Qualys Agent",
        nullptr
    };
    
    for (int i = 0; edrProcesses[i] != nullptr; i++) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe;
            pe.dwSize = sizeof(pe);
            
            if (Process32FirstW(hSnapshot, &pe)) {
                do {
                    std::wstring procName = pe.szExeFile;
                    std::string procNameStr = WStringToString(procName);
                    
                    if (_stricmp(procNameStr.c_str(), edrProcesses[i]) == 0) {
                        // Check if already in list
                        bool found = false;
                        for (const auto& p : products) {
                            if (p.name == edrNames[i]) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            SecurityProduct sp;
                            sp.name = edrNames[i];
                            sp.type = "EDR";
                            sp.state = "Running";
                            products.push_back(sp);
                        }
                        break;
                    }
                } while (Process32NextW(hSnapshot, &pe));
            }
            CloseHandle(hSnapshot);
        }
    }
    
    CoUninitialize();
    return products;
}

// ============================================================================
//  Network Communication
// ============================================================================
bool SendReportToRouter(const std::string& jsonData) {
    HINTERNET hSession = WinHttpOpen(L"SecurityMonitor/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) {
        std::cerr << "[ERROR] Failed to open HTTP session" << std::endl;
        return false;
    }
    
    HINTERNET hConnect = WinHttpConnect(hSession, ROUTER_HOST, ROUTER_PORT, 0);
    if (!hConnect) {
        std::cerr << "[ERROR] Failed to connect to router" << std::endl;
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", REPORT_ENDPOINT,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    
    if (!hRequest) {
        std::cerr << "[ERROR] Failed to open HTTP request" << std::endl;
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    
    // Set headers
    const wchar_t* headers = L"Content-Type: application/json\r\n";
    
    // Send request
    BOOL result = WinHttpSendRequest(hRequest, headers, -1,
        (LPVOID)jsonData.c_str(), (DWORD)jsonData.length(),
        (DWORD)jsonData.length(), 0);
    
    if (result) {
        result = WinHttpReceiveResponse(hRequest, nullptr);
    }
    
    if (result) {
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
        
        if (statusCode == 200) {
            std::cout << "[OK] Report sent successfully" << std::endl;
        } else {
            std::cerr << "[WARN] Server returned status: " << statusCode << std::endl;
        }
    } else {
        std::cerr << "[ERROR] Failed to send request: " << GetLastError() << std::endl;
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return result == TRUE;
}

// ============================================================================
//  Build JSON Report
// ============================================================================
std::string BuildJsonReport() {
    OSInfo os = GetOSInfo();
    std::string hostname = GetHostname();
    std::string localIP = GetLocalIP();
    std::vector<SecurityProduct> security = GetSecurityProducts();
    
    std::ostringstream json;
    json << "{";
    json << "\"hostname\":\"" << EscapeJson(hostname) << "\",";
    json << "\"ip\":\"" << EscapeJson(localIP) << "\",";
    json << "\"osType\":\"" << EscapeJson(os.osType) << "\",";
    json << "\"osVersion\":\"" << EscapeJson(os.osVersion) << "\",";
    json << "\"osBuild\":\"" << EscapeJson(os.osBuild) << "\",";
    json << "\"osArch\":\"" << EscapeJson(os.osArchitecture) << "\",";
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
    
    return json.str();
}

// ============================================================================
//  Console Banner
// ============================================================================
void PrintBanner() {
    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "  Security Monitor Client v1.0.0" << std::endl;
    std::cout << "  Reporting to: " << WStringToString(ROUTER_HOST) << ":" << ROUTER_PORT << std::endl;
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
int main() {
    PrintBanner();
    
    // Initialize Winsock for hostname/IP functions
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[ERROR] Failed to initialize Winsock" << std::endl;
        return 1;
    }
    
    std::cout << "[INFO] Collecting system information..." << std::endl;
    
    // Initial report
    OSInfo os = GetOSInfo();
    std::cout << "[INFO] OS: " << os.osVersion << " (" << os.osArchitecture << ")" << std::endl;
    std::cout << "[INFO] Build: " << os.osBuild << std::endl;
    std::cout << "[INFO] Hostname: " << GetHostname() << std::endl;
    std::cout << "[INFO] IP: " << GetLocalIP() << std::endl;
    
    std::vector<SecurityProduct> security = GetSecurityProducts();
    std::cout << "[INFO] Security products found: " << security.size() << std::endl;
    for (const auto& sp : security) {
        std::cout << "       - " << sp.name << " (" << sp.type << "): " << sp.state << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "[INFO] Starting monitoring loop (interval: " << REPORT_INTERVAL_SECONDS << "s)" << std::endl;
    std::cout << std::endl;
    
    // Main loop
    while (true) {
        std::string report = BuildJsonReport();
        std::cout << "[SEND] Sending report to router..." << std::endl;
        
        if (SendReportToRouter(report)) {
            std::cout << "[OK] Report sent at " << std::time(nullptr) << std::endl;
        } else {
            std::cout << "[WARN] Failed to send report, will retry..." << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(REPORT_INTERVAL_SECONDS));
    }
    
    WSACleanup();
    return 0;
}

/*
 * ============================================================================
 *  Build Instructions:
 *  
 *  Using MSVC (Visual Studio):
 *    cl /EHsc /O2 SecurityMonitor.cpp /link ws2_32.lib winhttp.lib wbemuuid.lib ole32.lib oleaut32.lib wscapi.lib
 *  
 *  Using MinGW-w64:
 *    g++ -o SecurityMonitor.exe SecurityMonitor.cpp -lws2_32 -lwinhttp -lwbemuuid -lole32 -loleaut32 -lwscapi -static
 * 
 * ============================================================================
 *  © 2026 Khaled M.Alshammri | @ik0z . All rights reserved.
 *  https://github.com/ik0z
 * ============================================================================
 */
