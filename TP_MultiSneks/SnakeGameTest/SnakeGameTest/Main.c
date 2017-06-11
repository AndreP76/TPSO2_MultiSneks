#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<tchar.h>
#include<io.h>
#include<fcntl.h>
#include<time.h>

TCHAR* ModelUpdateEvent=_T ("ModelUpdated");


#define GAME_MIN_ROWS 10
#define GAME_MIN_COLS 10
#define GAME_MAX_ROWS 80
#define GAME_MAX_COLS 40

#define FOOD 3
#define POISON 4
#define EMPTY 0
#define GRENADE 5
#define SNAKE_BODY 1
#define SNAKE_HEAD 2
#define VODKA 6
#define OIL 7
#define GLUE 8
#define OOIL 9
#define OGLUE 10

typedef struct _msg {
	char command[64];
	char params[64];
	DWORD ProcessID;
	DWORD PlayerID;
};
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
	struct _snek* PlayerSnake;
	BOOL LocalClient;
}Player;

typedef struct _coordinate {
	int Row;
	int Col;
	char Direction;
}Coordinate;

typedef struct _snek {
	DWORD OwnerPID;
	DWORD PlayerID;
	Coordinate* SnakeBody;
	int SnakeLength;
	Player* Owner;
	int TicksToMove;
	int MovementState; //0 = normal, 1 = Fast, 2 = Slow, 3 = Vodka
	int MovementDirection; // N S E W

	BOOL DrunkOnVodka;
	BOOL StuckOnGlue;
	BOOL OilSpeedUp;
	BOOL MoveFlag;
}Snake;

Snake newSnake (int MovementState, Player* Owner, int SnakeLength, int MovementDirection) {
	Snake S;
	S.MovementState=MovementState;
	S.Owner=Owner;
	S.SnakeLength=SnakeLength;
	S.SnakeBody=malloc (sizeof (Coordinate) * S.SnakeLength);
	S.TicksToMove=50;
	S.MovementDirection=MovementDirection;
	return S;
}

int GrowSnake (Snake* S) {
	S->SnakeLength++;
	S->SnakeBody=realloc (S->SnakeBody, S->SnakeLength * sizeof (S->SnakeBody[0]));	 //set the new size

	for (int i=S->SnakeLength - 1; i > 0; --i) {
		S->SnakeBody[i].Col=S->SnakeBody[i - 1].Col;
		S->SnakeBody[i].Row=S->SnakeBody[i - 1].Row;
		S->SnakeBody[i].Direction=S->SnakeBody[i - 1].Direction;
	}//move all parts one position forward, since we are adding at the end of the snake

	if (S->SnakeBody[1].Direction == 'N') {//snake is going north, so, add south
		S->SnakeBody[0].Col=S->SnakeBody[1].Col;
		S->SnakeBody[0].Row=S->SnakeBody[1].Row + 1;
	} else if (S->SnakeBody[1].Direction == 'S') {//snake is going south, so, add north
		S->SnakeBody[0].Col=S->SnakeBody[1].Col;
		S->SnakeBody[0].Row=S->SnakeBody[1].Row - 1;
	} else if (S->SnakeBody[1].Direction == 'E') {//snake is going east, so, add west
		S->SnakeBody[0].Col=S->SnakeBody[1].Col - 1;
		S->SnakeBody[0].Row=S->SnakeBody[1].Row;
	} else if (S->SnakeBody[1].Direction == 'W') {//snake is going west, so, add east
		S->SnakeBody[0].Col=S->SnakeBody[1].Col + 1;
		S->SnakeBody[0].Row=S->SnakeBody[1].Row;
	}

	S->SnakeBody[0].Direction=S->SnakeBody[1].Direction;
	return S->SnakeLength;
}

int ShrinkSnake (Snake* S) {
	if (S->SnakeLength <= 2) return 0;
	for (int i=0; i > S->SnakeLength; ++i) {
		S->SnakeBody[i].Col=S->SnakeBody[i + 1].Col;
		S->SnakeBody[i].Row=S->SnakeBody[i + 1].Row;
		S->SnakeBody[i].Direction=S->SnakeBody[i + 1].Direction;
	}//move all parts one position back, since we are removing the end of the snake

	S->SnakeBody=realloc (S->SnakeBody, sizeof (Coordinate) * S->SnakeLength - 1);
	S->SnakeLength--;
	return S->SnakeLength;
}

int MoveSnake (Snake* S, Map *GM) {
	char *DirectionString=calloc ((S->SnakeLength + 1), sizeof (char));

	for (int i=0; i < S->SnakeLength; ++i) {
		DirectionString[i]=S->SnakeBody[i].Direction;
	}
	DirectionString[S->SnakeLength]=S->MovementDirection;

	for (int i=0; i < S->SnakeLength; ++i) {
		S->SnakeBody[i].Direction=DirectionString[i + 1];
		if (S->SnakeBody[i].Direction == 'N') {
			if (S->SnakeBody[i].Row - 1 < 0) {
				S->SnakeBody[i].Row=GM->MapLines - 1;
			} else { S->SnakeBody[i].Row--; }
		} else if (S->SnakeBody[i].Direction == 'S') {
			if (S->SnakeBody[i].Row + 1 >= GM->MapLines) {
				S->SnakeBody[i].Row=0;
			} else { S->SnakeBody[i].Row++; }
		} else if (S->SnakeBody[i].Direction == 'E') {
			if (S->SnakeBody[i].Col + 1 >= GM->MapCols) {
				S->SnakeBody[i].Col=0;
			} else { S->SnakeBody[i].Col++; }
		} else if (S->SnakeBody[i].Direction == 'W') {
			if (S->SnakeBody[i].Col - 1 < 0) {
				S->SnakeBody[i].Col=GM->MapCols - 1;
			} else { S->SnakeBody[i].Col--; }
		}
		if (GM->Board[S->SnakeBody[i].Row][S->SnakeBody[i].Col] == FOOD)//food here
			GrowSnake (S);
		else if (GM->Board[S->SnakeBody[i].Row][S->SnakeBody[i].Col] == POISON) {
			ShrinkSnake (S);
		} else if (GM->Board[S->SnakeBody[i].Row][S->SnakeBody[i].Col] == GRENADE) {
			KillSnake (S);
		} else if (GM->Board[S->SnakeBody[i].Row][S->SnakeBody[i].Col] == VODKA) {
			S->DrunkOnVodka=TRUE;
		}
	}

	free (DirectionString);
}
BOOL WINAPI SetGameEvent (LPVOID p) {
	HANDLE H=OpenEvent (EVENT_ALL_ACCESS, FALSE, _T ("GameTimer"));
	SetEvent (H);
	return TRUE;
}

BOOL WINAPI RenderThread (LPVOID p) {
	Map GameMap=*(Map*)p;
	HANDLE GameTimerEvent=OpenEvent (EVENT_ALL_ACCESS, FALSE, _T ("GameTimer"));
	HANDLE StdinHandle=GetStdHandle (STD_INPUT_HANDLE);
	HANDLE UpdateEventHandle=OpenEvent (EVENT_ALL_ACCESS, FALSE, _T ("Update"));
	while (TRUE) {
		DWORD D=WaitForSingleObject (GameTimerEvent, 100);
		if (D == WAIT_TIMEOUT) {
			WaitForSingleObject (UpdateEventHandle, INFINITE);
			system ("cls");
			for (int i=0; i < GameMap.MapLines; ++i) {
				for (int j=0; j < GameMap.MapCols; ++j) {
					if (GameMap.Board[i][j] == SNAKE_BODY)//1 stands for snake body
						_tprintf (_T ("x"));
					else if (GameMap.Board[i][j] == SNAKE_HEAD)//2 stands for snake head
						_tprintf (_T ("o"));
					else if (GameMap.Board[i][j] == FOOD)//3 is food
						_tprintf (_T ("f"));
					else if ((GameMap.Board[i][j] == POISON))//4 is poison
						_tprintf (_T ("p"));
					else if ((GameMap.Board[i][j] == GRENADE))//5 is grenade
						_tprintf (_T ("g"));
					else if ((GameMap.Board[i][j] == VODKA))//6 is vodka
						_tprintf (_T ("v"));
					else
						_tprintf (_T (" "));
				}
				_tprintf (_T ("\n"));
			}
		} else {
			break;
		}
	}
}

int BuildMap (Snake* Snakes, int SnakeCount, Map* GameMap) {
	for (int i=0; i < GameMap->MapLines; ++i) {
		for (int j=0; j < GameMap->MapCols; ++j) {
			GameMap->Board[i][j]=0;
		}
	}

	for (int i=0; i < SnakeCount; ++i) {
		for (int j=0; j < Snakes[i].SnakeLength; ++j) {
			if (j == Snakes[i].SnakeLength - 1) //the head
				GameMap->Board[Snakes[i].SnakeBody[j].Row][Snakes[i].SnakeBody[j].Col]=2;
			else
				GameMap->Board[Snakes[i].SnakeBody[j].Row][Snakes[i].SnakeBody[j].Col]=1;
		}
	}
}

int main (void) {
	Map GameMap;
	GameConfig GCfg;
	Time GameDuration;
	Player P;
	strcpy (P.ClientName, "TEST USER");
	P.LocalClient=TRUE;

	_tprintf (_T ("How many lines : "));
	_tscanf (_T (" %d"), &GCfg.GameLines);

	_tprintf (_T ("How many columns : "));
	_tscanf (_T (" %d"), &GCfg.GameCols);

	_tprintf (_T ("How many AI snakes : "));
	_tscanf (_T (" %d"), &GCfg.AIPlayers);

	_tprintf (_T ("How long (hh-mm-ss) : "));
	_tscanf (_T (" %d-%d-%d"), &GameDuration.Hours, &GameDuration.Minutes, &GameDuration.Seconds);

	GameMap.MapCols=GCfg.GameCols;
	GameMap.MapLines=GCfg.GameLines;

	GameMap.Board=calloc (GameMap.MapLines, sizeof (short int*));
	for (int i=0; i < GameMap.MapLines; ++i)
		GameMap.Board[i]=calloc (GameMap.MapCols, sizeof (short int));
	_tprintf (_T ("Board created!"));

	Snake S=newSnake (0, &P, 3, 'S');
	S.SnakeBody[0].Col=0;
	S.SnakeBody[1].Col=0;
	S.SnakeBody[2].Col=0;
	S.SnakeBody[0].Row=0;
	S.SnakeBody[1].Row=1;
	S.SnakeBody[2].Row=2;
	S.SnakeBody[0].Direction='S';
	S.SnakeBody[1].Direction='S';
	S.SnakeBody[2].Direction='S';

	struct _msg* messageBuffer;
	int maxMessagesInBuffer=64;
	messageBuffer=calloc (maxMessagesInBuffer, sizeof (struct _msg));

	int MillisecondsInSection=1000;
	HANDLE GameTimer=CreateWaitableTimer (NULL, TRUE, NULL);
	HANDLE GameTimerEvent=CreateEvent (NULL, TRUE, FALSE, _T ("GameTimer"));
	LARGE_INTEGER LI;
	LI.LowPart=(GameDuration.Seconds + (GameDuration.Minutes * 60) + (GameDuration.Hours * 3600)) * -1 * MillisecondsInSection;
	if (!SetWaitableTimer (GameTimer, &LI, 0, &SetGameEvent, NULL, FALSE)) {
		_tprintf (_T ("SetWaitableTimer failed... Error : %d"), GetLastError ());
	}
	HANDLE UpdateEvent=CreateEvent (NULL, FALSE, FALSE, _T ("Update"));
	HANDLE RT=CreateThread (NULL, 0, &RenderThread, &GameMap, 0, NULL);

	while (WaitForSingleObject (GameTimerEvent, 100) == WAIT_TIMEOUT) {
		//Processing message queue
		for (int i=0; i < maxMessagesInBuffer; ++i) {
			if (strcmp (messageBuffer[i].command, "SETDIR") == 0) {
				Snake** PIDSnakes=GetSnakesByPID (&S, 1, messageBuffer[i].ProcessID);
				if (strcmp (messageBuffer[i].params, "N") == 0) {
					if (!PIDSnakes[messageBuffer[i].PlayerID]->DrunkOnVodka) {
						if (PIDSnakes[messageBuffer[i].PlayerID]->MovementDirection != 'S') {
							S.MovementDirection='N';
						}
					} else {
						if (PIDSnakes[messageBuffer[i].PlayerID]->MovementDirection != 'N') {
							S.MovementDirection='S';
						}
					}
				} else if (strcmp (messageBuffer[i].params, "S") == 0) {
					if (!PIDSnakes[messageBuffer[i].PlayerID]->DrunkOnVodka) {
						if (PIDSnakes[messageBuffer[i].PlayerID]->MovementDirection != 'N') {
							S.MovementDirection='S';
						}
					} else {
						if (PIDSnakes[messageBuffer[i].PlayerID]->MovementDirection != 'S') {
							S.MovementDirection='N';
						}
					}
				} else if (strcmp (messageBuffer[i].params, "E") == 0) {
					if (!PIDSnakes[messageBuffer[i].PlayerID]->DrunkOnVodka) {
						if (PIDSnakes[messageBuffer[i].PlayerID]->MovementDirection != 'W') {
							S.MovementDirection='E';
						}
					} else {
						if (PIDSnakes[messageBuffer[i].PlayerID]->MovementDirection != 'E') {
							S.MovementDirection='W';
						}
					}
				} else if (strcmp (messageBuffer[i].params, "W") == 0) {
					if (!PIDSnakes[messageBuffer[i].PlayerID]->DrunkOnVodka) {
						if (PIDSnakes[messageBuffer[i].PlayerID]->MovementDirection != 'E') {
							S.MovementDirection='W';
						}
					} else {
						if (PIDSnakes[messageBuffer[i].PlayerID]->MovementDirection != 'W') {
							S.MovementDirection='E';
						}
					}
				}
				free (PIDSnakes);
			}
			if (strcmp (messageBuffer[i].command, "QUIT") == 0) {
				Snake** PIDSnakes=GetSnakesByPID (&S, 1, messageBuffer[i].ProcessID);
				KillSnake (PIDSnakes[messageBuffer[i].PlayerID]);
				free (PIDSnakes);
			}
			memset (messageBuffer + i, 0x0, sizeof (messageBuffer[i]));
		}

		Sleep (500);
		MoveSnake (&S, &GameMap);
		BuildMap (&S, 1, &GameMap);
		SetEvent (UpdateEvent);
		//Debug
		if (kbhit ()) {
			char c=_getch ();
			if (c == 'w' || c == 'W') if (S.MovementDirection != 'S') { S.MovementDirection='N'; }
			if (c == 's' || c == 'S') if (S.MovementDirection != 'N') { S.MovementDirection='S'; }
			if (c == 'a' || c == 'A') if (S.MovementDirection != 'E') { S.MovementDirection='W'; }
			if (c == 'd' || c == 'D') if (S.MovementDirection != 'W') { S.MovementDirection='E'; }
			if (c == 'g' || c == 'G') GrowSnake (&S);
			if (c == 'b' || c == 'B') ShrinkSnake (&S);
		}
	}

	_tprintf (_T ("Game has ended"));
}