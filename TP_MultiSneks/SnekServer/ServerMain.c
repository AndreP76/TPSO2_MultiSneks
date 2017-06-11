#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<tchar.h>
#include<io.h>
#include<fcntl.h>
#include<time.h>
//#include "../SharedMemoryControllerDLL/Main.h"
#define GAME_MIN_ROWS 10
#define GAME_MIN_COLS 10
#define GAME_MAX_ROWS 80
#define GAME_MAX_COLS 40
typedef struct _time {
	int Hours;
	int Minutes;
	int Seconds;
}Time;
typedef struct _config {
	int GameLines;
	int GameCols;
	int Players;
	int SnakeStartSize;
	int MaxObjectCount;
	Time gameDuration;
	int AIPlayers;

	int FoodChance;
	int PoisonChance;
	int GrenadeChance;
	int VodkaChance;
	int VodkaDuration;
	int OilChance;
	int OilDuration;
	int GlueChance;
	int GlueDuration;
	int OVodkaChance;
	int OOilChance;
	int OGlueChance;
}GameConfig;
typedef struct _Map {
	int MapLines;
	int MapCols;
	short int** Board;
}Map;
typedef struct _player {
	int ClientNumber;
	char ClientName[64];
	Snake* PlayerSnake;
	BOOL LocalClient;
}Player;
typedef struct _coordinate {
	int Row;
	int Col;
	char Direction;
}Coordinate;

typedef struct _snek {
	Coordinate* SnakeBody;
	int SnakeLength;
	Player* Owner;
	int TicksToMove;
	int MovementState; //0 = normal, 1 = Fast, 2 = Slow, 3 = Vodka
}Snake;

BOOL (*DLLInit)(int TOKEN);
BOOL (*BroadcastUpdate)();
BOOL (*CommandWrite)(TCHAR*);
BOOL (*CommandRead)(TCHAR**);
int (*GetMessageSize)();

Player* CurrentPlayers;
int PlayerCount=0;

Snake* CurrentSnakes;
int CurrentSnakes=0;

const int SERVER_START_TOKEN=1;
int messageSize=0;
int messageCount=0;
int maxMessageCount=16;
char** MessageStack;

TCHAR* getErrorAsString (int ErrorCode) {
	TCHAR* messageBuffer=NULL;
	size_t size=FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, ErrorCode, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), (TCHAR*)&messageBuffer, 0, NULL);
	return messageBuffer;
}

BOOL WINAPI ReadCommand (LPVOID param) {
	BOOL (*CommandRead)(TCHAR**)=param;

	TCHAR* Buffer=malloc (messageSize * sizeof (TCHAR));
	_tprintf (_T ("Waiting for commands from SHM...\n"));
	while (CommandRead (&Buffer)) {
		_tprintf (_T ("Got a command from the DLL : %s\n"), Buffer);
	}
}

BOOL WINAPI ClientSnakeHandling () {

}
BOOL WINAPI AISnakeHandling () {

}

BOOL WINAPI GameEngine (LPVOID param) {
	GameConfig GC=*(GameConfig*)param;
	HANDLE StartGameHandle=OpenEvent (EVENT_ALL_ACCESS, FALSE, _T ("StartGameEvent"));
	if (WaitForSingleObject (StartGameHandle, INFINITE) == WAIT_OBJECT_0) {
		Map GameMap;
		GameMap.MapCols=GC.GameCols;
		GameMap.MapLines=GC.GameLines;
		GameMap.Board=malloc (sizeof (short int *) * GameMap.MapLines);
		for (int i=0; i < GameMap.MapLines; ++i) {
			GameMap.Board[i]=malloc (sizeof (short int) * GameMap.MapCols);
		}//Map Created

		CurrentSnakes=malloc (sizeof (CurrentSnakes) * (PlayerCount + GC.AIPlayers));
		HANDLE* PlayerThreads=malloc (sizeof (HANDLE) * PlayerCount);
		HANDLE* AIThreads=malloc (sizeof (HANDLE) * GC.AIPlayers);
		for (int i=0; i < PlayerCount + GC.AIPlayers; ++i) {
			CurrentSnakes[i].MovementState=0;
			CurrentSnakes[i].SnakeLength=GC.SnakeStartSize;
			CurrentSnakes[i].SnakeBody=malloc (sizeof (Coordinate) * CurrentSnakes[i].SnakeLength);

			if (i < PlayerCount) {//snake is from a player
				CurrentSnakes[i].Owner=CurrentPlayers + i;
				CurrentPlayers[i].PlayerSnake=CurrentSnakes + i;
				PlayerThreads=CreateThread (NULL, 0, ClientSnakeHandling, CurrentSnakes + i, 0, NULL);
			} else {
				CurrentSnakes[i].Owner=NULL;
				AIThreads=CreateThread (NULL, 0, AISnakeHandling, CurrentSnakes + i, 0, NULL);
			}
		}
		//Create Sneks

		HANDLE GameDurationTimer=CreateWaitableTimer (NULL, TRUE, NULL);//The end of the game signal
		LARGE_INTEGER RemainingGameTime;
		RemainingGameTime.QuadPart=(GC.gameDuration.Hours * 3600) + (GC.gameDuration.Minutes * 60) + GC.gameDuration.Seconds;
		SetWaitableTimer (GameDurationTimer, &RemainingGameTime, 0, NULL, NULL, FALSE);//timer ready
		while (WaitForSingleObject (GameDurationTimer, 1) != WAIT_OBJECT_0) {//While time still remains
			while (messageCount > 0) {											//Process messages
				ProcessMessage (MessageStack[messageCount - 1]);
				MessageStack=realloc (MessageStack, messageSize * messageCount - 1);	//Wait for message queue mutex
				messageCount--;
			}																	    //Process messages until end reached
																					//empty the message stack

																				//Calculate next game tick
																					//Move sneks
																						//Check active efects (Slow, etc)
																							//If Slow, move every T*4 Ticks
																							//If Fast, move every T Ticks
																							//If Normal, move every T*2 Ticks
																							//If Vodka, move every T*2 Ticks, reverse commands
																								//If Dir = up, Dir = down
																								//If Dir = down, Dir = up
																								//If Dir = left, Dir = right
																								//If Dir = right, Dir = left

		}

	}
}
BOOL LoadFunction () {
	HANDLE hDLL=LoadLibrary (TEXT ("./SharedMemoryControllerDll.dll"));
	if (hDLL == INVALID_HANDLE_VALUE || hDLL == NULL) {
		_tprintf (_T ("Error loading the DLL\nMessage : %s\n"), getErrorAsString (GetLastError ())); return FALSE;
	}

	DLLInit=GetProcAddress (hDLL, "DLLInit");
	if (DLLInit == NULL) {
		_tprintf (_T ("Error aquiring DLLInit pointer!\nMessage : %s"), getErrorAsString (GetLastError ())); return FALSE;
	}

	BroadcastUpdate=GetProcAddress (hDLL, "BroadcastUpdate");
	if (BroadcastUpdate == NULL) {
		_tprintf (_T ("Error aquiring BroadcastUpdate pointer!\nMessage : %s"), getErrorAsString (GetLastError ())); return FALSE;
	}

	CommandWrite=GetProcAddress (hDLL, "CommandWrite");
	if (CommandWrite == NULL) {
		_tprintf (_T ("Error aquiring CommandWrite pointer!\nMessage : %s"), getErrorAsString (GetLastError ())); return FALSE;
	}

	CommandRead=GetProcAddress (hDLL, "CommandRead");
	if (CommandRead == NULL) {
		_tprintf (_T ("Error aquiring CommandRead pointer!\nMessage : %s"), getErrorAsString (GetLastError ())); return FALSE;
	}

	GetMessageSize=GetProcAddress (hDLL, "getMessageSize");
	if (GetMessageSize == NULL) {
		_tprintf (_T ("Error aquiring GetMessageSize pointer!\nMessage : %s"), getErrorAsString (GetLastError ())); return FALSE;
	}
}

int _tmain (int argcount, TCHAR** argvalues) {
	LoadFunction ();
	do {
		if (DLLInit (SERVER_START_TOKEN)) {
			messageSize=getMessageSize ();
			_tprintf (_T ("Server can start, all systems green\n"));
			HANDLE CommandReaderThread=CreateThread (NULL, 0, ReadCommand, (void*)CommandRead, 0, NULL);
			HANDLE GameEngineThread=CreateThread (NULL, 0x00, GameEngine, NULL, 0x00, NULL);
		} else {
			_tprintf (_T ("Error has happened while initializing the DLL\nMessage : %s"), getErrorAsString (GetLastError ()));
		}
	} while (FALSE);
	getchar ();
}