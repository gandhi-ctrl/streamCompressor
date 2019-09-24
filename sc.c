#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "argEval.h"
#include "sc_protocol.h"
#include "sc_zeroEncoder.h"
#include "sc_textEncoder.h"

struct config_t{
	char*   inputFile;
	char*   outputFile;
	int     encode;
	int     useStats;
	int     junkSize;
	int     showStats;

	int     exit;
} _config = {
	NULL,
	NULL,
	0,
	0,
	1024,
	0,

	0
};

int _statistic[256];

sc_encoder_table_t* _new_encode = NULL;
sc_decoder_table_t* _new_decode = NULL;

int extraArgumentFunction(int argc, char** argv);
int typeSpecificationFunction(int argc, char** argv);

static ArgumentDefinition_t _argArray[] = {
  {"h", "help",     "shows this screen",                                                       0, vType_none,     NULL,                      NULL},
  {"c", "compress", "enable stream compression",                                               0, vType_none,     NULL,                      &_config.encode},
  {"i", "inFile",   "specifies an input file (default: stdin)",                                1, vType_string,   &_config.inputFile,        NULL},
  {"o", "outFile",  "specifies an output file (default: stdout)",                              1, vType_string,   &_config.outputFile,       NULL},
  {"j", "junk",     "specifies the junksize (default: {})",                                    1, vType_integer,  &_config.junkSize,         NULL},
  {"t", "type",     "specifies the type of datastream for better compression (default: none)", 1, vType_callback, typeSpecificationFunction, NULL},
  {"s", "show",     "show statistic when a new encoding table is being created",               0, vType_none,     NULL,                      &_config.showStats}
};

static int _startEncoding(FILE* in, FILE* out);
static int _startDecoding(FILE* in, FILE* out);

int main(int argc, char** argv)
{
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
		if(_new_encode == NULL){
			char* arg[] = { "none" };
			typeSpecificationFunction(1, arg);
		}
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
		_statistic[tmp]++;
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

static void _showStats(){
	float sum = 0.0;
	for(int n = 0; n < 256; n++){
		sum += _statistic[n];
	}
	sum /= 100.0;

	fprintf(stderr, "Stats:\n");
	for(int n = 0; n < 256; ){
		fprintf(stderr, "    ");
		for(int m = 0; m < 8; m++, n++){
			if(isprint(n) != 0){
				fprintf(stderr, "%02X '%c': ", n, n);
			}else{
				fprintf(stderr, "%02X    : ", n);
			}
			if(_statistic[n] == 0){
				fprintf(stderr, "  -    ");
			}else{
				fprintf(stderr, "%4.1f   ", _statistic[n] /sum);
			}
		}
		fprintf(stderr, "\n");
	}
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
			memset(_statistic, 0, sizeof(_statistic));
		}
		_sendEncoded(in, out, encode);
		
		if(_config.showStats != 0){
			_showStats();
		}
	}
	_sendExit(out);

	return EXIT_SUCCESS;
}

static int _tryDecode(uint32_t shift, int bits, sc_decoder_table_t* decode, int* n, FILE* out){
	int found = 0;
	sc_decoder_entry_t* dtemp;
	int tbits = bits;
	uint32_t tshift = shift;
	do{
		shift = tshift;
		bits = tbits;
		found = 0;
		dtemp = decode->entries;

		int valid;
		do{
			valid = 0;
			int tmp;
			if(tbits < 8){
				tmp = (tshift << (8 -tbits)) &0xFF;
			}else if(tbits > 8){
				tmp = (tshift >> (tbits -8)) &0xFF;
			}else{
				tmp = tshift &0xFF;
			}

			if(tbits >= (dtemp[tmp].flag_bits &SC_DECODER_BITS_MASK)){
				valid = 1;
				tshift >>= dtemp[tmp].flag_bits &SC_DECODER_BITS_MASK;
				tbits -= dtemp[tmp].flag_bits &SC_DECODER_BITS_MASK;

				if((dtemp[tmp].flag_bits &SC_DECODER_JUMP_FLAG) != 0){
					dtemp = &dtemp[dtemp[tmp].code];
				}else{
					fputc(dtemp[tmp].code, out);
					dtemp = decode->entries;
					found = 1;
					(*n)++;
				}
			}
		}while(valid != 0);
	}while(found != 0);

	return bits;
}

static int _startDecoding(FILE* in, FILE* out){
	sc_decoder_table_t* decode = NULL;
	sc_header_t head;
	
	while(_config.exit == 0){
		fread(&head, sizeof(sc_header_t), 1, in);

		switch(head.header){
			case SC_DATA_HEADER:
				if(decode == NULL){
					fprintf(stderr, "error: no decode tabe was found!\n");
					_config.exit = 1;
				}else{
					int bits = 0;
					int bdiff = 0;
					uint32_t shift = 0;
					for(int n = 0; n < head.size && !feof(in) && bdiff >= 0; ){
						shift <<= 8;
						shift |= fgetc(in);
						bits += 8;

						int newBits = _tryDecode(shift, bits, decode, &n, out);
						int bdiff = bits -newBits;
						if(bdiff > 0){
							shift >>= bdiff;
							bits -= bdiff;
						}
					}
				}
			break;
			case SC_HUFFMAN_DECODER:
				if(decode != NULL){
					free(decode);
					decode = NULL;
				}
				decode = malloc(head.size);
				if(decode == NULL){
					fprintf(stderr, "error: unable to allocate memory (%dbytes)\n", head.size);
					_config.exit = 1;
				}else{
					fread(decode, head.size, 1, in);
				}
			break;
			case SC_EXIT:
				_config.exit = 1;
			break;
			default: {
				fprintf(stderr, "warning: unknown header detected (0x%08X), skipping %dbytes\n", head.header, head.size);
				uint8_t dump[256];
				for(int n = 0; n < head.size && !feof(in); n++){
					int diff = head.size -n;
					if(diff > 256){
						fread(dump, 256, 1, in);
					}else{
						fread(dump, diff, 1, in);
					}
				}
			} break;
		}

		if(feof(in)){
			_config.exit = 1;
		}
	}
	_sendExit(out);

	return EXIT_SUCCESS;
}

int extraArgumentFunction(int argc, char** argv){
	return 0;
}

int typeSpecificationFunction(int argc, char** argv){
	if(strcmp("help", argv[0]) == 0){
		printf("Possible types are:\n");
		printf("    none\n");
		printf("    text\n");
		exit(EXIT_SUCCESS);
	}else if(strcmp("none", argv[0]) == 0){
		_new_encode = _sc_creatZeroEncoding();
		_new_decode = _sc_creatZeroDecoding();
	}else if(strcmp("text", argv[0]) == 0){
		_new_encode = _sc_creatTextEncoding();
		_new_decode = _sc_creatTextDecoding();
	}
	return 0;
}