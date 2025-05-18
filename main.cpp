#include <Windows.h>
#include <iostream>
#include <string>
#include <TlHelp32.h>

// Функция для поиска процесса по имени
DWORD GetProcessIdByName(const char* processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        
        if (Process32First(snapshot, &processEntry)) {
            do {
                if (_stricmp(processEntry.szExeFile, processName) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
    }
    
    CloseHandle(snapshot);
    return processId;
}

// Функция для инжекции DLL в процесс
bool InjectDLL(DWORD processId, const char* dllPath) {
    bool success = false;
    
    // Получаем хэндл процесса
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Не удалось открыть процесс. Ошибка: " << GetLastError() << std::endl;
        return false;
    }
    
    // Выделяем память в целевом процессе для пути к DLL
    LPVOID dllPathAddress = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (dllPathAddress == NULL) {
        std::cerr << "Не удалось выделить память в процессе. Ошибка: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }
    
    // Записываем путь к DLL в выделенную память
    if (!WriteProcessMemory(hProcess, dllPathAddress, dllPath, strlen(dllPath) + 1, NULL)) {
        std::cerr << "Не удалось записать путь к DLL в процесс. Ошибка: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Получаем адрес функции LoadLibraryA
    LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (loadLibraryAddress == NULL) {
        std::cerr << "Не удалось получить адрес LoadLibraryA. Ошибка: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Создаем удаленный поток, который вызовет LoadLibraryA с нашим путем к DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, dllPathAddress, 0, NULL);
    if (hThread == NULL) {
        std::cerr << "Не удалось создать удаленный поток. Ошибка: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Ждем завершения потока
    WaitForSingleObject(hThread, INFINITE);
    
    // Проверяем результат выполнения
    DWORD exitCode = 0;
    if (GetExitCodeThread(hThread, &exitCode) && exitCode != 0) {
        success = true;
        std::cout << "DLL успешно инжектирована!" << std::endl;
    } else {
        std::cerr << "Не удалось инжектировать DLL. Ошибка: " << GetLastError() << std::endl;
    }
    
    // Освобождаем ресурсы
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    
    return success;
}

int main(int argc, char* argv[]) {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    
    std::cout << "=== Инжектор DLL для Geometry Dash Megahack ===" << std::endl;
    
    if (argc < 2) {
        std::cout << "Использование: " << argv[0] << " <путь_к_dll>" << std::endl;
        std::cout << "Пример: " << argv[0] << " C:\\path\\to\\GreenIndicator.dll" << std::endl;
        return 1;
    }
    
    const char* dllPath = argv[1];
    
    // Проверяем существование файла DLL
    DWORD fileAttributes = GetFileAttributesA(dllPath);
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "Ошибка: Файл DLL не найден: " << dllPath << std::endl;
        return 1;
    }
    
    std::cout << "Поиск процесса Geometry Dash..." << std::endl;
    
    // Пытаемся найти процесс Geometry Dash
    DWORD processId = GetProcessIdByName("GeometryDash.exe");
    if (processId == 0) {
        std::cerr << "Процесс Geometry Dash не найден. Убедитесь, что игра запущена." << std::endl;
        return 1;
    }
    
    std::cout << "Процесс Geometry Dash найден (PID: " << processId << ")" << std::endl;
    std::cout << "Инжектирование DLL: " << dllPath << std::endl;
    
    // Инжектируем DLL
    if (InjectDLL(processId, dllPath)) {
        std::cout << "Инжекция успешно выполнена!" << std::endl;
    } else {
        std::cerr << "Ошибка при инжекции DLL." << std::endl;
        return 1;
    }
    
    std::cout << "Нажмите любую клавишу для выхода..." << std::endl;
    std::cin.get();
    
    return 0;
} 