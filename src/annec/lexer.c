#include <annec/lexer.h>
#include <ctype.h>
#include <wctype.h>
#include "../cli.h"

AncTokenType AncTokenType_FromKeyword(const char *keyword) {
#define X(NAME, KW) if(strcmp(keyword, #KW) == 0) return ANC_TOKEN_TYPE_##NAME
	ANC_X_TOKEN_TYPE_KEYWORDS_(X, ;);
#undef X
	return ANC_TOKEN_TYPE_ERROR;
}

void AncInputFile_Init(AncInputFile *self, AnchAllocator *allocator, AnchUtf8ReadStream *input, const char *filename) {
	assert(self != NULL);

	self->allocator = allocator;
	self->filename = filename;
	self->stream = input;
	self->position = (AncSourcePosition){};
	self->lineLenOffset = 0;
	AnchDynArray_Init(&self->lines, self->allocator, 0);
}

void AncInputFile_Free(AncInputFile *self) {
	assert(self != NULL);

	self->filename = NULL;
	self->stream = NULL;
	self->position = (AncSourcePosition){};
	self->lineLenOffset = 0;
	AnchDynArray_Free(&self->lines);
	self->allocator = NULL;
}

void AncInputFile_ReportError(AncInputFile *self, bool show, const AncSourceSpan *span, const char *fmt, ...) {
	assert(self != NULL);
	assert(fmt != NULL);

	AnchWriteFormat(wsStderr, ANSI_BRED "Error: " ANSI_RESET);
	va_list va;
	va_start(va, fmt);
	AnchWriteFormatV(wsStderr, fmt, va);
	va_end(va);
	AnchWriteString(wsStderr, "\n");
	
	if(span == NULL) return;

	if(AncSourcePosition_Equal(span->start, span->end)) {
		AnchWriteFormat(
			wsStderr, ANSI_GRAY "  At %s:%d:%d\n" ANSI_RESET,
			self->filename,
			span->start.line + 1, span->start.column + 1
		);
	} else {
		AnchWriteFormat(
			wsStderr, ANSI_GRAY "  At %s:%d:%d ... %d:%d\n" ANSI_RESET,
			self->filename,
			span->start.line + 1, span->start.column + 1,
			span->end.line + 1, span->end.column + 1
		);
	}

	if(!show) return;
	if(span->start.line != span->end.line) return;
}

#define ANC_IS_NL_(C) (C == '\n' || C == u'\u2028' || C == u'\u2029')

struct AncPushUt8_Result {
	int length;
	uint8_t *ptr;
} AncPushUtf8_(AnchArena *arena, char32_t c32) {
	assert(arena != NULL);

	if(c32 == 0) {
		uint8_t *ptr = (uint8_t*)AnchArena_Push(arena, 1);
		*ptr = 0;
		return (struct AncPushUt8_Result){ 1, ptr };
	}

	uint8_t data[MB_CUR_MAX];
	mbstate_t ps = {};
	int len = c32rtomb((char*)data, c32, &ps);
	assert(len != -1);
	uint8_t *ptr = AnchArena_Push(arena, len);
	memcpy(ptr, data, len);
	return (struct AncPushUt8_Result){ len, ptr };
}

char32_t AncInputFile_Get_(AncInputFile *self) {
	assert(self != NULL);

	char32_t c = AnchUtf8ReadStream_Read(self->stream);
	if(c == ANCH_UTF8_STREAM_ERROR) {
		AncInputFile_ReportError(
			self, false, &ANC_SOURCE_SPAN_SAME(self->position),
			"Illegal UTF-8 sequence."
		);
	}

	if(c == ANCH_UTF8_STREAM_EOF) {
		return ANC_INPUT_FILE_EOF;
	}

	if(ANC_IS_NL_(c)) {
		AncString *str = AnchDynArray_Push(&self->lines, sizeof(AncString));
		*str = (AncString){};
		self->lineLenOffset = (uintptr_t)(void*)str;
		self->position.line += 1;
		self->position.column = 0;
	} else {
		AncPushUtf8_(&self->lines, c);
		*(size_t*)(self->lines.data + self->lineLenOffset) += 1;
		self->position.column += 1;
	}
	return c;
}

// Integer constant
//   (([1-9]([0-9]['0-9]*)?)|(0([0-7]['0-7]*)?)|
//   (0[xX][0-9a-fA-F]['0-9a-fA-F]*)|(0[bB][01]['01]*))([uUlL]|ll|LL)?

// Floating constant
//   ((0[xX](([0-9a-fA-F]['0-9a-fA-F]*)?\.[0-9a-fA-F]['0-9a-fA-F]*)|
//   [0-9a-fA-F]['0-9a-fA-F]*\.)|(([0-9]['0-9]*)?\.[0-9]['0-9]*)|[0-9]['0-9]*\.)
//   ([eEpP][+-]?[0-9]['0-9]*)?([fFlL]|df|DF|dd|DD|dl|DL)?

#define ANC_HEX_DIGIT_VALUE_(C) (C >= 'a' ? C - 'a' : C >= 'A' ? C - 'A' : C - '0')

// Character constant
//   (L|u|U|u8)?'([^\\]|(\\(['"?\\abfnrtv]|[0-7]{1,3}|x
//   [0-9a-fA-F]{1,2}|u[0-9a-fA-F]{4}|U[0-9a-fA-F]{8})))+'

// String literals
//   (L|u|U|u8)?"([^\\]|(\\(['"?\\abfnrtv]|[0-7]{1,3}|x
//   [0-9a-fA-F]{1,2}|u[0-9a-fA-F]{4}|U[0-9a-fA-F]{8})))*"

char32_t AnchLexer_Read_StringOrChar_(AncLexer *self, bool isChar, char32_t c, AnchDynArray *s, AncToken *token) {
	assert(self != NULL);
	assert(token != NULL);
	assert(s != NULL);

	if(isChar && c == '\'') {
		AncInputFile_ReportError(
			self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
			"An empty character literal is illegal."
		);
		return c;
	}

	const char end = isChar ? '\'' : '"';
	while(c != end) {
		if(ANC_IS_NL_(c)) {
			AncInputFile_ReportError(
				self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
				"Newlines in character or string literals are not allowed."
			);
			break;
		}

		intmax_t value = 0;
		if(c == '\\') {
			c = AncInputFile_Get_(self->input);
			switch(c) {
				case '\'': value = '\''; break;
				case '\\': value = '\\'; break;
				case '"': value = '\"'; break;
				case '?': value = '\?'; break;
				case 'a': value = '\a'; break;
				case 'b': value = '\b'; break;
				case 'f': value = '\f'; break;
				case 'n': value = '\n'; break;
				case 'r': value = '\r'; break;
				case 't': value = '\t'; break;
				case 'v': value = '\v'; break;
				case 'e': value = '\x1b' /* = '\033' */; break;
				case 'x': {
					c = AncInputFile_Get_(self->input);
					if(isxdigit(c)) {
						while(isxdigit(c)) {
							value = value * 16 + ANC_HEX_DIGIT_VALUE_(c);
							c = AncInputFile_Get_(self->input);
						}
					} else {
						AncInputFile_ReportError(
							self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
							"Non-hexadecimal digit '%c' in hexadecimal escape sequence.", c
						);
					}
				} break;
				case 'u': {
					c = AncInputFile_Get_(self->input);
					for(int i = 0; i < 4; ++i) {
						if(!isxdigit(c)) {
							AncInputFile_ReportError(
								self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
								"Non-hexadecimal digit '%c' in short universal characer name escape sequence.", c
							);
						}
						value = value * 16 + ANC_HEX_DIGIT_VALUE_(c);
						c = AncInputFile_Get_(self->input);
					}
				} break;
				case 'U': {
					c = AncInputFile_Get_(self->input);
					for(int i = 0; i < 8; ++i) {
						if(!isxdigit(c)) {
							AncInputFile_ReportError(
								self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
								"Non-hexadecimal digit '%c' in long universal characer name escape sequence.", c
							);
						}
						value = value * 16 + ANC_HEX_DIGIT_VALUE_(c);
						c = AncInputFile_Get_(self->input);
					}
				} break;
				default:
					if(c >= '0' && c <= '7') {
						while(isdigit(c)) {
							if(c > '7') {
								AncInputFile_ReportError(
									self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
									"Non-octal digit '%c' in octal escape sequence.", c
								);
							}
							value = value * 8 + c - '0';
							c = AncInputFile_Get_(self->input);
						}
					} else {
						AncInputFile_ReportError(
							self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
							"Bad escape sequence '\\%lc'.", c
						);
					}
			}
		} else {
			value = c;
			c = AncInputFile_Get_(self->input);
		}
		AncPushUtf8_(s, value);
	}

	return AncInputFile_Get_(self->input);
}

void AncLexer_Read_(AncLexer *self, AncToken *token) {
	assert(self != NULL);
	assert(token != NULL);
	
	char32_t c = AncInputFile_Get_(self->input);
	if(c == ANC_INPUT_FILE_EOF) {
		token->type = ANC_TOKEN_TYPE_EOF;
		token->value = (AncArenaStringView){};
		token->span = ANC_SOURCE_SPAN_SAME(self->input->position);
		return;
	}

	AncSourcePosition startPosition = self->input->position;

	if(iswalpha(c) || c == '_' || c == '\'' || c == '"') {
		AnchDynArray_Type(char8_t) *s = &self->tokenValues;
		size_t oldSize = s->size;

		while(iswalnum(c) || c == '_') {
			AncPushUtf8_(s, c);
			c = AncInputFile_Get_(self->input);
		}
		
		if(c == '"' || c == '\'') {
			bool isChar = c == '\'';
			AncPushUtf8_(s, c);
			c = AncInputFile_Get_(self->input);
			c = AnchLexer_Read_StringOrChar_(self, isChar, c, s, token);
		}
		
		AncPushUtf8_(s, 0);
		AncTokenType type = AncTokenType_FromKeyword((char*)s->data);
		token->type = !type ? ANC_TOKEN_TYPE_IDENT : type;
		token->value.length = s->size - oldSize;
		token->value.bytesOffset = oldSize;
		token->span = (AncSourceSpan){ startPosition, self->input->position };
	}
}

void AncLexer_Init(AncLexer *self, AnchAllocator *allocator, AncInputFile *input) {
	assert(self != NULL);

	self->allocator = allocator;
	self->input = input;
	AnchDynArray_Init(&self->tokens, self->allocator, 0);
	AnchArena_Init(&self->tokenValues, self->allocator, 0);
	AnchDynArray_Init(&self->tokenPeekBuf, self->allocator, 0);
}

void AncLexer_Free(AncLexer *self) {
	assert(self != NULL);

	AnchDynArray_Free(&self->tokens);
	AnchArena_Free(&self->tokenValues);
	AnchDynArray_Free(&self->tokenPeekBuf);
	self->input = NULL;
	self->allocator = NULL;
}

AncToken *AncLexer_Read(AncLexer *self) {
	assert(self != NULL);
	
	if(self->tokenPeekBuf.size > 0) {
		AncToken *last = *(AncToken**)ANCH_ARENA_LAST(&self->tokenPeekBuf);
		AnchArena_Pop(&self->tokenPeekBuf, sizeof(AncToken*));
		return last;
	}

	AncToken *token = AnchArena_Push(&self->tokens, sizeof(AncToken));
	AncLexer_Read_(self, token);
	return token;
}
