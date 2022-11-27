struct KillInfo {
	char  playerName[10][50];
	char   playerUid[10][20];

	char deadName[50];
	char deadUid[20];

	char weaponName[50];
	int playerIndex;
};

extern int isDebug();
extern int isValidUid(char* uid);
extern void getTimeStr(long t, char* formatYMDHMS, char* output, int outputSize);
extern int exists(const char* path);
extern void writeFile(char* path, char* str);
extern void split(const char* src, const char* pattern, char** outvec, size_t* outsize);
extern void getWordLeft(char* input, char* start, char* output);
extern void getWordRight(char* input, char* start, char* output);
extern void getWordRightRange(char* input,char * startOffset, char* start, char* end, char* output);
extern void getWordRange(char* input, char* start, char* end, char* output);
extern void trim(char* str, char* output, int size);
extern int strtoi(const char* str);


extern int parseKillInfo(char* strIn, struct KillInfo* killInfo);