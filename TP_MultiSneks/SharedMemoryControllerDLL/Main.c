#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<tchar.h>
#include<io.h>
#include<fcntl.h>
#include<time.h>

#include "Main.h"
/*
HANDLE hClientReadComplete;
HANDLE hSharedView;
HANDLE hSharedBuff;
DWORD dwClientNumber;

#pragma data_seg(".serverData")
HANDLE hClientRegisterEvent = NULL;
LPHANDLE lphClientReadsCompletedEvents = NULL;
DWORD dwClientsReadEventCount = 0;
DWORD dwClientsCount = 0;
BOOL bServerPresent = FALSE;
//#pragma data_seg()

//#pragma data_seg(".commonData")
HANDLE hServerUpdateEvent = NULL;
TCHAR sharedViewPath[256] = TEXT("ViewMemory");
TCHAR sharedBufferPath[256] = TEXT("BufferMemory");
HANDLE hEmptySemaphore = NULL;
HANDLE hFullSemaphore = NULL;
SYSTEM_INFO Si;
//#pragma data_seg()

//#pragma data_seg(".configData")
int BoardLines = -1;
int BoardCols = -1;
int PlayerCount = -1;
BOOL GameReady = -1;
//#pragma data_seg()

//#pragma comment(linker,"/SECTION:.serverData,RWS")
//#pragma comment(linker,"/SECTION:.commonData,RWS")
//#pragma comment(linker,"/SECTION:.configData,RWS")

extern BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_  LPVOID lpvReserved) {
	GetSystemInfo(&Si);
	return TRUE;
}

extern BOOL __stdcall ClientRegister(_Outptr_ LPTCH* lptchBuffPointer) {
	if (lptchBuffPointer == NULL) return FALSE;
	if (!bServerPresent) return FALSE;
	lphClientReadsCompletedEvents = realloc(lphClientReadsCompletedEvents, (dwClientsReadEventCount+1) * sizeof(HANDLE));
	lphClientReadsCompletedEvents[dwClientsReadEventCount] = CreateEvent(NULL, TRUE, FALSE, NULL);
	hClientReadComplete = lphClientReadsCompletedEvents[dwClientsReadEventCount];
	dwClientsReadEventCount++;

	hSharedView = OpenFileMapping(GENERIC_READ, FALSE, sharedViewPath);
	hSharedBuff = OpenFileMapping(GENERIC_WRITE, FALSE, sharedBufferPath);

	*lptchBuffPointer = MapViewOfFile(hSharedBuff, FILE_MAP_WRITE, 0, 0, Si.dwAllocationGranularity);
	dwClientNumber = ++dwClientsCount;
	SetEvent(hClientRegisterEvent);
	return TRUE;
}

extern BOOL __stdcall GetGameViewPointer(_Outptr_ LPTCH* lptchViewPointer) {
	if (lptchViewPointer == NULL) return FALSE;

	if (GameReady) {
		*lptchViewPointer = MapViewOfFile(hSharedView, FILE_MAP_READ, 0, 0, BoardLines * BoardCols * sizeof(TCHAR));
	}else {
		return FALSE;
		*lptchViewPointer = NULL;
	}
}

extern BOOL __stdcall ServerStart(_Outptr_ LPTCH* lptchBuffPointer) {
	if (lptchBuffPointer == NULL) return FALSE;
	if (bServerPresent) {return FALSE;}
	else {
		bServerPresent = TRUE;
		hClientRegisterEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		hServerUpdateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hServerUpdateEvent == NULL || hServerUpdateEvent == INVALID_HANDLE_VALUE) return FALSE;
		hSharedBuff = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Si.dwAllocationGranularity, sharedBufferPath);
		if (hSharedBuff == NULL || hSharedBuff == INVALID_HANDLE_VALUE) return FALSE;
		*lptchBuffPointer = MapViewOfFile(hSharedBuff, FILE_MAP_READ, 0, 0, Si.dwAllocationGranularity);
		if (*lptchBuffPointer == NULL)
			return FALSE;
		else
			return TRUE;
	}
}

extern void __stdcall getServerData(LPHANDLE* lph, DWORD* dwClientsEventCount, DWORD* dwClients) {
	*dwClients = dwClientsCount;
	*dwClientsEventCount = dwClientsReadEventCount;
	*lph = lphClientReadsCompletedEvents;
}

extern void __stdcall getCommonData(HANDLE* hServerEvent, HANDLE* hES, HANDLE* hFS, SYSTEM_INFO* pSi, HANDLE* hRJ) {
	*hServerEvent = hServerUpdateEvent;
	*hES = hEmptySemaphore;
	*hFS = hFullSemaphore;
	*pSi = Si;
	*hRJ = hClientRegisterEvent;
}

extern void __stdcall getConfigData(int* BL, int* BC, int* PC) {
	*BL = BoardLines;
	*BC = BoardCols;
	*PC = PlayerCount;
}*/

//TODO :	ServerStart
//			ServerCopyView
//			ServerClose
//			ClientJoin
//			ClientClose



//VERSION 2 of the DLL starts here

/*	NEED NULL VERIFICATIONS ON EVERYTHING !!	*/

#pragma data_seg(".permanentvars")
//if the static doesn't work, switch to data_seg's
//it doesen't...
//only the DLL uses these anyway
//LPT : if the variables are uninitialized here, they will not carry their value between "sessions"
//oh god
static int ClientsCount=0;
static int MaxClients=16;//and even sixteen snakes may be too much
static SYSTEM_INFO Si={ NULL };
static BOOL FirstInitDone=FALSE;
static BOOL ServerOn=FALSE;
static HANDLE ClientEventsSHM=NULL;
static int ReadIndex=0;
static int WriteIndex=0;
#pragma data_seg()
#pragma comment(linker,"/SECTION:.permanentvars,RWS")

TCHAR* CommandBufferFileName=_T ("C:\\SO2\\CBFN.txt");
TCHAR* GameViewBufferFileName=_T ("C:\\SO2\\GVBFN.txt");
TCHAR* ClientEventFileName=_T ("C:\\SO2\\CEFN.txt");
TCHAR* ConfigFileName=_T ("C:\\SO2\\CFN.txt");

TCHAR* CommandBufferString=_T ("ServerCommandBufferSharedMemory");
TCHAR* GameViewBufferString=_T ("GameViewBufferSharedMemory");
TCHAR* ClientEventsSHMName=_T ("ClientEventsSHM");
TCHAR* ConfigSHMName=_T ("ConfigStructSHM");
int maxMessageCountInBuffer=256;
int maxMessageSize;
BOOL isServer=FALSE;

TCHAR* CommandBufferEmptySemaphoreName=_T ("CommandBufferES");
TCHAR* CommandBufferFullSemaphoreName=_T ("CommandBufferFS");
TCHAR* ServerUpdatedViewEventName=_T ("ViewUpdatedEvent");
TCHAR* ClientMutexName=_T ("ClientsMutex");
TCHAR* GameStartedEvent=_T ("GameStartedEvent");

int ClientNumber;
TCHAR EventName[256];

TCHAR* ClientReadEventBaseString=_T ("CLIENTREADCOMPLETEDEVENT");

extern BOOL __stdcall DLLInit (int TOKEN) {
	if (!FirstInitDone) {
		GetSystemInfo (&Si);
		HANDLE hCEFN=CreateFile (ClientEventFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
		ClientEventsSHM=CreateFileMapping (hCEFN, NULL, PAGE_READWRITE, 0, Si.dwAllocationGranularity * MaxClients * sizeof (TCHAR), ClientEventsSHMName);
		if (ClientEventsSHM == NULL) {
			return FALSE;
		}
		FirstInitDone=TRUE;
	}

	maxMessageSize=Si.dwAllocationGranularity;

	if (TOKEN == SERVER_START_TOKEN) {//what to do here....
		if (ServerOn) return FALSE;
		HANDLE h=CreateFile (CommandBufferFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
		CreateFileMapping (h, NULL, PAGE_READWRITE, 0, maxMessageCountInBuffer * maxMessageSize * sizeof (char), CommandBufferString);
		CreateSemaphore (NULL, maxMessageCountInBuffer, maxMessageCountInBuffer, CommandBufferEmptySemaphoreName);
		CreateSemaphore (NULL, 0, maxMessageCountInBuffer, CommandBufferFullSemaphoreName);
		CreateEvent (NULL, TRUE, FALSE, ServerUpdatedViewEventName);
		CreateEvent (NULL, TRUE, FALSE, GameStartedEvent);
		ServerOn=TRUE;
		return TRUE;
	} else if (TOKEN == CLIENT_LOCAL_TOKEN) {
		if (!ServerOn) return FALSE;
		HANDLE ClientsMutex=CreateMutex (NULL, FALSE, ClientMutexName);
		if (WaitForSingleObject (ClientsMutex, INFINITE) == WAIT_OBJECT_0) {

			ClientNumber=++ClientsCount;

			TCHAR* EventNameString=malloc ((sizeof (TCHAR) * Si.dwAllocationGranularity));

			TCHAR Buff2[8];
			_itot (ClientNumber, Buff2, 10);
			_tcscpy (EventName, ClientReadEventBaseString);
			_tcscat (EventName, Buff2);
			HANDLE hshm=OpenFileMapping (FILE_MAP_ALL_ACCESS, FALSE, ClientEventsSHMName);
			EventNameString=MapViewOfFile (hshm, FILE_MAP_ALL_ACCESS, 0, (sizeof (TCHAR) * Si.dwAllocationGranularity)*(ClientNumber - 1), sizeof (TCHAR) * Si.dwAllocationGranularity);
			if (EventNameString == NULL) {
				return FALSE;
			}
			memcpy (EventNameString, EventName, _tcslen (EventName) * sizeof (TCHAR));

			CreateEvent (NULL, FALSE, FALSE, EventNameString);

			TCHAR Buff3[128];
			_tcscpy (Buff3, _T ("HELLO FROM CLIENT "));
			_tcscat (Buff3, Buff2);
			ReleaseMutex (ClientsMutex);
			return CommandWrite (Buff3);
		} else {
			return FALSE;
		}
	} else {//INVALID TOKEN
		return FALSE;
	}
}

extern BOOL __stdcall BroadcastUpdate () {//The server calls this after processing a game "tick"
	//missing some pre-flight checks 
	HANDLE ClientsMutex=CreateMutex (NULL, FALSE, ClientMutexName);//The mutex may get destroyed between calls
	if (WaitForSingleObject (ClientsMutex, INFINITE) == WAIT_OBJECT_0) {
		HANDLE ev=OpenEvent (EVENT_ALL_ACCESS, FALSE, ServerUpdatedViewEventName);
		HANDLE* clientEvs=malloc (sizeof (HANDLE) * ClientsCount);
		TCHAR* ClientString=malloc (sizeof (TCHAR) * Si.dwAllocationGranularity);
		for (int i=0; i < ClientsCount; ++i) {
			ClientString=MapViewOfFile (ClientEventsSHM, FILE_MAP_READ, 0, (sizeof (TCHAR) * Si.dwAllocationGranularity)*i, sizeof (TCHAR) * Si.dwAllocationGranularity);
			clientEvs[i]=OpenEvent (EVENT_ALL_ACCESS, FALSE, ClientString);
			//unmap
		}
		SetEvent (ev);
		BOOL rval=(WaitForMultipleObjects (ClientsCount, clientEvs, TRUE, 10000) == WAIT_OBJECT_0);//Not gonna wait forever
		ResetEvent (ev);
		ReleaseMutex (ClientsMutex);
		return rval;
	} else {
		return FALSE;
	}
}

extern BOOL __stdcall CommandWrite (TCHAR* Message) {
	//Wait on empty, release on full
	HANDLE FullSemaphore=OpenSemaphore (GENERIC_ALL, FALSE, CommandBufferFullSemaphoreName);
	HANDLE EmptySemaphore=OpenSemaphore (GENERIC_ALL, FALSE, CommandBufferEmptySemaphoreName);
	if (WaitForSingleObject (EmptySemaphore, INFINITE) == WAIT_OBJECT_0) {
		HANDLE shm=OpenFileMapping (FILE_MAP_ALL_ACCESS, FALSE, CommandBufferString);
		TCHAR* MessageBuff=malloc (maxMessageSize);
		MessageBuff=MapViewOfFile (shm, FILE_MAP_WRITE, 0, maxMessageSize * WriteIndex, maxMessageSize);
		memcpy (MessageBuff, Message, sizeof (TCHAR) * _tcsclen (Message));
		ReleaseSemaphore (FullSemaphore, 1, NULL);
		WriteIndex=(WriteIndex + 1) % maxMessageCountInBuffer;
	}
	return TRUE;
}

extern BOOL __stdcall CommandRead (TCHAR** Output) {
	//Black magic here
	//Wait on full, release on empty
	HANDLE FullSemaphore=OpenSemaphore (GENERIC_ALL, FALSE, CommandBufferFullSemaphoreName);
	HANDLE EmptySemaphore=OpenSemaphore (GENERIC_ALL, FALSE, CommandBufferEmptySemaphoreName);
	if (WaitForSingleObject (FullSemaphore, INFINITE) == WAIT_OBJECT_0) {
		HANDLE shm=OpenFileMapping (FILE_MAP_ALL_ACCESS, FALSE, CommandBufferString);
		TCHAR* MessageBuff=malloc (maxMessageSize);
		MessageBuff=MapViewOfFile (shm, FILE_MAP_READ, 0, maxMessageSize * ReadIndex, maxMessageSize);
		memcpy (*Output, MessageBuff, maxMessageSize);
		ReleaseSemaphore (EmptySemaphore, 1, NULL);
		ReadIndex=(ReadIndex + 1) % maxMessageCountInBuffer;
		return TRUE;
	} else {
		return FALSE;
	}
}

extern int __stdcall getMessageSize () {
	return maxMessageSize;
}

extern BOOL __stdcall SignalStart () {
	SetEvent (OpenEvent (EVENT_ALL_ACCESS, FALSE,GameStartedEvent));
}

//client stuff
extern BOOL __stdcall WaitForServerUpdate (/*short int** UpdatedView*/) {
	HANDLE ServerEvent=OpenEvent (EVENT_ALL_ACCESS, FALSE, ServerUpdatedViewEventName);
	HANDLE MyEvent=OpenEvent (EVENT_ALL_ACCESS, FALSE, EventName);

	if (WaitForSingleObject (ServerEvent, INFINITE) == WAIT_OBJECT_0) {
		//UpdatedView = ReadFromSharedMemoryViewPart
		SetEvent (MyEvent);
		return TRUE;
	} else {
		return FALSE;
	}
}