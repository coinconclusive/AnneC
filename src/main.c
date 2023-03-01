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

	AncToken *token = AncLexer_Read(&lexer);
	AnchWriteFormat(wsStdout, "%d, `%s`\n", token->type, lexer.tokenValues.data + token->value.bytesOffset);

	AncLexer_Free(&lexer);

	AncInputFile_Free(&inputFile);

	AnchByteFileReadStream_Close(&inputFileStream);
}
