#ifndef CMD_PARSE
#define CMD_PARSE

struct CmdNode
{
	char* Key;
	char* Value;
};

struct CmdData
{
	int Len;
	struct CmdNode* Args;
};

struct CmdData ParseCmd(int argc, char** argv);
char* GetCmdValue(struct CmdData cmd, const char* key);
void PrintCmd(struct CmdData cmd);
void FreeCmd(struct CmdData cmd);

#endif
