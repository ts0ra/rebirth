#pragma once
#define WIN32_LEAN_AND_MEAN
#include <overlay.h>
#include <Windows.h>
#include <TlHelp32.h>

#include <cstdint>
#include <thread>
#include <vector>

class Memory
{
private:
    std::uintptr_t processId = 0;
    HANDLE processHandle = nullptr;

    void CloseHandleSafe(HANDLE handle) const noexcept
    {
        if (handle && !::CloseHandle(handle))
        {
            appLog.AddLog("[-] Failed to close handle, error code: %d", GetLastError());
        }
    }

public:
    Memory(const WCHAR* processName) noexcept
    {
        appLog.AddLog("[+] Memory constructor called, finding process handle: %s\n", processName);

        while (processId == 0)
        {
            PROCESSENTRY32 entry = { sizeof(PROCESSENTRY32) };
            const HANDLE snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapShot == INVALID_HANDLE_VALUE)
            {
                appLog.AddLog("[-] Failed to create process snapshot, error code: %d\n", GetLastError());
                std::exit(EXIT_FAILURE);
            }

            while (::Process32Next(snapShot, &entry))
            {
                if (wcscmp(processName, entry.szExeFile) == 0)
                {
                    processId = entry.th32ProcessID;
                    processHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
                    if (!processHandle)
                    {
                        appLog.AddLog("[-] Failed to open process handle, error code: %d\n", GetLastError());
                        std::exit(EXIT_FAILURE);
                    }
                    appLog.AddLog("[+] Process handle found\n");
                    break;
                }
            }

            CloseHandleSafe(snapShot);

            if (processId == 0)
            {
                break;
            }
        }
    }

    ~Memory()
    {
        appLog.AddLog("[-] Memory destructor called\n");
        CloseHandleSafe(processHandle);
    }

    void Reinitialize(const WCHAR* processName) noexcept
    {
        while (processId == 0)
        {
            PROCESSENTRY32 entry = { sizeof(PROCESSENTRY32) };
            const HANDLE snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapShot == INVALID_HANDLE_VALUE)
            {
                appLog.AddLog("[-] Failed to create process snapshot, error code: %d\n", GetLastError());
                std::exit(EXIT_FAILURE);
            }

            while (::Process32Next(snapShot, &entry))
            {
                if (wcscmp(processName, entry.szExeFile) == 0)
                {
                    processId = entry.th32ProcessID;
                    processHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
                    if (!processHandle)
                    {
                        appLog.AddLog("[-] Failed to open process handle, error code: %d\n", GetLastError());
                        std::exit(EXIT_FAILURE);
                    }
                    appLog.AddLog("[+] Process handle found\n");
                    break;
                }
            }

            CloseHandleSafe(snapShot);
            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (processId == 0)
            {
                break;
            }
        }
    }

    std::uintptr_t GetModuleAddress(const WCHAR* moduleName) const noexcept
    {
        MODULEENTRY32 entry = { sizeof(MODULEENTRY32) };
        const HANDLE snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
        if (snapShot == INVALID_HANDLE_VALUE)
        {
            appLog.AddLog("[-] Failed to create module snapshot, error code: %d\n", GetLastError());
            return 0;
        }

        std::uintptr_t result = 0;
        while (::Module32Next(snapShot, &entry))
        {
            if (wcscmp(moduleName, entry.szModule) == 0)
            {
                result = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
                break;
            }
        }

        CloseHandleSafe(snapShot);

        if (result == 0)
        {
            appLog.AddLog("[-] Module not found\n");
            std::exit(EXIT_FAILURE);
        }

        return result;
    }

    template <typename T>
    void ReadChar(const std::uintptr_t& address, T* value, int size) const noexcept
    {
        if (!::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), value, sizeof(T) * size, nullptr))
        {
            appLog.AddLog("[-] Failed to read process memory, error code: %d\n", GetLastError());
        }
    }

    template <typename T>
    T Read(const std::uintptr_t& address) const noexcept
    {
        T value = {};
        if (!::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), nullptr))
        {
            appLog.AddLog("[-] Failed to read process memory, error code: %d\n", GetLastError());
        }
        return value;
    }

    template <typename T>
    T Read(const std::uintptr_t& address, const std::vector<uintptr_t>& offsets) const noexcept
    {
        uintptr_t currentAddress = address;

        for (size_t i = 0; i < offsets.size() - 1; ++i)
        {
            if (!::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(currentAddress + offsets[i]), &currentAddress, sizeof(uintptr_t), nullptr))
            {
                appLog.AddLog("[-] Failed to read process memory, error code: %d\n", GetLastError());
            }
        }

        T value = {};
        if (!::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(currentAddress + offsets.back()), &value, sizeof(T), nullptr))
        {
            appLog.AddLog("[-] Failed to read process memory, error code: %d\n", GetLastError());
        }

        return value;
    }

    template <typename T>
    void Write(const std::uintptr_t& address, const T& value) const noexcept
    {
        if (!::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), nullptr))
        {
            appLog.AddLog("[-] Failed to write process memory, error code: %d\n", GetLastError());
        }
    }

    template <size_t N>
    void Write(const std::uintptr_t& address, const char(&value)[N]) const noexcept
    {
        if (!::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), value, N, nullptr))
        {
            appLog.AddLog("[-] Failed to write process memory, error code: %d\n", GetLastError());
        }
    }

    std::uintptr_t GetProcessId() const noexcept
    {
        return processId;
    }

    std::uintptr_t GetHandleAddress() const noexcept
    {
        return reinterpret_cast<uintptr_t>(processHandle);
    }

    void PatchEx(BYTE* dst, BYTE* src, unsigned int size) const
    {
        DWORD oldProtect;
        ::VirtualProtectEx(processHandle, dst, size, PAGE_EXECUTE_READWRITE, &oldProtect);
        ::WriteProcessMemory(processHandle, dst, src, size, nullptr);
        ::VirtualProtectEx(processHandle, dst, size, oldProtect, &oldProtect);
    }

    void NopEx(BYTE* dst, unsigned int size) const
    {
        std::vector<BYTE> nopArray(size, 0x90);
        PatchEx(dst, nopArray.data(), size);
    }
};

typedef Memory MEMORY;