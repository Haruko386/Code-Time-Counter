#include "../include/tracker.h"
#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <shlwapi.h> 
#include <windows.h> 

// 应用白名单
const std::vector<std::string> TARGET_APPS = {
    // IDEs & Editors
    "Visual Studio", "Code", "IntelliJ", "PyCharm", "CLion", "Eclipse", 
    "Sublime", "Vim", "Neovim", "Atom", "Dev-C++", "Qt Creator", 
    "Android Studio", "Cursor", "HBuilder", "WebStorm", "Rider", "Notepad++",
    // Browsers
    "Chrome", "Edge", "Firefox", "Brave", "Opera", "Safari",
    // Tools
    "DBeaver", "Navicat", "Postman", "Fiddler", "Wireshark", "Docker", 
    "PowerShell", "cmd.exe", "Terminal", "Git", "Unity", "Unreal"
};

std::string ToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::string GetAbsolutePath(const std::string& filename) {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    PathRemoveFileSpecA(buffer); 
    std::string path = std::string(buffer) + "\\" + filename;
    return path;
}

bool MatchesList(const std::string& title, const std::vector<std::string>& list) {
    std::string lowerTitle = title;
    std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);
    for (const auto& keyword : list) {
        std::string lowerKw = keyword;
        std::transform(lowerKw.begin(), lowerKw.end(), lowerKw.begin(), ::tolower);
        if (lowerTitle.find(lowerKw) != std::string::npos) return true;
    }
    return false;
}

void CodeTracker::Init() { LoadData(); }

void CodeTracker::StartLoop() {
    if (m_running) return;
    m_running = true;
    std::thread([this]() { this->Loop(); }).detach();
}

void CodeTracker::StopLoop() {
    m_running = false;
    std::lock_guard<std::mutex> lock(m_mutex);
    SaveData();
    UpdateBadge();
}

void CodeTracker::Loop() {
    while (m_running) {
        std::string title; 
        bool hasWindow = GetActiveWindowTitle(title);
        bool shouldCount = false;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!hasWindow) {
                 m_currentApp = "Idle";
            } else {
                m_currentApp = title;
                if (MatchesList(title, TARGET_APPS)) {
                    shouldCount = true;
                }
            }

            m_isTracking = shouldCount;

            if (shouldCount) {
                m_totalSeconds++;
                m_sessionSeconds++;
                if (m_totalSeconds % 5 == 0) {
                    SaveData();
                    UpdateBadge();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool CodeTracker::GetActiveWindowTitle(std::string& outTitle) {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;
    wchar_t wnd_title[256];
    if (GetWindowTextW(hwnd, wnd_title, sizeof(wnd_title)/sizeof(wchar_t)) == 0) return false;
    outTitle = ToUtf8(std::wstring(wnd_title)); 
    return true;
}

long long CodeTracker::GetTotalTime() { std::lock_guard<std::mutex> lock(m_mutex); return m_totalSeconds; }
long long CodeTracker::GetSessionTime() { std::lock_guard<std::mutex> lock(m_mutex); return m_sessionSeconds; }
bool CodeTracker::IsTracking() { std::lock_guard<std::mutex> lock(m_mutex); return m_isTracking; }
std::string CodeTracker::GetCurrentApp() { std::lock_guard<std::mutex> lock(m_mutex); return m_currentApp; }

void CodeTracker::LoadData() {
    std::string path = GetAbsolutePath(DATA_FILE);
    std::ifstream in(path);
    if (in >> m_totalSeconds) {} 
}
void CodeTracker::SaveData() {
    std::ofstream out(GetAbsolutePath(DATA_FILE));
    out << m_totalSeconds;
}

void CodeTracker::UpdateBadge() {
    long long totalHours = m_totalSeconds / 3600;
    long long minutes = (m_totalSeconds % 3600) / 60;
    long long seconds = m_totalSeconds % 60;
    char timeStr[64];
    sprintf(timeStr, "%lldh%%20%02lldm%%20%02llds", totalHours, minutes, seconds);
    std::string badgeUrl = "https://img.shields.io/badge/Code%20Time-" + std::string(timeStr) + "-blue?style=flat";
    std::string markdown = "![Code Time](" + badgeUrl + ")";
    std::ofstream out(GetAbsolutePath(BADGE_FILE));
    out << markdown;
}