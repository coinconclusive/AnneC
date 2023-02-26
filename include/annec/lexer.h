#ifndef ANNEC_LEXER_H
#define ANNEC_LEXER_H
#include <annec_anchor.h>

typedef struct AncfStringView {
	size_t length;
	uint8_t *bytes;
} AncStringView;

typedef struct AncArenaStringView {
	size_t length;
	uintptr_t bytesOffset;
} AncArenaStringView;

typedef struct AncString {
	size_t length;
	uint8_t bytes[];
} AncString;

#define ANC_ARENA_STRING_VIEW_GET(SV, ARENA) \
	(uint8_t*)((ARENA)->data + (SV)->bytesOffset)

#define ANC_X__COMMA_ ,
#define ANC_X_TOKEN_TYPE_KEYWORDS_(X, SEP) \
	X(U_ALIGNOF, _Alignof) SEP \
	X(U_STATIC_ASSERT, _Static_assert) SEP \
	X(U_ALIGNAS, _Alignas) SEP \
	X(U_ATOMIC, _Atomic) SEP \
	X(U_BITINT, _BitInt) SEP \
	X(U_BOOL, _Bool) SEP \
	X(U_COMPLEX, _Complex) SEP \
	X(U_DECIMAL128, _Decimal128) SEP \
	X(U_DECIMAL64, _Decimal64) SEP \
	X(U_DECIMAL32, _Decimal32) SEP \
	X(U_GENERIC, _Generic) SEP \
	X(U_IMAGINARY, _Imaginary) SEP \
	X(U_THREAD_LOCAL, _Thread_local) SEP \
	X(ALIGNAS, alignas) SEP \
	X(ALIGNOF, alignof) SEP \
	X(AUTO, auto) SEP \
	X(BOOL, bool) SEP \
	X(BREAK, break) SEP \
	X(CASE, case) SEP \
	X(CHAR, char) SEP \
	X(CONST, const) SEP \
	X(CONSTEXPR, constexpr) SEP \
	X(CONTINUE, continue) SEP \
	X(DEFAULT, default) SEP \
	X(DO, do) SEP \
	X(DOUBLE, double) SEP \
	X(ELSE, else) SEP \
	X(ENUM, enum) SEP \
	X(EXTERN, extern) SEP \
	X(FALSE, false) SEP \
	X(FLOAT, float) SEP \
	X(FOR, for) SEP \
	X(GOTO, goto) SEP \
	X(IF, if) SEP \
	X(INLINE, inline) SEP \
	X(INT, int) SEP \
	X(LONG, long) SEP \
	X(NULLPTR, nullptr) SEP \
	X(REGISTER, register) SEP \
	X(RESTRICT, restrict) SEP \
	X(RETURN, return) SEP \
	X(SHORT, short) SEP \
	X(SIGNED, signed) SEP \
	X(SIZEOF, sizeof) SEP \
	X(STATIC, static) SEP \
	X(STATIC_ASSERT, static_assert) SEP \
	X(STRUCT, struct) SEP \
	X(SWITCH, switch) SEP \
	X(THREAD_LOCAL, thread_local) SEP \
	X(TRUE, true) SEP \
	X(TYPEDEF, typedef) SEP \
	X(TYPEOF, typeof) SEP \
	X(TYPEOF_UNQUAL, typeof_unqual) SEP \
	X(UNION, union) SEP \
	X(UNSIGNED, unsigned) SEP \
	X(VOID, void) SEP \
	X(VOLATILE, volatile) SEP \
	X(WHILE, while)

typedef enum AncTokenType {
	ANC_TOKEN_TYPE_EOF = -1,
	ANC_TOKEN_TYPE_ERROR = 0,
	ANC_TOKEN_TYPE_IDENT,
	ANC_TOKEN_TYPE_INTLIT,
	ANC_TOKEN_TYPE_STRING,
	ANC_TOKEN_TYPE_CHARLIT, // 
	ANC_TOKEN_TYPE_PLUS_EQ, // +=
	ANC_TOKEN_TYPE_MINUS_EQ, // -=
	ANC_TOKEN_TYPE_STAR_EQ, // *=
	ANC_TOKEN_TYPE_SLASH_EQ, // /=
	ANC_TOKEN_TYPE_PERC_EQ, // %=
	ANC_TOKEN_TYPE_AMP_EQ, // &=
	ANC_TOKEN_TYPE_BAR_EQ, // |=
	ANC_TOKEN_TYPE_CIRC_EQ, // ^=
	ANC_TOKEN_TYPE_LTLT_EQ, // <<=
	ANC_TOKEN_TYPE_GTGT_EQ, // >>=

	ANC_TOKEN_TYPE_PLUS_PLUS, // ++
	ANC_TOKEN_TYPE_MINUS_MINUS, // --

	ANC_TOKEN_TYPE_LTLT, // <<
	ANC_TOKEN_TYPE_GTGT, // >>
	ANC_TOKEN_TYPE_HASHHASH, // ##
	ANC_TOKEN_TYPE_EQUALEQUAL, // ==
	ANC_TOKEN_TYPE_AMPAMP, // &&
	ANC_TOKEN_TYPE_BARBAR, // ||

	ANC_TOKEN_TYPE_EXC_EQ, // !=
	ANC_TOKEN_TYPE_LT_EQ, // <=
	ANC_TOKEN_TYPE_GT_EQ, // >=
	ANC_TOKEN_TYPE_ARROW, // ->
	ANC_TOKEN_TYPE_ELLIPSIS, // ...

#define X(NAME, KW) ANC_TOKEN_TYPE_##NAME
	ANC_X_TOKEN_TYPE_KEYWORDS_(X, ANC_X__COMMA_),
#undef X

	ANC_TOKEN_TYPE_PLUS = '+', // +
	ANC_TOKEN_TYPE_MINUS = '-', // -
	ANC_TOKEN_TYPE_STAR = '*', // *
	ANC_TOKEN_TYPE_SLASH = '/', // /
	ANC_TOKEN_TYPE_PERC = '%', // %
	ANC_TOKEN_TYPE_TILDE = '~', // ~
	ANC_TOKEN_TYPE_AMP = '&', // &
	ANC_TOKEN_TYPE_BAR = '|', // |
	ANC_TOKEN_TYPE_CIRC = '^', // ^
	ANC_TOKEN_TYPE_HASH = '#', // #
	ANC_TOKEN_TYPE_EQUAL = '=', // =
	ANC_TOKEN_TYPE_EXC = '!', // !
	ANC_TOKEN_TYPE_LT = '<', // <
	ANC_TOKEN_TYPE_GT = '>', // >
	ANC_TOKEN_TYPE_DOT = '.', // .
	ANC_TOKEN_TYPE_LBRACKET = '[', // [
	ANC_TOKEN_TYPE_RBRACKET = ']', // ]
	ANC_TOKEN_TYPE_LPAREN = '(', // (
	ANC_TOKEN_TYPE_RPAREN = ')', // )
	ANC_TOKEN_TYPE_LBRACE = '{', // {
	ANC_TOKEN_TYPE_RBRACE = '}', // }
	ANC_TOKEN_TYPE_QUEST = '?', // ?
	ANC_TOKEN_TYPE_COLON = ':', // :
	ANC_TOKEN_TYPE_SEMICOLON = ';', // ;
	ANC_TOKEN_TYPE_COMMA = ',', // ,
} AncTokenType;

AncTokenType AncTokenType_FromKeyword(const char *keyword);

typedef struct AncSourcePosition {
	unsigned int line;
	unsigned int column;
} AncSourcePosition;

static inline bool AncSourcePosition_Equal(AncSourcePosition self, AncSourcePosition other) {
	return self.line == other.line && self.column == other.column;
}

#define ANC_SOURCE_SPAN_SAME(POS) (AncSourceSpan){ (POS), (POS) }

typedef struct AncSourceSpan {
	AncSourcePosition start, end;
} AncSourceSpan;

static inline bool AncSourceSpan_Equal(AncSourceSpan *self, AncSourceSpan *other) {
	assert(self != NULL);
	assert(other != NULL);
	return AncSourcePosition_Equal(self->start, other->start) && AncSourcePosition_Equal(self->end, other->end);
}

typedef struct AncToken {
	AncTokenType type;
	AncArenaStringView value;
	AncSourceSpan span;
} AncToken;

typedef struct AncInputFile {
	AnchAllocator *allocator;
	AnchUtf8ReadStream *stream;
	AncSourcePosition position;
	AnchDynArray_Type(AncString) lines;
	uintptr_t lineLenOffset;
	const char *filename;
} AncInputFile;

void AncInputFile_Init(AncInputFile *self, AnchAllocator *allocator, AnchUtf8ReadStream *input, const char *filename);
void AncInputFile_Free(AncInputFile *self);
void AncInputFile_ReportError(AncInputFile *self, bool show, const AncSourceSpan *span, const char *fmt, ...)
	__attribute__((format(printf, 4, 5)));
	
#define ANC_INPUT_FILE_EOF ANCH_UTF8_STREAM_EOF

typedef struct AncLexer {
	AnchAllocator *allocator;
	AncInputFile *input;
	AnchArena tokenValues;
	AnchDynArray_Type(AncToken) tokens;
	AnchDynArray_Type(intptr_t) tokenPeekBuf;
} AncLexer;

void AncLexer_Init(AncLexer *self, AnchAllocator *allocator, AncInputFile *input);
void AncLexer_Free(AncLexer *self);
AncToken *AncLexer_Read(AncLexer *self);

#endif
