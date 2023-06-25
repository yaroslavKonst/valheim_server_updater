#include "cmd_parse.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct CmdData ParseCmd(int argc, char** argv)
{
	int keyCount = 0;

	for (int i = 1; i < argc; ++i) {
		int prefixLen = strspn(argv[i], "-");

		if (prefixLen > 0) {
			++keyCount;
		}
	}

	struct CmdData cmd;
	cmd.Len = keyCount;
	cmd.Args = malloc(sizeof(struct CmdNode) * keyCount);
	memset(cmd.Args, 0, sizeof(struct CmdNode) * keyCount);

	int keyIndex = 0;

	for (int i = 1; i < argc; ++i) {
		int prefixLen = strspn(argv[i], "-");

		if (prefixLen == 0) {
			printf("Command line warning:\n");
			printf("Argument %s without key ignored.\n", argv[i]);
			continue;
		}

		char* key = malloc(strlen(argv[i] + prefixLen) + 1);
		strcpy(key, argv[i] + prefixLen);

		++i;

		if (i >= argc) {
			--cmd.Len;
			continue;
		}

		char* value = malloc(strlen(argv[i]) + 1);
		strcpy(value, argv[i]);

		cmd.Args[keyIndex].Key = key;
		cmd.Args[keyIndex].Value = value;

		++keyIndex;
	}

	return cmd;
}

void FreeCmd(struct CmdData cmd)
{
	for (int i = 0; i < cmd.Len; ++i) {
		free(cmd.Args[i].Key);
		free(cmd.Args[i].Value);
	}

	free(cmd.Args);
}

void PrintCmd(struct CmdData cmd)
{
	for (int i = 0; i < cmd.Len; ++i) {
		printf(
			"Key: %s, value: %s.\n",
			cmd.Args[i].Key,
			cmd.Args[i].Value);
	}
}

char* GetCmdValue(struct CmdData cmd, const char* key)
{
	for (int i = 0; i < cmd.Len; ++i) {
		if(!strcmp(key, cmd.Args[i].Key)) {
			return cmd.Args[i].Value;
		}
	}

	return NULL;
}
