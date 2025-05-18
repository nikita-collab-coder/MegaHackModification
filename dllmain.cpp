#include <Windows.h>
#include <iostream>
#include <vector>
#include <TlHelp32.h>

// Смещения и паттерны для поиска нужных адресов
// Эти значения могут потребовать корректировки в зависимости от версии Megahack
DWORD indicatorBaseOffset = 0x0; // Это пример, нужно заменить реальным смещением

// Функция для поиска модуля в процессе
DWORD GetModuleBaseAddress(DWORD procId, const char* modName) {
    DWORD modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!_stricmp(modEntry.szModule, modName)) {
                    modBaseAddr = (DWORD)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}

// Функция для поиска адреса индикатора
DWORD FindIndicatorAddress() {
    DWORD procId = 0;
    HWND hwnd = FindWindowA(NULL, "Geometry Dash");
    if (hwnd == NULL) {
        hwnd = FindWindowA("LWJGL", NULL); // Альтернативное имя окна
    }
    
    if (hwnd == NULL) {
        MessageBoxA(NULL, "Не удалось найти окно Geometry Dash", "Ошибка", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    GetWindowThreadProcessId(hwnd, &procId);
    if (!procId) {
        MessageBoxA(NULL, "Не удалось получить ID процесса", "Ошибка", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    DWORD baseAddress = GetModuleBaseAddress(procId, "GeometryDash.exe");
    if (!baseAddress) {
        MessageBoxA(NULL, "Не удалось получить базовый адрес модуля", "Ошибка", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    // Вычисляем адрес индикатора (это пример, нужно заменить реальной логикой)
    return baseAddress + indicatorBaseOffset;
}

// Основная функция для установки зеленого индикатора
void SetGreenIndicator() {
    DWORD indicatorAddress = FindIndicatorAddress();
    if (!indicatorAddress) {
        return;
    }
    
    // Получаем хэндл процесса
    DWORD procId = 0;
    HWND hwnd = FindWindowA(NULL, "Geometry Dash");
    if (hwnd == NULL) {
        hwnd = FindWindowA("LWJGL", NULL);
    }
    
    GetWindowThreadProcessId(hwnd, &procId);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    
    if (hProcess == NULL) {
        MessageBoxA(NULL, "Не удалось открыть процесс", "Ошибка", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Устанавливаем значение для зеленого индикатора
    // Предполагается, что значение 1 соответствует зеленому цвету
    int greenValue = 1;
    WriteProcessMemory(hProcess, (LPVOID)indicatorAddress, &greenValue, sizeof(greenValue), NULL);
    
    CloseHandle(hProcess);
}

// Поток для постоянной установки зеленого индикатора
DWORD WINAPI MainThread(LPVOID lpParam) {
    // Создаем консоль для отладки
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    
    std::cout << "DLL успешно инжектирована!" << std::endl;
    std::cout << "Индикатор чита теперь всегда зеленый" << std::endl;
    
    // Бесконечный цикл для постоянного обновления индикатора
    while (true) {
        SetGreenIndicator();
        Sleep(100); // Обновляем каждые 100 мс
    }
    
    // Код никогда не дойдет сюда, но для полноты:
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Создаем поток при загрузке DLL
        CreateThread(NULL, 0, MainThread, hModule, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
} 