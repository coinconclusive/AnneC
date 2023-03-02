#include <annec/lexer.h>
#include <ctype.h>
#include <wctype.h>
#include "../cli.h"

AncTokenType AncTokenType_FromKeyword(const char *keyword) {
	// TODO: Make this more efficient...
#define X(NAME, KW) if(strcmp(keyword, #KW) == 0) return ANC_TOKEN_TYPE_##NAME
	ANC_X_TOKEN_TYPE_KEYWORDS_(X, ;);
#undef X
	return ANC_TOKEN_TYPE_ERROR;
}


static const char *const AncTokenType_keywordNameMap[] = {
#define X(NAME, KW) [ANC_TOKEN_TYPE_##NAME] = (#KW "\0" #KW " keyword") // shady
	// I do this (the shady bit) so that we don't need to *dynamically* append a " keyword" every time.
	ANC_X_TOKEN_TYPE_KEYWORDS_(X, ANC_X__COMMA_)
#undef X
};

const char [[nullable]] *AncTokenType_ToString(AncTokenType self) {
	static const char *symbolic[] = {
#define X(NAME, DESC, KW) [ANC_TOKEN_TYPE_##NAME] = KW
		ANC_X_TOKEN_TYPE_SYMBOLIC_(X, ANC_X__COMMA_),
#undef X
	};

	if(self > ANC_TOKEN_TYPE__SYMB_TOKENS_START_ && self < ANC_TOKEN_TYPE__SYMB_TOKENS_END_)
		return symbolic[self];
	if(self > ANC_TOKEN_TYPE__KW_TOKENS_START_ && self < ANC_TOKEN_TYPE__KW_TOKENS_END_)
		return AncTokenType_keywordNameMap[self];
	return NULL;
}

const char [[nullable]] *AncTokenType_ToNameString(AncTokenType self) {
	static const char *symbolic[] = {
#define X(NAME, DESC, KW) [ANC_TOKEN_TYPE_##NAME] = DESC
		ANC_X_TOKEN_TYPE_SYMBOLIC_(X, ANC_X__COMMA_),
#undef X
	};

	if(self > ANC_TOKEN_TYPE__SYMB_TOKENS_START_ && self < ANC_TOKEN_TYPE__SYMB_TOKENS_END_)
		return symbolic[self];
	if(self > ANC_TOKEN_TYPE__KW_TOKENS_START_ && self < ANC_TOKEN_TYPE__KW_TOKENS_END_)
		return AncTokenType_keywordNameMap[self] + strlen(AncTokenType_keywordNameMap[self]) + 1; // shady
	return NULL;
}

void AncInputFile_Init(AncInputFile *self, AnchAllocator *allocator, AnchUtf8ReadStream *input, const char *filename) {
	assert(self != NULL);

	self->allocator = allocator;
	self->filename = filename;
	self->stream = input;
	self->position = (AncSourcePosition){};
	self->peek = 0;
	AnchDynArray_Init(&self->lines, self->allocator, 0);
}

void AncInputFile_Free(AncInputFile *self) {
	assert(self != NULL);

	self->allocator = NULL;
	self->filename = NULL;
	self->stream = NULL;
	self->position = (AncSourcePosition){};
	self->peek = 0;
	AnchDynArray_Free(&self->lines);
}

AncString *AncInputFile_GetLine(AncInputFile *self, unsigned int lineIndex) {
	assert(self != NULL);

	if(lineIndex == 0) return (AncString*)self->lines.data;
	
	// lineIndex += 1;

	AncString *s = NULL;
	for(size_t offset = 0; lineIndex --> 0;) {
		s = (void*)(self->lines.data + offset);
		offset += sizeof(AncString);
		offset += s->length + 1;
	}

	return s;
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
			span->start.line + 1, span->start.column
		);
	} else {
		AnchWriteFormat(
			wsStderr, ANSI_GRAY "  At %s:%d:%d ... %d:%d\n" ANSI_RESET,
			self->filename,
			span->start.line + 1, span->start.column,
			span->end.line + 1, span->end.column
		);
	}

	if(!show) return;
	if(span->start.line != span->end.line) return;
	AncString *line = AncInputFile_GetLine(self, span->start.line);

	for(size_t i = 0; i < line->length; ++i) {
		if(line->bytes[i] == '\t') {
			AnchWriteChar(wsStderr, ' ');
			AnchWriteChar(wsStderr, ' ');
		}
		AnchWriteChar(wsStderr, line->bytes[i]);
	}

	AnchWriteString(wsStderr, "\n");

	for(unsigned int i = 0; i < span->start.column - 1; ++i) {
		if(line->bytes[i] == '\t')
			AnchWriteChar(wsStderr, ' ');
		AnchWriteChar(wsStderr, ' ');
	}

	AnchWriteString(wsStderr, ANSI_GREEN "^");

	for(unsigned int i = 0; i < span->end.column - span->start.column; ++i) {
		AnchWriteChar(wsStderr, '~');
	}
	AnchWriteString(wsStderr, ANSI_RESET "\n");
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

void AncInputFile_ReadLines(AncInputFile *self) {
	AnchDynArray_PushZeros(&self->lines, sizeof(AncString));
	size_t lineLenOffset = 0;

	char32_t c = 0;
	do {
		c = AnchUtf8ReadStream_Read(self->stream);
		if(c == ANCH_UTF8_STREAM_ERROR) {
			AncInputFile_ReportError(
				self, false, &ANC_SOURCE_SPAN_SAME(self->position),
				"Illegal UTF-8 sequence."
			);
		}

		if(ANC_IS_NL_(c)) {
			if(self->lines.size != 0) *(uint8_t*)AnchDynArray_Push(&self->lines, 1) = 0;
			AnchDynArray_PushZeros(&self->lines, sizeof(AncString));
			lineLenOffset = self->lines.size - sizeof(AncString);
			self->position.line += 1;
			self->position.column = 0;
		} else {
			*(size_t*)(self->lines.data + lineLenOffset) += 1;
			self->position.column += 1;
			AncPushUtf8_(&self->lines, c);
		}
	} while(c != ANCH_UTF8_STREAM_EOF);

	self->position = (AncSourcePosition){};
}

char32_t AncInputFile_Get_(AncInputFile *self, bool peeking) {
	assert(self != NULL);

	if(self->peek) {
		char32_t v = self->peek;
		if(ANC_IS_NL_(v)) {
			self->position.line += 1;
			self->position.column = 0;
		} else {
			self->position.column += 1;
		}
		self->peek = 0;
		return v;
	}

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

	if(!peeking) {
		if(ANC_IS_NL_(c)) {
			self->position.line += 1;
			self->position.column = 0;
		} else {
			self->position.column += 1;
		}
	}

	return c;
}

char32_t AncInputFile_Get(AncInputFile *self) {
	return AncInputFile_Get_(self, false);
}

char32_t AncInputFile_Peek(AncInputFile *self) {
	assert(self != NULL);
	assert(!self->peek);

	self->peek = AncInputFile_Get_(self, true);
	return self->peek;
}

#define ANC_HEX_DIGIT_VALUE_(C) (C >= 'a' ? C - 'a' : C >= 'A' ? C - 'A' : C - '0')

// Integer constant
//   (([1-9]([0-9]['0-9]*)?)|(0([0-7]['0-7]*)?)|
//   (0[xX][0-9a-fA-F]['0-9a-fA-F]*)|(0[bB][01]['01]*))([uUlL]|ll|LL)?

// Floating constant
//   ((0[xX](([0-9a-fA-F]['0-9a-fA-F]*)?\.[0-9a-fA-F]['0-9a-fA-F]*)|
//   [0-9a-fA-F]['0-9a-fA-F]*\.)|(([0-9]['0-9]*)?\.[0-9]['0-9]*)|[0-9]['0-9]*\.)
//   ([eEpP][+-]?[0-9]['0-9]*)?([fFlL]|df|DF|dd|DD|dl|DL)?

/** Check if C is digit in BASE. C and BASE are evaluated multiple times. BASE = 16, 10...2. */
#define ANC_IS_DIGIT_(BASE, C) \
	((BASE) == 16 ? iswxdigit((C)) : (iswdigit((C)) && (C) < ('0' + BASE)))

/** Check if C is digit in BASE=16 or base 10 if BASE=2...10. C and BASE are evaluated multiple times. */
#define ANC_IS_DIGIT_RELAXED_(BASE, C) \
	((BASE) == 16 ? iswxdigit((C)) : iswdigit((C)))

/** Convert C to digit in BASE. BASE = 16, 10...2. */
#define ANC_DIGIT_VALUE_(BASE, C) \
	((BASE) == 16 ? ANC_HEX_DIGIT_VALUE_((C)) : (C) - '0')

typedef enum AncFloatLiteralType {
	ANC_FLOAT_LITERAL_TYPE_INVALID = '!',
	ANC_FLOAT_LITERAL_TYPE_INVALID_DECIMAL_CASE = 1,
	ANC_FLOAT_LITERAL_TYPE_INVALID_DECIMAL_SIZE = 2,
	ANC_FLOAT_LITERAL_TYPE_FLOAT = 'f',
	ANC_FLOAT_LITERAL_TYPE_DECIMAL = 'd',
} AncFloatLiteralType;

typedef enum AncFloatLiteralSize {
	ANC_FLOAT_LITERAL_SIZE_FLOAT = 'f',
	ANC_FLOAT_LITERAL_SIZE_DOUBLE = 'd',
	ANC_FLOAT_LITERAL_SIZE_LONG = 'l',
} AncFloatLiteralSize;

typedef struct AncFloatLiteralSuffix {
	AncFloatLiteralType type;
	AncFloatLiteralSize size;
} AncFloatLiteralSuffix;

#define ANC_FLOATLIT_TOKEN_FORMAT "%c%c%ju.%juP%c%ju"
#define ANC_FLOATLIT_TOKEN_FORMAT_PARAMS suffix.type, suffix.size, wholePart, fracPart, negExp ? '-' : '+', expPart

typedef enum AncIntLiteralType {
	ANC_INT_LITERAL_TYPE_INVALID = '!',
	ANC_INT_LITERAL_TYPE_INVALID_LONGLONG_CASE = 1,
	ANC_INT_LITERAL_TYPE_INVALID_MULTIPLE_U = 2,
	ANC_INT_LITERAL_TYPE_INT = 'i',
	ANC_INT_LITERAL_TYPE_LONG = 'l',
	ANC_INT_LITERAL_TYPE_LONGLONG = 'L',
} AncIntLiteralType;

typedef enum AncIntLiteralSign {
	ANC_INT_LITERAL_SIGN_SIGNED = 's',
	ANC_INT_LITERAL_SIGN_UNSIGNED = 'u',
} AncIntLiteralSign;

typedef struct AncIntLiteralSuffix {
	AncIntLiteralType type;
	AncIntLiteralSign sign;
} AncIntLiteralSuffix;

#define ANC_INTLIT_TOKEN_FORMAT "%c%c%ju"
#define ANC_INTLIT_TOKEN_FORMAT_PARAMS suffix.type, suffix.sign, wholePart

static AncIntLiteralSuffix Parse_Int_Suffix_(const char32_t *str) {
	AncIntLiteralSuffix suffix = { .type = ANC_INT_LITERAL_TYPE_INT, .sign = ANC_INT_LITERAL_SIGN_SIGNED };
	if(!*str) return suffix;
	if(*str == 'u' || *str == 'U') {
		suffix.sign = ANC_INT_LITERAL_SIGN_UNSIGNED;
		str += 1;
	}
	
	if(*str == 'l' || *str == 'L') {
		suffix.type = ANC_INT_LITERAL_TYPE_LONG;
		char32_t first = *str;
		str += 1;
		if(*str == 'l' || *str == 'L') {
			if(*str != first) {
				return (AncIntLiteralSuffix){ .type = ANC_INT_LITERAL_TYPE_INVALID_LONGLONG_CASE };
			}
		}
	}
	
	if(*str == 'u' || *str == 'U') {
		if(suffix.sign == ANC_INT_LITERAL_SIGN_UNSIGNED)
			return (AncIntLiteralSuffix){ .type = ANC_INT_LITERAL_TYPE_INVALID_MULTIPLE_U };
		suffix.sign = ANC_INT_LITERAL_SIGN_UNSIGNED;
		str += 1;
	}

	if(*str) {
		if(*str == 'u' || *str == 'U')
			return (AncIntLiteralSuffix){ .type = ANC_INT_LITERAL_TYPE_INVALID_MULTIPLE_U };
		return (AncIntLiteralSuffix){ .type = ANC_INT_LITERAL_TYPE_INVALID };
	}

	return suffix;
}

static AncFloatLiteralSuffix Parse_Float_Suffix_(const char32_t *str) {
	AncFloatLiteralSuffix suffix = { .type = ANC_FLOAT_LITERAL_TYPE_FLOAT, .size = ANC_FLOAT_LITERAL_SIZE_DOUBLE };
	if(!*str) return suffix;

	if(*str == 'f' || *str == 'F') {
		suffix.size = ANC_FLOAT_LITERAL_SIZE_FLOAT;
		str += 1;
	} else if(*str == 'l' || *str == 'L') {
		suffix.size = ANC_FLOAT_LITERAL_SIZE_LONG;
		str += 1;
	} else if(*str == 'd' || *str == 'D') {
		suffix.type = ANC_FLOAT_LITERAL_TYPE_DECIMAL;
		char32_t first = *str;
		str += 1;
		if(*str == 'f' || *str == 'F') {
			if((*str == 'f' && first != 'd') || (*str == 'F' && first != 'D'))
				return (AncFloatLiteralSuffix){ ANC_FLOAT_LITERAL_TYPE_INVALID_DECIMAL_CASE, suffix.size };
			suffix.size = ANC_FLOAT_LITERAL_SIZE_FLOAT;
			str += 1;
		} else if(*str == 'd' || *str == 'D') {
			suffix.size = ANC_FLOAT_LITERAL_SIZE_DOUBLE;
			if((*str == 'd' && first != 'd') || (*str == 'D' && first != 'D'))
				return (AncFloatLiteralSuffix){ ANC_FLOAT_LITERAL_TYPE_INVALID_DECIMAL_CASE, suffix.size };
			str += 1;
		} else if(*str == 'l' || *str == 'L') {
			if((*str == 'l' && first != 'd') || (*str == 'L' && first != 'D'))
				return (AncFloatLiteralSuffix){ ANC_FLOAT_LITERAL_TYPE_INVALID_DECIMAL_CASE, suffix.size };
			suffix.size = ANC_FLOAT_LITERAL_SIZE_LONG;
			str += 1;
		} else {
			return (AncFloatLiteralSuffix){ ANC_FLOAT_LITERAL_TYPE_INVALID_DECIMAL_SIZE };
		}
	}
	
	if(*str) {
		return (AncFloatLiteralSuffix){ .type = ANC_FLOAT_LITERAL_TYPE_INVALID };
	}

	return suffix;
}

static char32_t AncLexer_Read_Numeric_(AncLexer *self, char32_t c, AncToken *token) {
	intmax_t wholePart = 0, fracPart = 0, expPart = 1;
	bool first = true;
	bool negExp = false;
	int base = 10;
	AncTokenType type = ANC_TOKEN_TYPE_INTLIT;
	AncSourcePosition startPosition = self->input->position;

	if(c == '0') {
		base = 8;
		c = AncInputFile_Get(self->input);
		if(c == 'x') {
			base = 16;
			first = true;
			c = AncInputFile_Get(self->input);
		} else if(c == 'b') {
			base = 2;
			first = true;
			c = AncInputFile_Get(self->input);
		}
	}

	while(ANC_IS_DIGIT_RELAXED_(base, c)) {
		if(!ANC_IS_DIGIT_(base, c)) {
			AncInputFile_ReportError(self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
				"Invalid %s digit in number literal.", base == 8 ? "octal" : "binary");
		}
		wholePart = wholePart * base + ANC_DIGIT_VALUE_(base, c);
		c = AncInputFile_Get(self->input);
		char32_t p = AncInputFile_Peek(self->input);
		if(!first && c == '\'' && ANC_IS_DIGIT_(base, p))
			c = AncInputFile_Get(self->input);
		first = false;
	}
	
	if(c == '.' && (base == 16 || base == 10 || (base == 8 && wholePart == 0))) {
		type = ANC_TOKEN_TYPE_FLOATLIT;
		first = true;
		c = AncInputFile_Get(self->input);
		while(ANC_IS_DIGIT_RELAXED_(base, c)) {
			if(!ANC_IS_DIGIT_(base, c)) {
				AncInputFile_ReportError(self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
					"Invalid %s digit in fraction part of number literal.", base == 8 ? "octal" : "binary");
			}
			fracPart = fracPart * base + ANC_DIGIT_VALUE_(base, c);
			c = AncInputFile_Get(self->input);
			char32_t p = AncInputFile_Peek(self->input);
			if(!first && c == '\'' && ANC_IS_DIGIT_(base, p))
				c = AncInputFile_Get(self->input);
			first = false;
		}
	}

	if((base == 16 && (c == 'p' || c == 'P')) || c == 'e' || c == 'E') {
		type = ANC_TOKEN_TYPE_FLOATLIT;
		first = true;
		c = AncInputFile_Get(self->input);
		
		if(c == '+' || c == '-') {
			negExp = (c == '-');
			c = AncInputFile_Get(self->input);
		}

		if(!iswdigit(c)) {
			AncInputFile_ReportError(self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
				"Expected number after exponent sign.");
		} else {
			expPart = 0;
		}

		while(iswdigit(c)) {
			expPart = expPart * 10 + ANC_DIGIT_VALUE_(10, c);
			c = AncInputFile_Get(self->input);
			char32_t p = AncInputFile_Peek(self->input);
			if(!first && c == '\'' && iswdigit(p))
				c = AncInputFile_Get(self->input);
			first = false;
		}
	}

	token->span = (AncSourceSpan){ startPosition, self->input->position };
	token->type = type;

	AncSourcePosition suffixStart = self->input->position;
	AnchDynArray_Type(char32_t) suffixBuffer;
	AnchDynArray_Init(&suffixBuffer, self->allocator, sizeof(char32_t) * 4);

	for(
		char32_t p = self->input->peek ? self->input->peek : AncInputFile_Peek(self->input);
		iswalnum(p) || p == '_';
	) {
		c = AncInputFile_Get(self->input);
		*(char32_t*)AnchDynArray_Push(&suffixBuffer, sizeof(char32_t)) = c;
		p = AncInputFile_Peek(self->input);
	}
	*(char32_t*)AnchDynArray_Push(&suffixBuffer, sizeof(char32_t)) = '\0';

	if(type == ANC_TOKEN_TYPE_INTLIT) {
		AncIntLiteralSuffix suffix = Parse_Int_Suffix_((char32_t*)suffixBuffer.data);

		if(suffix.type == ANC_INT_LITERAL_TYPE_INVALID_LONGLONG_CASE) {
			AncInputFile_ReportError(self->input, true, &(AncSourceSpan){ suffixStart, self->input->position },
				"Integer suffixes for long long have to be of the same case (`lL` and `Ll` are not allowed).");
			
			// set the type back so that we can detect invalid literals if we want (without having to check every case).
			suffix.type = ANC_INT_LITERAL_TYPE_INVALID;
		} else if(suffix.type == ANC_INT_LITERAL_TYPE_INVALID_MULTIPLE_U) {
			AncInputFile_ReportError(self->input, true, &(AncSourceSpan){ suffixStart, self->input->position },
				"Integer suffix has multiple `u` or `U` specifiers!");
			
			// set the type back so that we can detect invalid literals if we want (without having to check every case).
			suffix.type = ANC_INT_LITERAL_TYPE_INVALID;
		} else if(suffix.type == ANC_INT_LITERAL_TYPE_INVALID) {
			AncInputFile_ReportError(self->input, true, &(AncSourceSpan){ suffixStart, self->input->position },
				"Invalid integer constant suffix.");
		}

		int len = snprintf(NULL, 0, ANC_INTLIT_TOKEN_FORMAT, ANC_INTLIT_TOKEN_FORMAT_PARAMS);
		size_t oldSize = self->tokenValues.size;
		char *buf = AnchArena_Push(&self->tokenValues, len + 1);
		snprintf(buf, len + 1, ANC_INTLIT_TOKEN_FORMAT, ANC_INTLIT_TOKEN_FORMAT_PARAMS);
		token->value.length = len + 1;
		token->value.bytesOffset = oldSize;
	} else {
		AncFloatLiteralSuffix suffix = Parse_Float_Suffix_((char32_t*)suffixBuffer.data);
		
		if(suffix.type == ANC_FLOAT_LITERAL_TYPE_INVALID_DECIMAL_SIZE) {
		AncInputFile_ReportError(self->input, true, &(AncSourceSpan){ suffixStart, self->input->position },
			"Decimal number suffixes require a size specifier (`df`, `dd` or `dl` - upper or lower case).");
			
			// set the type back so that we can detect invalid literals if we want (without having to check every case).
			suffix.type = ANC_FLOAT_LITERAL_TYPE_INVALID;
		} else if(suffix.type == ANC_FLOAT_LITERAL_TYPE_INVALID_DECIMAL_CASE) {
			AncInputFile_ReportError(self->input, true, &(AncSourceSpan){ suffixStart, self->input->position },
				"Decimal number suffixes have to be the same case (`df`, `DF`, `dd`, `DD`, `dl` or `DL`)");
			
			// set the type back so that we can detect invalid literals if we want (without having to check every case).
			suffix.type = ANC_FLOAT_LITERAL_TYPE_INVALID;
		} else if(suffix.type == ANC_INT_LITERAL_TYPE_INVALID) {
			AncInputFile_ReportError(self->input, true, &(AncSourceSpan){ suffixStart, self->input->position },
				"Invalid float constant suffix.");
		}
		
		int len = snprintf(NULL, 0, ANC_FLOATLIT_TOKEN_FORMAT, ANC_FLOATLIT_TOKEN_FORMAT_PARAMS);
		size_t oldSize = self->tokenValues.size;
		char *buf = AnchArena_Push(&self->tokenValues, len + 1);
		snprintf(buf, len + 1, ANC_FLOATLIT_TOKEN_FORMAT, ANC_FLOATLIT_TOKEN_FORMAT_PARAMS);
		token->value.length = len + 1;
		token->value.bytesOffset = oldSize;
	}

	AnchDynArray_Free(&suffixBuffer);
	return AncInputFile_Get(self->input);
}

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
			c = AncInputFile_Get(self->input);
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
					c = AncInputFile_Get(self->input);
					if(isxdigit(c)) {
						while(isxdigit(c)) {
							value = value * 16 + ANC_HEX_DIGIT_VALUE_(c);
							c = AncInputFile_Get(self->input);
						}
					} else {
						AncInputFile_ReportError(
							self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
							"Non-hexadecimal digit '%c' in hexadecimal escape sequence.", c
						);
					}
				} break;
				case 'u': {
					c = AncInputFile_Get(self->input);
					for(int i = 0; i < 4; ++i) {
						if(!isxdigit(c)) {
							AncInputFile_ReportError(
								self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
								"Non-hexadecimal digit '%c' in short universal characer name escape sequence.", c
							);
						}
						value = value * 16 + ANC_HEX_DIGIT_VALUE_(c);
						c = AncInputFile_Get(self->input);
					}
				} break;
				case 'U': {
					c = AncInputFile_Get(self->input);
					for(int i = 0; i < 8; ++i) {
						if(!isxdigit(c)) {
							AncInputFile_ReportError(
								self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
								"Non-hexadecimal digit '%c' in long universal characer name escape sequence.", c
							);
						}
						value = value * 16 + ANC_HEX_DIGIT_VALUE_(c);
						c = AncInputFile_Get(self->input);
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
							c = AncInputFile_Get(self->input);
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
			c = AncInputFile_Get(self->input);
		}
		AncPushUtf8_(s, value);
	}

	return AncInputFile_Get(self->input);
}

void AncLexer_Read_(AncLexer *self, AncToken *token) {
	assert(self != NULL);
	assert(token != NULL);
	
	char32_t c = AncInputFile_Get(self->input);

	while(iswspace(c)) {
		c = AncInputFile_Get(self->input);
		if(ANC_IS_NL_(c)) self->newline = true;
	}

	while(c == '/') {
		char32_t p = AncInputFile_Peek(self->input);
		if(p == '/') {
			c = AncInputFile_Get(self->input);
			while(!ANC_IS_NL_(c)) c = AncInputFile_Get(self->input);
			self->newline = true;
		} else if(p == '*') {
			c = AncInputFile_Get(self->input);
			while(1) {
				c = AncInputFile_Get(self->input);
				if(c == '*' && AncInputFile_Peek(self->input) == '/') {
					c = AncInputFile_Get(self->input);
					break;
				}
			}
			c = AncInputFile_Get(self->input);
		} else break;

		while(iswspace(c)) {
			c = AncInputFile_Get(self->input);
			if(ANC_IS_NL_(c)) self->newline = true;
		}
	}

	while(iswspace(c)) {
		c = AncInputFile_Get(self->input);
		if(ANC_IS_NL_(c)) self->newline = true;
	}

	if(c == ANC_INPUT_FILE_EOF) {
		token->type = ANC_TOKEN_TYPE_EOF;
		token->value = (AncArenaStringView){};
		token->span = ANC_SOURCE_SPAN_SAME(self->input->position);
		return;
	}

	AncSourcePosition startPosition = self->input->position;

	if(self->mode == ANC_LEXER_MODE_PP_INCLUDE && c == '<') {
	} else if(iswalpha(c) || c == '_' || c == '\'' || c == '"') {
		if(self->mode != ANC_LEXER_MODE_PP)
			self->mode = ANC_LEXER_MODE_NO_PP;

		AnchDynArray_Type(char8_t) *s = &self->tokenValues;
		size_t oldSize = s->size;

		AncPushUtf8_(s, c);
		char32_t p = AncInputFile_Peek(self->input);
		while(iswalnum(p) || p == '_') {
			c = AncInputFile_Get(self->input);
			AncPushUtf8_(s, c);
			p = AncInputFile_Peek(self->input);
		}

		AncTokenType type = 0;
		
		if(p == '"' || p == '\'') {
			bool isChar = (p == '\'');
			AncPushUtf8_(s, p);
			c = AncInputFile_Get(self->input);
			c = AnchLexer_Read_StringOrChar_(self, isChar, c, s, token);
			type = isChar ? ANC_TOKEN_TYPE_CHARLIT : ANC_TOKEN_TYPE_STRING;
		}
		
		AncPushUtf8_(s, 0);
		type = type ? type : AncTokenType_FromKeyword((char*)s->data);
		token->type = !type ? ANC_TOKEN_TYPE_IDENT : type;
		token->value.length = s->size - oldSize;
		token->value.bytesOffset = oldSize;
		token->span = (AncSourceSpan){ startPosition, self->input->position };
	} else if(iswdigit(c) || (c == '.' && iswdigit(AncInputFile_Peek(self->input)))) {
		if(self->mode != ANC_LEXER_MODE_PP)
			self->mode = ANC_LEXER_MODE_NO_PP;
		
		c = AncLexer_Read_Numeric_(self, c, token);
	} else {
		char32_t p = self->input->peek ? self->input->peek : AncInputFile_Peek(self->input);
		bool eat = false;

#define OPTIONAL_EQUALS(DEFAULT, EQUALS) token->type = DEFAULT; \
	if(p == '=' && (eat = true)) token->type = EQUALS;

		switch(c) {
			case '+':
				token->type = ANC_TOKEN_TYPE_PLUS;
				if(p == '=' && (eat = true)) token->type = ANC_TOKEN_TYPE_PLUS_EQ;
				if(p == '+' && (eat = true)) token->type = ANC_TOKEN_TYPE_PLUS_PLUS;
				break;
			case '-':
				token->type = ANC_TOKEN_TYPE_MINUS;
				if(p == '=' && (eat = true)) token->type = ANC_TOKEN_TYPE_MINUS_EQ;
				if(p == '-' && (eat = true)) token->type = ANC_TOKEN_TYPE_MINUS_MINUS;
				if(p == '>' && (eat = true)) token->type = ANC_TOKEN_TYPE_ARROW;
				break;
			case '*': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_STAR, ANC_TOKEN_TYPE_STAR_EQ); break;
			case '/': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_SLASH, ANC_TOKEN_TYPE_SLASH_EQ); break;
			case '%': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_PERC, ANC_TOKEN_TYPE_PERC_EQ); break;
			case '&': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_AMP, ANC_TOKEN_TYPE_AMP_EQ); break;
			case '|': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_BAR, ANC_TOKEN_TYPE_BAR_EQ); break;
			case '^': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_CIRC, ANC_TOKEN_TYPE_CIRC_EQ); break;
			case '=': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_EQUAL, ANC_TOKEN_TYPE_EQUALEQUAL); break;
			case '!': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_EXC, ANC_TOKEN_TYPE_EXC_EQ); break;
			case '<': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_LT, ANC_TOKEN_TYPE_LT_EQ); break;
			case '>': OPTIONAL_EQUALS(ANC_TOKEN_TYPE_GT, ANC_TOKEN_TYPE_GT_EQ); break;
			case '#':
				if(self->mode != ANC_LEXER_MODE_NO_PP)
					self->mode = ANC_LEXER_MODE_PP;
				token->type = ANC_TOKEN_TYPE_HASH;
				break;
			case '~': token->type = ANC_TOKEN_TYPE_TILDE; break;
			case '.': token->type = ANC_TOKEN_TYPE_DOT; break;
			case '[': token->type = ANC_TOKEN_TYPE_LBRACKET; break;
			case ']': token->type = ANC_TOKEN_TYPE_RBRACKET; break;
			case '(': token->type = ANC_TOKEN_TYPE_LPAREN; break;
			case ')': token->type = ANC_TOKEN_TYPE_RPAREN; break;
			case '{': token->type = ANC_TOKEN_TYPE_LBRACE; break;
			case '}': token->type = ANC_TOKEN_TYPE_RBRACE; break;
			case '?': token->type = ANC_TOKEN_TYPE_QUEST; break;
			case ':': token->type = ANC_TOKEN_TYPE_COLON; break;
			case ';': token->type = ANC_TOKEN_TYPE_SEMICOLON; break;
			case ',': token->type = ANC_TOKEN_TYPE_COMMA; break;
			default:
				AncInputFile_ReportError(
					self->input, true, &ANC_SOURCE_SPAN_SAME(self->input->position),
					"Unknown character."
				);
				break;
		}

		if(self->mode != ANC_LEXER_MODE_PP)
			self->mode = ANC_LEXER_MODE_NO_PP;
		
		if(eat) c = AncInputFile_Get(self->input);
	}
}

void AncLexer_Init(AncLexer *self, AnchAllocator *allocator, AncInputFile *input) {
	assert(self != NULL);

	self->allocator = allocator;
	self->input = input;
	self->mode = ANC_LEXER_MODE_DEFAULT;
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
	self->mode = ANC_LEXER_MODE_DEFAULT;
}

AncToken *AncLexer_Read(AncLexer *self) {
	assert(self != NULL);
	
	if(self->tokenPeekBuf.size > 0) {
		AncToken *last = *(AncToken**)ANCH_ARENA_LAST(&self->tokenPeekBuf);
		AnchArena_Pop(&self->tokenPeekBuf, sizeof(AncToken*));
		return last;
	}

	AncToken *token = AnchArena_PushZeros(&self->tokens, sizeof(AncToken));
	AncLexer_Read_(self, token);
	return token;
}
