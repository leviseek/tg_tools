#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <cctype>
#include <string>
#include <iomanip>
#include <bitset>
#include <io.h>
#include <fcntl.h>

// 设置进程优先级
bool SetProcessPriority(DWORD pid, DWORD priorityClass) {
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        std::cerr << "无法打开进程 PID: " << pid << " 错误: " << GetLastError() << std::endl;
        return false;
    }
    
    if (!SetPriorityClass(hProcess, priorityClass)) {
        std::cerr << "设置优先级失败 PID: " << pid << " 错误: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    
    CloseHandle(hProcess);
    return true;
}

// 设置CPU相关性
bool SetProcessAffinity(DWORD pid, DWORD_PTR affinityMask) {
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        std::cerr << "无法打开进程 PID: " << pid << " 错误: " << GetLastError() << std::endl;
        return false;
    }
    
    if (!SetProcessAffinityMask(hProcess, affinityMask)) {
        std::cerr << "设置相关性失败 PID: " << pid << " 错误: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    
    CloseHandle(hProcess);
    return true;
}

// 启用效能模式 (EcoQoS)
bool EnableEfficiencyMode(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        std::cerr << "无法打开进程 PID: " << pid << " 错误: " << GetLastError() << std::endl;
        return false;
    }

    PROCESS_POWER_THROTTLING_STATE powerThrottling = {0};
    powerThrottling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    powerThrottling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    powerThrottling.StateMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;

    if (!SetProcessInformation(hProcess, ProcessPowerThrottling, 
                               &powerThrottling, sizeof(powerThrottling))) {
        std::cerr << "启用效能模式失败 PID: " << pid << " 错误: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    CloseHandle(hProcess);
    return true;
}

// 获取进程ID列表
std::vector<DWORD> GetProcessIdsByName(const std::wstring& processName) {
    std::vector<DWORD> pids;
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "创建进程快照失败" << std::endl;
        return pids;
    }
    
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return pids;
}

// 将优先级字符串转换为系统常量
DWORD GetPriorityClassFromString(const std::string& priority) {
    if (priority == "idle") return IDLE_PRIORITY_CLASS;
    if (priority == "below") return BELOW_NORMAL_PRIORITY_CLASS;
    if (priority == "normal") return NORMAL_PRIORITY_CLASS;
    if (priority == "above") return ABOVE_NORMAL_PRIORITY_CLASS;
    if (priority == "high") return HIGH_PRIORITY_CLASS;
    if (priority == "realtime") return REALTIME_PRIORITY_CLASS;
    return 0; // 无效值
}

// 计算后三分之一核心的掩码
DWORD_PTR GetLastThirdCoresMask() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    DWORD totalCores = sysInfo.dwNumberOfProcessors;
    if (totalCores <= 1) {
        // 如果系统只有1个核心，使用所有核心
        return 1;
    }
    
    // 计算最后三分之一的核心数量（向上取整）
    DWORD lastThirdCount = (totalCores + 2) / 3; // 向上取整
    if (lastThirdCount < 1) lastThirdCount = 1;
    
    // 计算最后三分之一核心的掩码
    DWORD_PTR mask = 0;
    DWORD startIndex = totalCores - lastThirdCount;
    
    for (DWORD i = startIndex; i < totalCores; i++) {
        mask |= (1ULL << i);
    }
    
    return mask;
}

// 解析掩码参数或特殊关键字
DWORD_PTR ParseAffinityMask(const std::string& maskStr) {
    // 特殊关键字：last3 - 使用后三分之一的核心
    if (maskStr == "last3") {
        return GetLastThirdCoresMask();
    }
    
    // 尝试解析十六进制
    try {
        return std::stoull(maskStr, nullptr, 16);
    } catch (...) {
        return 0;
    }
}

int main(int argc, char* argv[]) {
    // 获取进程名、优先级和相关性掩码
    // 检查参数
    if (argc < 4) {
        std::cout << "===================================" << std::endl;
        std::cout << "用法: " << argv[0] << " <进程名> <优先级> <相关性掩码(十六进制或last3)> [--eco]\n"
                  << "优先级选项: idle, below, normal, above, high, realtime\n"
                  << "相关性掩码选项:\n"
                  << "  十六进制值 (如 0xF 表示前4个核心)\n"
                  << "  last3 (使用系统的后三分之一核心)\n\n"
                  << "添加 --eco 参数启用效能模式\n\n"
                  << "示例1: 使用后三分之一核心\n"
                  << "  ProcessModifier.exe chrome.exe idle last3 --eco\n\n"
                  << "示例2: 使用特定核心\n"
                  << "  ProcessModifier.exe notepad.exe high 0xF\n";
        std::cout << "===================================" << std::endl;
        
        return 1;
    }

    // 解析参数
    std::string processName = argv[1];
    DWORD priorityClass = GetPriorityClassFromString(argv[2]);
    DWORD_PTR affinityMask = ParseAffinityMask(argv[3]);
    bool enableEco = (argc >= 5 && std::string(argv[4]) == "--eco");

    if (priorityClass == 0 || affinityMask == 0) {
        std::cerr << "无效的优先级或相关性参数" << std::endl;
        return 1;
    }

    // 显示掩码信息
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    std::cout << "系统核心数: " << sysInfo.dwNumberOfProcessors << std::endl;
    
    std::bitset<64> bits(affinityMask);
    std::cout << "使用的相关性掩码: 0x" << std::hex << affinityMask 
              << " (二进制: " << bits.to_string().substr(64 - sysInfo.dwNumberOfProcessors) 
              << ")" << std::endl;

    // 转换为宽字符串
    std::wstring wProcessName(processName.begin(), processName.end());

    // 获取所有匹配的进程ID
    std::vector<DWORD> pids = GetProcessIdsByName(wProcessName);
    if (pids.empty()) {
        std::cout << "未找到进程: " << processName << std::endl;
        return 1;
    }

    // 为每个进程应用设置
    std::cout << "找到 " << pids.size() << " 个进程实例\n";
    for (DWORD pid : pids) {
        std::cout << "修改进程 PID: " << pid << "\n";
        
        if (!SetProcessPriority(pid, priorityClass)) {
            std::cerr << "  [!] 设置优先级失败" << std::endl;
        }
        
        if (!SetProcessAffinity(pid, affinityMask)) {
            std::cerr << "  [!] 设置相关性失败" << std::endl;
        }
        
        if (enableEco) {
            if (!EnableEfficiencyMode(pid)) {
                std::cerr << "  [!] 启用效能模式失败" << std::endl;
            } else {
                std::cout << "  [+] 效能模式已启用" << std::endl;
            }
        }
    }

    std::cout << "\n操作完成! 在任务管理器中验证设置。" << std::endl;
    return 0;
}