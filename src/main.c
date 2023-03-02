#include <ctype.h>
#include <locale.h>
#include <annec/lexer.h>
#include "cli.h"

AnchCharWriteStream *wsStdout;
AnchCharWriteStream *wsStderr;

static AnchAllocator *allocator;

int main(int argc, char *argv[]) {
	fputc('\n', stdout);
	setlocale(LC_ALL, "en_US.utf8");
	
  AnchFileWriteStream valueWsStdout = {0};
  AnchFileWriteStream_InitWith(&valueWsStdout, stdout);
  wsStdout = &valueWsStdout.stream;
  
  AnchFileWriteStream valueWsStderr = {0};
  AnchFileWriteStream_InitWith(&valueWsStderr, stderr);
  wsStderr = &valueWsStderr.stream;

  AnchDefaultAllocator defaultAllocator;
  AnchDefaultAllocator_Init(&defaultAllocator);

  AnchStatsAllocator statsAllocator;
  AnchStatsAllocator_Init(&statsAllocator, &defaultAllocator);

  allocator = &statsAllocator.base;

	AnchByteFileReadStream inputFileStream = {};
	AnchByteFileReadStream_Init(&inputFileStream);
	AnchByteFileReadStream_Open(&inputFileStream, "test.txt");

	AncInputFile inputFile = {};
	AncInputFile_Init(&inputFile, allocator, &inputFileStream.stream, "test.txt");

	AncInputFile_ReadLines(&inputFile);
	AnchByteFileReadStream_Rewind(&inputFileStream);

	AncLexer lexer = {};
	AncLexer_Init(&lexer, allocator, &inputFile);

	while(1) {
		AncToken *token = AncLexer_Read(&lexer);
		if(token->type == ANC_TOKEN_TYPE_EOF) break;

		size_t wrote = 0;

		if(isprint(token->type)) {
			wrote += AnchWriteFormat(wsStdout, "'%lc'", token->type);
		} else {
			const char *s = AncTokenType_ToString(token->type);
			if(s == NULL) {
				switch(token->type) {
					case ANC_TOKEN_TYPE_IDENT: s = "id"; break;
					case ANC_TOKEN_TYPE_INTLIT: s = "int"; break;
					case ANC_TOKEN_TYPE_FLOATLIT: s = "float"; break;
					case ANC_TOKEN_TYPE_CHARLIT: s = "char"; break;
					case ANC_TOKEN_TYPE_STRING: s = "str"; break;
					case ANC_TOKEN_TYPE_INCLUDE_STRING: s = "istr"; break;
					default: break;
				}
				wrote += AnchWriteFormat(wsStdout, "%s", s);
			} else {
				wrote += AnchWriteFormat(wsStdout, "'%s'", s);
			}
		}
		if(token->type == ANC_TOKEN_TYPE_IDENT
		|| token->type == ANC_TOKEN_TYPE_INTLIT
		|| token->type == ANC_TOKEN_TYPE_FLOATLIT
		|| token->type == ANC_TOKEN_TYPE_CHARLIT
		|| token->type == ANC_TOKEN_TYPE_STRING
		) {
			for(int i = 0; i < 6 - (int)wrote; ++i) {
				AnchWriteChar(wsStdout, ' ');
				wrote += 1;
			}
			wrote += AnchWriteFormat(wsStdout, "`%s`", lexer.tokenValues.data + token->value.bytesOffset);
		}

		for(int i = 0; i < 16 - (int)wrote; ++i) {
			AnchWriteChar(wsStdout, ' ');
		}

		if(AncSourcePosition_Equal(token->span.start, token->span.end)) {
			AnchWriteFormat(
				wsStdout, "@ %s:%d:%d\n",
				lexer.input->filename,
				token->span.start.line + 1, token->span.start.column + 1
			);
		} else {
			AnchWriteFormat(
				wsStdout, "@ %s:%d:%d ... %d:%d\n",
				lexer.input->filename,
				token->span.start.line + 1, token->span.start.column,
				token->span.end.line + 1, token->span.end.column + 1
			);
		}
	}

	AncLexer_Free(&lexer);

	AncInputFile_Free(&inputFile);

	AnchByteFileReadStream_Close(&inputFileStream);
}
