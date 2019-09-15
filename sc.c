#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "argEval.h"

struct config_t{
	char*   inputFile;
	char*   outputFile;
	int     compress;
	int     useStats;
} _config = {
	NULL,
	NULL,
	0,
	0
};

int extraArgumentFunction(int argc, char** argv);

static ArgumentDefinition_t _argArray[] = {
  {"h", "help",     "shows this screen",                          0, vType_none,    NULL,               NULL},
  {"c", "compress", "enable stream compression",                  0, vType_none,    NULL,               &_config.compress},
  {"i", "inFile",   "specifies an input file (default: stdin)",   1, vType_string,  &_config.inputFile, NULL},
  {"o", "outFile",  "specifies an output file (default: stdout)", 1, vType_string,  &_config.inputFile, NULL},
};

void generateNames();

int main(int argc, char** argv)
{
	argEval_enableHelpWithoutArguments();
	argEval_registerCallbackForExtraArguments(extraArgumentFunction);
	argEval_registerArguments(_argArray, sizeof (_argArray) / sizeof (ArgumentDefinition_t), &_argArray[0]);
	argEval_Parse(argc, argv, stderr);

	if(_config.inputFile == NULL){
		exit (EXIT_FAILURE);
	}
}

int extraArgumentFunction(int argc, char** argv){
	return 0;
}