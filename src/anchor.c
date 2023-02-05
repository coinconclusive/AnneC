#include <annec/anchor.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

void AnchFileWriteStream_Write(AnchCharWriteStream *self, char c) {
  assert(self != NULL);
  fputc(c, ((AnchFileWriteStream*)self)->handle);
}

void AnchFileWriteStream_Init(AnchFileWriteStream *self) {
  assert(self != NULL);
  self->stream.write = &AnchFileWriteStream_Write;
  self->handle = NULL;
}

void AnchFileWriteStream_InitWith(AnchFileWriteStream *self, FILE *file) {
  assert(self != NULL);
  self->stream.write = &AnchFileWriteStream_Write;
  self->handle = file;
}

void AnchFileWriteStream_Open(AnchFileWriteStream *self, const char *filename) {
  assert(self != NULL);
  assert(self->handle == NULL);
  assert(filename != NULL);

  self->handle = fopen(filename, "w");
}

void AnchFileWriteStream_Close(AnchFileWriteStream *self) {
  assert(self != NULL);
  assert(self->handle != NULL);

  fclose(self->handle);
  self->handle = NULL;
}

char AnchFileReadStream_Read(AnchCharReadStream *self) {
  assert(self != NULL);
  return fgetc(((AnchFileReadStream*)self)->handle);
}

void AnchFileReadStream_Init(AnchFileReadStream *self) {
  self->stream.read = &AnchFileReadStream_Read;
  self->handle = NULL;
}

void AnchFileReadStream_InitWith(AnchFileReadStream *self, FILE *file) {
  self->stream.read = &AnchFileReadStream_Read;
  self->handle = file;
}

void AnchFileReadStream_Open(AnchFileReadStream *self, const char *filename) {
  assert(self != NULL);
  assert(self->handle == NULL);
  assert(filename != NULL);

  self->handle = fopen(filename, "r");
}

void AnchFileReadStream_Close(AnchFileReadStream *self) {
  assert(self != NULL);
  assert(self->handle != NULL);

  fclose(self->handle);
  self->handle = NULL;
}

void AnchWriteString(AnchCharWriteStream *out, const char *string) {
  if(string == NULL) return AnchWriteString(out, "(null)");
  while(*string) out->write(out, *(string++));
}

void AnchWriteFormatV(AnchCharWriteStream *out, const char *format, va_list va) {
  va_list va2;
  va_copy(va2, va);
  size_t size = vsnprintf(NULL, 0, format, va2);
  char str[size + 1];
  vsnprintf(str, size + 1, format, va);
  AnchWriteString(out, str);
}

void AnchStatsAllocator_Init(AnchStatsAllocator *self, AnchAllocator *allocator) {
  assert(self != NULL);
  self->base.alloc = &AnchStatsAllocator_Alloc;
  self->base.allocZero = &AnchStatsAllocator_AllocZero;
  self->base.realloc = &AnchStatsAllocator_Realloc;
  self->base.free = &AnchStatsAllocator_Free;
  self->allocator = allocator;
  self->allocCount = 0;
  self->reallocCount = 0;
  self->freeCount = 0;
}

void AnchDefaultAllocator_Init(AnchDefaultAllocator *self) {
  self->alloc = &AnchDefaultAllocator_Alloc;
  self->allocZero = &AnchDefaultAllocator_AllocZero;
  self->realloc = &AnchDefaultAllocator_Realloc;
  self->free = &AnchDefaultAllocator_Free;
}

void *AnchDefaultAllocator_Alloc(AnchAllocator *self, size_t size) {
  return malloc(size);
}

void *AnchDefaultAllocator_AllocZero(AnchAllocator *self, size_t size) {
  return calloc(1, size);
}

void *AnchDefaultAllocator_Realloc(AnchAllocator *self, size_t size, void *ptr) {
  return realloc(ptr, size);
}

void AnchDefaultAllocator_Free(AnchAllocator *self, void *ptr) {
  return free(ptr);
}


void *AnchStatsAllocator_Alloc(AnchAllocator *self_, size_t size) {
  AnchStatsAllocator *self = (AnchStatsAllocator *)self_;
  ++self->allocCount;
  return self->allocator->alloc(self->allocator, size);
}

void *AnchStatsAllocator_AllocZero(AnchAllocator *self_, size_t size) {
  AnchStatsAllocator *self = (AnchStatsAllocator *)self_;
  ++self->allocZeroCount;
  return self->allocator->allocZero(self->allocator, size);
}

void *AnchStatsAllocator_Realloc(AnchAllocator *self_, size_t size, void *ptr) {
  AnchStatsAllocator *self = (AnchStatsAllocator *)self_;
  ++self->reallocCount;
  return self->allocator->realloc(self->allocator, size, ptr);
}

void AnchStatsAllocator_Free(AnchAllocator *self_, void *ptr) {
  AnchStatsAllocator *self = (AnchStatsAllocator *)self_;
  ++self->freeCount;
  return self->allocator->free(self->allocator, ptr);
}

