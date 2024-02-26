#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>

using namespace std;

uintptr_t GetModulesBaseAddress(const wchar_t* modName, DWORD procId) {
    MODULEENTRY32 modEntry;
    modEntry.dwSize = sizeof(modEntry);

    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);

    if (hSnap != INVALID_HANDLE_VALUE) {
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!_wcsicmp(modEntry.szModule, modName)) {
                    modBaseAddr = reinterpret_cast<uintptr_t>(modEntry.modBaseAddr);
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }

    CloseHandle(hSnap);
    return modBaseAddr;
}

uintptr_t findDMAAddy(HANDLE hProc, uintptr_t ptr, vector<unsigned int> offsets) {
    uintptr_t addr = ptr;

    for (unsigned int i = 0; i < offsets.size(); i++) {
        ReadProcessMemory(hProc, reinterpret_cast<BYTE*>(addr), &addr, sizeof(addr), 0);
        addr += offsets[i];
    }

    return addr;
}

int main() {
    DWORD pid;
    LPCSTR windowName = "The Battle for Wesnoth - 1.16.11";
    const wchar_t* mainModule = L"wesnoth.exe";

    HWND hwnd = FindWindowA(0, windowName);

    if (hwnd == NULL) {
        cout << "Window not found." << endl;
        return 1;
    }

    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

    if (pHandle == NULL) {
        cout << "Failed to open process." << endl;
        return 1;
    }

    uintptr_t wesnothMainModule = GetModulesBaseAddress(mainModule, pid);

    if (wesnothMainModule == 0) {
        cout << "Failed to get module base address." << endl;
        return 1;
    }

    cout << "Base Address: 0x" << hex << wesnothMainModule << endl;

    uintptr_t goldAddress = wesnothMainModule + 0x01523880;
    vector<unsigned int> goldOffsets = { 0x8, 0x8 };
    int goldValue = 0; // Variável goldValue declarada e inicializada

    uintptr_t goldDMA = findDMAAddy(pHandle, goldAddress, goldOffsets);

    while (true) {
        if (!ReadProcessMemory(pHandle, reinterpret_cast<BYTE*>(goldDMA), &goldValue, sizeof(goldValue), 0)) {
            cout << "Failed to read memory." << endl;
            break;
        }

        cout << "Gold Value: " << dec << goldValue << endl;
        Sleep(3000);
    }

    CloseHandle(pHandle);
    return 0;
}
