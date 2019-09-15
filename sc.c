#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "argEval.h"
#include "sc_protocol.h"
#include "sc_zeroEncoder.h"

struct config_t{
	char*   inputFile;
	char*   outputFile;
	int     encode;
	int     useStats;
	int     junkSize;

	int     exit;
} _config = {
	NULL,
	NULL,
	0,
	0,
	1024,

	0
};

int _statistic[256];

sc_encoder_table_t* _new_encode = NULL;
sc_decoder_table_t* _new_decode = NULL;

int extraArgumentFunction(int argc, char** argv);

static ArgumentDefinition_t _argArray[] = {
  {"h", "help",     "shows this screen",                          0, vType_none,    NULL,                NULL},
  {"c", "compress", "enable stream compression",                  0, vType_none,    NULL,                &_config.encode},
  {"i", "inFile",   "specifies an input file (default: stdin)",   1, vType_string,  &_config.inputFile,  NULL},
  {"o", "outFile",  "specifies an output file (default: stdout)", 1, vType_string,  &_config.outputFile, NULL},
};

static int _startEncoding(FILE* in, FILE* out);
static int _startDecoding(FILE* in, FILE* out);

int main(int argc, char** argv)
{
	argEval_enableHelpWithoutArguments();
	argEval_registerCallbackForExtraArguments(extraArgumentFunction);
	argEval_registerArguments(_argArray, sizeof (_argArray) / sizeof (ArgumentDefinition_t), &_argArray[0]);
	argEval_Parse(argc, argv, stderr);

	FILE* in = stdin;
	FILE* out = stdout;
	FILE* err = stderr;

	if(_config.inputFile != NULL){
		in = fopen(_config.inputFile, "rb");
	}

	if(_config.outputFile != NULL){
		out = fopen(_config.outputFile, "wb");
	}

	if(in == NULL || out == NULL){
		fprintf(err, "Error File IO: unable to open input or output file\n");
		if(in == NULL){
			fprintf(err, "error unable to open input: %s\n", _config.inputFile);
		}
		if(out == NULL){
			fprintf(err, "error unable to open output: %s\n", _config.outputFile);
		}
		exit(EXIT_FAILURE);
	}

	if(_config.encode != 0){
		_new_encode = _sc_creatZeroEncoding();
		_new_decode = _sc_creatZeroDecoding();

		_startEncoding(in, out);
	}else{
		_startDecoding(in, out);
	}

	if(_config.inputFile != NULL){
		fclose(in);
	}

	if(_config.outputFile != NULL){
		fclose(out);
	}
}

static void _sendDecoding(FILE* out, sc_decoder_table_t* decode){
	sc_header_t head;
	head.header = SC_HUFFMAN_DECODER;
	head.size = sizeof(sc_decoder_table_t) +(decode->size -1) *sizeof(sc_decoder_entry_t);

	fwrite(&head, sizeof(sc_header_t), 1, out);
	fwrite(decode, head.size, 1, out);
}

static void _sendEncoded(FILE* in, FILE* out, sc_encoder_table_t* encode){
	sc_header_t head;
	head.header = SC_DATA_HEADER;
	head.size = _config.junkSize;

	if(encode->size != 256){
		fprintf(stderr, "Error: encoder table is invalid!\n");
		_config.exit = 1;
		return;
	}

	fwrite(&head, sizeof(sc_header_t), 1, out);

	int maxcount = _config.junkSize;
	int bits = 0;
	uint32_t shift = 0;
	for(int count = 0; count < maxcount && !feof(in); count++){
		int tmp = fgetc(in);
		int bit = encode->entries[tmp].bits;

		shift <<= bit;
		shift |= encode->entries[tmp].pattern;
		bits += bit;

		while(bits >= 8){
			bits -= 8;
			int send = shift >> bits;
			fputc(send & 0x00FF, out);
		}
	}

	if(bits > 0){
		shift <<= 8;
		shift |= 0x00FF;
		bits += 8;

		while(bits >= 8){
			bits -= 8;
			int send = shift >> bits;
			fputc(send & 0x00FF, out);
		}
	}

	if(feof(in)){
		_config.exit = 1;
	}
}

static void _sendExit(FILE* out){
	sc_header_t head;
	head.header = SC_EXIT;
	head.size = 0;
	
	fwrite(&head, sizeof(sc_header_t), 1, out);
}

static int _startEncoding(FILE* in, FILE* out){
	sc_encoder_table_t* encode = NULL;
	sc_decoder_table_t* decode = NULL;
	if(_new_encode == NULL || _new_decode == NULL){
		fprintf(stderr, "Error unable to create encoder and decoder matrix\n");
		return EXIT_FAILURE;
	}

	while(_config.exit == 0){
		if(decode != _new_decode){
			decode = _new_decode;
			encode = _new_encode;
			_sendDecoding(out, decode);
		}
		_sendEncoded(in, out, encode);
	}
	_sendExit(out);

	return EXIT_SUCCESS;
}

static int _startDecoding(FILE* in, FILE* out){

	return EXIT_SUCCESS;
}

int extraArgumentFunction(int argc, char** argv){
	return 0;
}