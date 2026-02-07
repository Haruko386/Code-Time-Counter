#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <windows.h>

class CodeTracker {
public:
    static CodeTracker& Get() {
        static CodeTracker instance;
        return instance;
    }

    void Init();
    void StartLoop();
    void StopLoop();

    long long GetTotalTime();
    long long GetSessionTime();
    bool IsTracking();
    std::string GetCurrentApp();

private:
    CodeTracker() {}
    
    void Loop();
    void LoadData();
    void SaveData();
    void UpdateBadge();
    
    bool GetActiveWindowTitle(std::string& outTitle);

    long long m_totalSeconds = 0;
    long long m_sessionSeconds = 0;
    bool m_isTracking = false;
    std::string m_currentApp;
    
    std::atomic<bool> m_running{false};
    std::mutex m_mutex;

    const std::string DATA_FILE = "code_time.dat";
    const std::string BADGE_FILE = "badge.md";
};