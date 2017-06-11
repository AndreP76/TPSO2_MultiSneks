#pragma once

#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<tchar.h>

/*
__declspec(dllexport) BOOL __stdcall ClientRegister(_Outptr_ LPTCH* lptchBuffPointer);
__declspec(dllexport) BOOL __stdcall GetGameViewPointer(_Outptr_ LPTCH* lptchViewPointer);
__declspec(dllexport) BOOL __stdcall ServerStart(_Outptr_ LPTCH* lptchBuffPointer);
__declspec(dllexport) void __stdcall getServerData(LPHANDLE* lph, DWORD* dwClientsEventCount, DWORD* dwClients);
__declspec(dllexport) void __stdcall getCommonData(HANDLE* hServerEvent, HANDLE* hES, HANDLE* hFS, SYSTEM_INFO* pSi, HANDLE*);
__declspec(dllexport) void __stdcall getConfigData(int* BL, int* BC, int* PC);
/*


BOOL (*ClientRegister)(_Outptr_ LPTCH* lptchBuffPointer);
BOOL (*GetGameViewPointer)(_Outptr_ LPTCH* lptchViewPointer);
BOOL (*ServerStart)(_Outptr_ LPTCH* lptchBuffPointer);
void (*getServerData)(LPHANDLE* lph, DWORD* dwClientsEventCount, DWORD* dwClients);
void (*getCommonData)(HANDLE* hServerEvent, HANDLE* hES, HANDLE* hFS, SYSTEM_INFO* pSi, HANDLE*);
void (*getConfigData)(int* BL, int* BC, int* PC);



*/

//VERSION 2 OF THE DLL STARTS HERE
extern const int SERVER_START_TOKEN = 1;
extern const int CLIENT_LOCAL_TOKEN = 2;

__declspec(dllexport) BOOL __stdcall DLLInit(int TOKEN);
__declspec(dllexport) BOOL __stdcall BroadcastUpdate();
__declspec(dllexport) BOOL __stdcall CommandWrite(TCHAR*);
__declspec(dllexport) BOOL __stdcall CommandRead(TCHAR**);
__declspec(dllexport) int __stdcall getMessageSize();
__declspec(dllexport) BOOL __stdcall WaitForServerUpdate();

__declspec(dllexport) TCHAR* CommandBufferString;
__declspec(dllexport) TCHAR* GameViewBufferString;
__declspec(dllexport) int maxMessageCountInBuffer;
__declspec(dllexport) int maxMessageSize;
__declspec(dllexport) BOOL isServer;

__declspec(dllexport) TCHAR* CommandBufferEmptySemaphoreName;
__declspec(dllexport) TCHAR* CommandBufferFullSemaphoreName;
__declspec(dllexport) TCHAR* ServerUpdatedViewEventName;
__declspec(dllexport) TCHAR* ClientMutexName;
__declspec(dllexport) int ClientNumber;

/*

BOOL (*DLLInit)(int TOKEN);
BOOL (*BroadcastUpdate)();
BOOL (*CommandWrite)(TCHAR*);
BOOL (*CommandRead)(TCHAR**);


*/