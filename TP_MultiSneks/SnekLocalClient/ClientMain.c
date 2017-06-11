#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<tchar.h>
#include<io.h>
#include<fcntl.h>
#include<time.h>

const int CLIENT_LOCAL_TOKEN = 2;
TCHAR* getErrorAsString(int ErrorCode) {
	TCHAR* messageBuffer = NULL;
	size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (TCHAR*)&messageBuffer, 0, NULL);
	return messageBuffer;
}

int _tmain(int argcount, TCHAR** argvalues) {
	BOOL(*DLLInit)(int TOKEN);
	BOOL(*BroadcastUpdate)();
	BOOL(*CommandWrite)(TCHAR*);
	BOOL(*CommandRead)(TCHAR**);
	int(*getMessageSize)();
	BOOL(*WaitForServerUpdate)();
	do {
		HANDLE hDLL = LoadLibrary(TEXT("./SharedMemoryControllerDll.dll"));
		if (hDLL == INVALID_HANDLE_VALUE || hDLL == NULL) {
			_tprintf(_T("Error loading the DLL\nMessage : %s\n"), getErrorAsString(GetLastError())); break;
		}

		DLLInit = GetProcAddress(hDLL, "DLLInit");
		if (DLLInit == NULL) {
			_tprintf(_T("Error aquiring DLLInit pointer!\nMessage : %s"), getErrorAsString(GetLastError())); break;
		}

		BroadcastUpdate = GetProcAddress(hDLL, "BroadcastUpdate");
		if (BroadcastUpdate == NULL) {
			_tprintf(_T("Error aquiring BroadcastUpdate pointer!\nMessage : %s"), getErrorAsString(GetLastError())); break;
		}

		CommandWrite = GetProcAddress(hDLL, "CommandWrite");
		if (CommandWrite == NULL) {
			_tprintf(_T("Error aquiring CommandWrite pointer!\nMessage : %s"), getErrorAsString(GetLastError())); break;
		}

		CommandRead = GetProcAddress(hDLL, "CommandRead");
		if (CommandRead == NULL) {
			_tprintf(_T("Error aquiring CommandRead pointer!\nMessage : %s"), getErrorAsString(GetLastError())); break;
		}

		WaitForServerUpdate = GetProcAddress(hDLL, "WaitForServerUpdate");
		if (WaitForServerUpdate == NULL) {
			_tprintf(_T("Error aquiring WaitForServerUpdate pointer!\nMessage : %s"), getErrorAsString(GetLastError())); break;
		}
		if (DLLInit(CLIENT_LOCAL_TOKEN)) {
			_tprintf(_T("Client registered in shm and ready!\n"));
			while (WaitForServerUpdate()) {
				_tprintf(_T("Got an update from the server!\n"));
			}
		}else {
			_tprintf(_T("Error initializing the DLL\nMessage : %s"),getErrorAsString(GetLastError()));
		}
	} while (FALSE);
	getchar();
}