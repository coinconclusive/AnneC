#ifndef ANNEC_ANCHOR_H
#define ANNEC_ANCHOR_H
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <uchar.h>
#include <wchar.h>

#define ANCH_OWN /* pointer is owned by the surrounding object. */
#define ANCH_NULLABLE(T) T /* pointer being NULL is valid behaviour. */

/** Check if argument is a power of two. N is evaluated multiple times. */
#define ANCH_IS_POWEROF2(N) ((N) && (((N) & ((N) - 1)) == 0))

#define ANCH__X_COMMON_TYPES__(X, SEP) \
  X(size, size_t) SEP \
  X(uint64, uint64_t) SEP \
  X(int64, int64_t) SEP \
  X(uint32, uint32_t) SEP \
  X(int32, int32_t) SEP \
  X(uint16, uint16_t) SEP \
  X(int16, int16_t) SEP \
  X(uint8, uint8_t) SEP \
  X(int8, int8_t) SEP \
  X(uint, unsigned int) SEP \
  X(int, int)


#define X(NAME, TYPE) \
  static inline bool AnchIsPowerOf2_##NAME(TYPE x) { return ANCH_IS_POWEROF2(x); }
ANCH__X_COMMON_TYPES__(X, );
#undef X

#define ANCH__XF__ANCH_IS_POWEROF2_GENERIC_(NAME, TYPE) TYPE: AnchIsPowerOf2_##NAME
#define AnchIsPowerOf2(N) \
  _Generic((N), \
    ANCH__X_COMMON_TYPES__(ANCH__XF__ANCH_IS_POWEROF2_GENERIC_,), \
    default: AnchIsPowerOf2_size\
  )(N)

/** Round up N to closest power of two P. P is evaluated multiple times. */
#define ANCH_ROUNDUP_POWEROF2(N, P) ({ assert(ANCH_IS_POWEROF2(P)); (N + P - 1) & -P; })

/** Round up N to closest multiple of P. N >= 0. P > 0. P is evaluated multiple times. */
#define ANCH_ROUNDUP(N, P) ({ assert(P != 0); (N + P - 1) & -P; })

typedef struct AnchAllocator AnchAllocator;

typedef void *AnchAllocator_AllocFunc(AnchAllocator *self, size_t size);
typedef void *AnchAllocator_AllocZeroFunc(AnchAllocator *self, size_t size);
// TODO: flip ptr and size
typedef void *AnchAllocator_ReallocFunc(AnchAllocator *self, size_t size, void *ptr);
typedef void AnchAllocator_FreeFunc(AnchAllocator *self, void *ptr);

struct AnchAllocator {
  AnchAllocator_AllocFunc *alloc;
  ANCH_NULLABLE(AnchAllocator_AllocZeroFunc *) allocZero;
  ANCH_NULLABLE(AnchAllocator_ReallocFunc *) realloc;
  ANCH_NULLABLE(AnchAllocator_FreeFunc *) free;
};

static void *AnchAllocator_Alloc(AnchAllocator *self, size_t size)
  __attribute__((malloc)) __attribute__((alloc_size(2)));

static void *AnchAllocator_AllocZero(AnchAllocator *self, size_t size)
  __attribute__((malloc)) __attribute__((alloc_size(2)));

static inline void *AnchAllocator_Alloc(AnchAllocator *self, size_t size) {
  assert(self != NULL);
  return self->alloc(self, size);
}

static inline void *AnchAllocator_AllocZero(AnchAllocator *self, size_t size) {
  assert(self != NULL);
  if(self->allocZero) return self->allocZero(self, size);
  void *ptr = AnchAllocator_Alloc(self, size);
  memset(ptr, 0, size);
  return ptr;
}

static inline void AnchAllocator_Free(AnchAllocator *self, void *ptr) {
  assert(self != NULL);
  if(self->free) self->free(self, ptr);
}

static inline void *AnchAllocator_Realloc(AnchAllocator *self, void *ptr, size_t size) {
  assert(self != NULL);
  if(self->realloc != NULL) return self->realloc(self, size, ptr);
  void *new = AnchAllocator_Alloc(self, size);
  memcpy(new, ptr, size);
  AnchAllocator_Free(self, ptr);
  return new;
}

typedef AnchAllocator AnchDefaultAllocator;
extern AnchAllocator_AllocFunc AnchDefaultAllocator_Alloc;
extern AnchAllocator_AllocZeroFunc AnchDefaultAllocator_AllocZero;
extern AnchAllocator_ReallocFunc AnchDefaultAllocator_Realloc;
extern AnchAllocator_FreeFunc AnchDefaultAllocator_Free;
void AnchDefaultAllocator_Init(AnchDefaultAllocator *allocator);

typedef struct {
  AnchAllocator base;
  AnchAllocator *allocator;
  size_t allocCount;
  size_t allocZeroCount;
  size_t reallocCount;
  size_t freeCount;
} AnchStatsAllocator;

void AnchStatsAllocator_Init(AnchStatsAllocator *self, AnchAllocator *allocator);
extern AnchAllocator_AllocFunc AnchStatsAllocator_Alloc;
extern AnchAllocator_AllocZeroFunc AnchStatsAllocator_AllocZero;
extern AnchAllocator_ReallocFunc AnchStatsAllocator_Realloc;
extern AnchAllocator_FreeFunc AnchStatsAllocator_Free;

//////////////////////////////////////////////////////////////////////////////////////////

/** Arena allocator. Or stack allocator. */
typedef struct AnchArena {
  AnchAllocator *allocator;
  uint8_t *data;
  size_t size;
  size_t allocated;
} AnchArena;

void AnchArena_Init(AnchArena *self, AnchAllocator *allocator, size_t prealloc);
void AnchArena_Free(AnchArena *self);
void *AnchArena_Push(AnchArena *self, size_t size);
void AnchArena_Pop(AnchArena *self, size_t size);

/** Get pointer to the last element of an AnchArena* ARENA. ARENA is evaluated multiple times. */
#define ANCH_ARENA_LAST(ARENA) (void*)((ARENA)->data + (ARENA)->size)

//////////////////////////////////////////////////////////////////////////////////////////

typedef struct AnchArena AnchDynArray;
// TODO: implement anchdynarray through an arena
#define AnchDynArray_Init(self, allocator, prealloc) AnchArena_Init(self, allocator, prealloc)
#define AnchDynArray_Free(self) AnchArena_Free(self)
#define AnchDynArray_Push(self, size) AnchArena_Push(self, size)
#define AnchDynArray_Pop(self, size) AnchArena_Pop(self, size)

#define AnchDynArray_Type(TYPE) AnchDynArray

#define ANCH_DYNARRAY_PUSH(DYNARR, TYPE, VALUE) *(TYPE*)AnchDynArray_Push((DYNARR), sizeof(TYPE)) = (VALUE)

//////////////////////////////////////////////////////////////////////////////////////////

typedef struct AnchCharWriteStream AnchCharWriteStream;
typedef void AnchCharWriteStream_WriteFunc(AnchCharWriteStream *self, int c);
struct AnchCharWriteStream {
  AnchCharWriteStream_WriteFunc *write;
};

typedef struct AnchCharReadStream AnchCharReadStream;
typedef int AnchCharReadStream_ReadFunc(AnchCharReadStream *self);
struct AnchCharReadStream {
  AnchCharReadStream_ReadFunc *read;
};

static inline int AnchCharReadStream_Read(AnchCharReadStream *self) {
  assert(self != NULL);
  return self->read(self);
}

static inline void AnchCharWriteStream_Write(AnchCharWriteStream *self, char value) {
  assert(self != NULL);
  self->write(self, value);
}

//////////////////////////////////////////////////////////////////////////////////////////

typedef struct AnchByteWriteStream AnchByteWriteStream;
typedef void AnchByteWriteStream_WriteFunc(AnchByteWriteStream *self, uint8_t c);
struct AnchByteWriteStream {
  AnchByteWriteStream_WriteFunc *write;
};

typedef struct AnchByteReadStream AnchByteReadStream;
typedef uint8_t AnchByteReadStream_ReadFunc(AnchByteReadStream *self);
struct AnchByteReadStream {
  AnchByteReadStream_ReadFunc *read;
};

static inline uint8_t AnchByteReadStream_Read(AnchByteReadStream *self) {
  assert(self != NULL);
  return self->read(self);
}

static inline void AnchByteWriteStream_Write(AnchByteWriteStream *self, uint8_t value) {
  assert(self != NULL);
  self->write(self, value);
}

//////////////////////////////////////////////////////////////////////////////////////////

typedef AnchByteReadStream AnchUtf8ReadStream;
typedef AnchByteWriteStream AnchUtf8WriteStream;

#define ANCH_UTF8_STREAM_EOF ((char32_t)0)
#define ANCH_UTF8_STREAM_ERROR (~(char32_t)0)

char32_t AnchUtf8ReadStream_Read(AnchUtf8ReadStream *self);
void AnchUtf8WriteStream_Write(AnchUtf8WriteStream *self, char32_t value);

//////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
  AnchCharReadStream read;
  AnchCharWriteStream write;
} AnchCharReadWriteStream;

typedef struct {
  AnchByteReadStream read;
  AnchByteWriteStream write;
} AnchByteReadWriteStream;

typedef struct {
  AnchUtf8ReadStream read;
  AnchUtf8WriteStream write;
} AnchUtfReadWriteStream;

//////////////////////////////////////////////////////////////////////////////////////////

void AnchWriteString(AnchCharWriteStream *out, const char *string);
void AnchWriteFormatV(AnchCharWriteStream *out, const char *format, va_list va);

static inline void AnchWriteFormat(AnchCharWriteStream *out, const char *format, ...)
  __attribute__((__format__(printf, 2, 3)));

static inline void AnchWriteFormat(AnchCharWriteStream *out, const char *format, ...) {
  va_list va;
  va_start(va, format);
  AnchWriteFormatV(out, format, va);
  va_end(va);
}

static inline void AnchRWriteFormatV(AnchCharReadWriteStream *out, const char *format, va_list va) {
  AnchWriteFormatV(&out->write, format, va);
}

static inline void AnchRWriteFormat(AnchCharReadWriteStream *out, const char *format, ...)
  __attribute__((__format__(printf, 2, 3)));

static inline void AnchRWriteFormat(AnchCharReadWriteStream *out, const char *format, ...) {
  va_list va;
  va_start(va, format);
  AnchRWriteFormatV(out, format, va);
  va_end(va);
}

static inline void AnchRWriteString(AnchCharReadWriteStream *out, const char *string) {
  AnchWriteString(&out->write, string);
}

typedef struct {
  AnchCharWriteStream stream;
  FILE *handle;
} AnchFileWriteStream;

typedef struct {
  AnchCharReadStream stream;
  FILE *handle;
} AnchFileReadStream;

typedef struct {
  AnchCharReadWriteStream stream;
  FILE *handle;
} AnchFileReadWriteStream;

extern AnchCharWriteStream_WriteFunc AnchFileWriteStream_Write;
void AnchFileWriteStream_Init(AnchFileWriteStream *self);
void AnchFileWriteStream_InitWith(AnchFileWriteStream *self, FILE *file);
void AnchFileWriteStream_Open(AnchFileWriteStream *self, const char *filename);
void AnchFileWriteStream_Close(AnchFileWriteStream *self);

extern AnchCharReadStream_ReadFunc AnchFileReadStream_Read;
void AnchFileReadStream_Init(AnchFileReadStream *self);
void AnchFileReadStream_InitWith(AnchFileReadStream *self, FILE *file);
void AnchFileReadStream_Open(AnchFileReadStream *self, const char *filename);
void AnchFileReadStream_Close(AnchFileReadStream *self);

typedef struct {
  AnchByteWriteStream stream;
  FILE *handle;
} AnchByteFileWriteStream;

typedef struct {
  AnchByteReadStream stream;
  FILE *handle;
} AnchByteFileReadStream;

typedef struct {
  AnchByteReadWriteStream stream;
  FILE *handle;
} AnchByteFileReadWriteStream;

extern AnchByteWriteStream_WriteFunc AnchByteFileWriteStream_Write;
void AnchByteFileWriteStream_Init(AnchByteFileWriteStream *self);
void AnchByteFileWriteStream_InitWith(AnchByteFileWriteStream *self, FILE *file);
void AnchByteFileWriteStream_Open(AnchByteFileWriteStream *self, const char *filename);
void AnchByteFileWriteStream_Close(AnchByteFileWriteStream *self);

extern AnchByteReadStream_ReadFunc AnchByteFileReadStream_Read;
void AnchByteFileReadStream_Init(AnchByteFileReadStream *self);
void AnchByteFileReadStream_InitWith(AnchByteFileReadStream *self, FILE *file);
void AnchByteFileReadStream_Open(AnchByteFileReadStream *self, const char *filename);
void AnchByteFileReadStream_Close(AnchByteFileReadStream *self);

#endif
