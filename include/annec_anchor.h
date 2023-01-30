#ifndef ANNEC_ANCHOR_H
#define ANNEC_ANCHOR_H
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANCH_OWN /* pointer is owned by the surrounding object. */
#define ANCH_NULLABLE(T) T /* pointer being NULL is valid behaviour. */

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
  return self->alloc(self, size);
}

static inline void *AnchAllocator_AllocZero(AnchAllocator *self, size_t size) {
  if(self->allocZero) return self->allocZero(self, size);
  void *ptr = AnchAllocator_Alloc(self, size);
  memset(ptr, 0, size);
  return ptr;
}

static inline void AnchAllocator_Free(AnchAllocator *self, void *ptr) {
  if(self->free) self->free(self, ptr);
}

static inline void *AnchAllocator_Realloc(AnchAllocator *self, void *ptr, size_t size) {
  if(self->realloc) return self->realloc(self, size, ptr);
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

typedef struct AnchCharWriteStream AnchCharWriteStream;
typedef void AnchCharWriteStream_WriteFunc(AnchCharWriteStream *self, char c);
struct AnchCharWriteStream {
  AnchCharWriteStream_WriteFunc *write;
};

typedef struct AnchCharReadStream AnchCharReadStream;
typedef char AnchCharReadStream_ReadFunc(AnchCharReadStream *self);
struct AnchCharReadStream {
  AnchCharReadStream_ReadFunc *read;
};

typedef struct {
  AnchCharReadStream read;
  AnchCharWriteStream write;
} AnchCharReadWriteStream;

void AnchWriteString(AnchCharWriteStream *out, const char *string);
void AnchWriteFormatV(AnchCharWriteStream *out, const char *format, va_list va);

static inline void AnchWriteFormat(AnchCharWriteStream *out, const char *format, ...) {
  va_list va;
  va_start(va, format);
  AnchWriteFormatV(out, format, va);
  va_end(va);
}

static inline void AnchRWriteFormatV(AnchCharReadWriteStream *out, const char *format, va_list va) {
  AnchWriteFormatV(&out->write, format, va);
}

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

#endif
