#include<iostream>
#include<windows.h>
#include <windows.h>
#include<string>

// ����GLOBAL_PARAM�ṹ��
typedef struct _GLOBAL_PARAM {
    PBYTE dwStart;
    PBYTE lpStartVA;
    ULONGLONG dwImageBase;
    DWORD dwOEP;
    DWORD dwCodeSize;
    CHAR pass[30] = { 0 };
    INT passlen = 0;
    DWORD importsize;
    DWORD importrva;
    DWORD Virrva;
    DWORD Virsize;
} GLOBAL_PARAM, * PGLOBAL_PARAM;

// ����ָ������
typedef FARPROC(WINAPI* fnGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
typedef HMODULE(WINAPI* fnLoadLibraryA)(LPCSTR lpLibFileName);
typedef HMODULE(WINAPI* fnGetModuleHandleA)(LPCSTR lpModuleName);
typedef BOOL(WINAPI* fnVirtualProtect)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
typedef int(WINAPI* fnMessageBox)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
typedef VOID(WINAPI* fnExitProcess)(UINT uExitCode);
typedef HANDLE(WINAPI* fnGetStdHandle)(DWORD nStdHandle);
typedef BOOL(WINAPI* fnReadConsoleA)(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, LPDWORD lpNumberOfCharsRead, LPVOID lpReserved);
typedef BOOL(WINAPI* fnWriteConsoleA)(HANDLE hConsoleOutput, const VOID* lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);

// ��������
ULONGLONG GetKernel32Addr();
ULONGLONG MyGetProcAddress();
int mystrcmp(const char* p, const char* q);
void encryptCode();
void Start();
void RecIAT64();