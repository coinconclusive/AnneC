#include <annec_anchor.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////////////////

void AnchFileWriteStream_Write(AnchCharWriteStream *self, int c) {
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

void AnchFileWriteStream_Rewind(AnchFileWriteStream *self) {
  assert(self != NULL);
  assert(self->handle != NULL);
  
  rewind(self->handle);
}

//////////////////////////////////////////////////////////////////////////////////////////

int AnchFileReadStream_Read(AnchCharReadStream *self) {
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

void AnchFileReadStream_Rewind(AnchFileReadStream *self) {
  assert(self != NULL);
  assert(self->handle != NULL);

  rewind(self->handle);
}

//////////////////////////////////////////////////////////////////////////////////////////

void AnchByteFileWriteStream_Write(AnchByteWriteStream *self, uint8_t c) {
  assert(self != NULL);
  fputc(c, ((AnchByteFileWriteStream*)self)->handle);
}

void AnchByteFileWriteStream_Init(AnchByteFileWriteStream *self) {
  assert(self != NULL);
  self->stream.write = &AnchByteFileWriteStream_Write;
  self->handle = NULL;
}

void AnchByteFileWriteStream_InitWith(AnchByteFileWriteStream *self, FILE *file) {
  assert(self != NULL);
  self->stream.write = &AnchByteFileWriteStream_Write;
  self->handle = file;
}

void AnchByteFileWriteStream_Open(AnchByteFileWriteStream *self, const char *filename) {
  assert(self != NULL);
  assert(self->handle == NULL);
  assert(filename != NULL);

  self->handle = fopen(filename, "wb");
}

void AnchByteFileWriteStream_Close(AnchByteFileWriteStream *self) {
  assert(self != NULL);
  assert(self->handle != NULL);

  fclose(self->handle);
  self->handle = NULL;
}

void AnchByteFileWriteStream_Rewind(AnchByteFileWriteStream *self) {
  assert(self != NULL);
  assert(self->handle != NULL);
  
  rewind(self->handle);
}

//////////////////////////////////////////////////////////////////////////////////////////

uint8_t AnchByteFileReadStream_Read(AnchByteReadStream *self) {
  assert(self != NULL);
  return fgetc(((AnchFileReadStream*)self)->handle);
}

void AnchByteFileReadStream_Init(AnchByteFileReadStream *self) {
  self->stream.read = &AnchByteFileReadStream_Read;
  self->handle = NULL;
}

void AnchByteFileReadStream_InitWith(AnchByteFileReadStream *self, FILE *file) {
  self->stream.read = &AnchByteFileReadStream_Read;
  self->handle = file;
}

void AnchByteFileReadStream_Open(AnchByteFileReadStream *self, const char *filename) {
  assert(self != NULL);
  assert(self->handle == NULL);
  assert(filename != NULL);

  self->handle = fopen(filename, "rb");
}

void AnchByteFileReadStream_Close(AnchByteFileReadStream *self) {
  assert(self != NULL);
  assert(self->handle != NULL);

  fclose(self->handle);
  self->handle = NULL;
}

void AnchByteFileReadStream_Rewind(AnchByteFileReadStream *self) {
  assert(self != NULL);
  assert(self->handle != NULL);

  rewind(self->handle);
}

//////////////////////////////////////////////////////////////////////////////////////////

char32_t AnchUtf8ReadStream_Read(AnchUtf8ReadStream *self) {
  assert(self != NULL);
  mbstate_t state = {};
  char buf[MB_CUR_MAX];
  char32_t c32;
  for(int i = 0; i < MB_CUR_MAX; ++i) {
    buf[i] = AnchByteReadStream_Read(self);
    if(i == 0 && buf[i] == EOF) return ANCH_UTF8_STREAM_EOF;
    size_t v = mbrtoc32(&c32, buf, i + 1, &state);
    assert(v != (size_t)-3); // must be UTF-32
    if(v == (size_t)-1) return ANCH_UTF8_STREAM_ERROR;
    if(v >= 0) break;
  }
  return c32;
}

void AnchUtf8WriteStream_Write(AnchUtf8WriteStream *self, char32_t value) {
  assert(self != NULL);
	uint8_t data[MB_CUR_MAX];
	mbstate_t ps = {};
	int len = c32rtomb((char*)data, value, &ps);
	assert(len != -1);
  for(int i = 0; i < len; ++i)
    AnchByteWriteStream_Write(self, data[i]);
}

//////////////////////////////////////////////////////////////////////////////////////////

void AnchWriteString(AnchCharWriteStream *out, const char *string) {
  if(string == NULL) return AnchWriteString(out, "(null)");
  while(*string) out->write(out, *(string++));
}

size_t AnchWriteFormatV(AnchCharWriteStream *out, const char *format, va_list va) {
  va_list va2;
  va_copy(va2, va);
  size_t size = vsnprintf(NULL, 0, format, va2);
  char str[size + 1];
  size_t r = vsnprintf(str, size + 1, format, va);
  AnchWriteString(out, str);
  return r;
}

// void AnchWriteFormatV(AnchCharWriteStream *out, const char *format, va_list va) {
//   for(const char *c = format; *c; ++c) {
//     if(*c == '%') {
//       ++c;
//       enum { ET_I, ET_W, ET_Z } expectType = ET_I;
//       char precchar = '\0';
//       int prec = 0;
//     read:
//       switch(*c++) {
//         case '%': out->write(out, '%'); break;
//         case 'c': out->write(out, va_arg(va, int)); break;
//         case 'd': case 'i': {
//           int len = snprintf(NULL, 0, "%d",
//               expectType == ET_I ? va_arg(va, int)
//             : expectType == ET_Z ? va_arg(va, size_t)
//             : expectType == ET_U ? va_arg(va, unsigned int));
//           char str[len + 1];
//           snprintf(str, len + 1, "%d", va_arg(va, int));
//           AnchWriteString(out, str);
//           break;
//         }
//         case 'u': break;
//         case 'z': break;
//         case '0':
//           ++c;
//           precchar = '0';
//           while(isdigit(*c)) prec = prec * 10 + *c++ - '0';
//           goto read;
//         case '.':
//           ++c;
//           precchar = '.';
//           while(isdigit(*c)) prec = prec * 10 + *c++ - '0';
//           goto read;
//       }
//     }
//   }
// }

//////////////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////////////

void AnchArena_Init(AnchArena *self, AnchAllocator *allocator, size_t step) {
  assert(self != NULL);

  self->allocator = allocator;
  self->size = 0;
  self->allocated = 0;
  self->step = step ? step : 256;
  self->data = NULL;
  if(self->allocated > 0)
    self->data = AnchAllocator_Alloc(self->allocator, self->allocated);
}

void AnchArena_Free(AnchArena *self) {
  assert(self != NULL);

  self->size = 0;
  self->step = 0;
  if(self->allocated > 0)
    AnchAllocator_Free(self->allocator, self->data);
  self->data = NULL;
  self->allocator = NULL;
}

void *AnchArena_Push(AnchArena *self, size_t size) {
  assert(self != NULL);

  if(self->size + size > self->allocated) {
    bool alloced = (self->allocated > 0);
    if(self->step > 1) {
      self->allocated += size < self->step ? self->step : size;
      self->allocated = ANCH_ROUNDUP_POWEROF2(self->allocated, self->step);
    }
    if(alloced)
      self->data = AnchAllocator_Realloc(self->allocator, self->data, self->allocated);
    else
      self->data = AnchAllocator_Alloc(self->allocator, self->allocated);
  }
  void *oldData = self->data + self->size;
  self->size += size;
  return oldData;
}

void *AnchArena_PushZeros(AnchArena *self, size_t size) {
  assert(self != NULL);

  void *addr = AnchArena_Push(self, size);
  memset(addr, 0, size);
  return addr;
}

/** Does not deallocate 'snugly'. \ref ShrinkToFit should be used to deallocate not needed bytes. */
void AnchArena_Pop(AnchArena *self, size_t size) {
  assert(self != NULL);
  assert(size <= self->size);

  self->size -= size;

  /* 256 instead of self->step because we only need to deallocate once every 256 bytes. */
  if(self->allocated - self->size > 256) {
    self->allocated -= ((self->allocated - self->size) / 256) * 256;

    if(self->step != 1) self->allocated = ANCH_ROUNDUP_POWEROF2(self->allocated, self->step);
    self->data = AnchAllocator_Realloc(self->allocator, self->data, self->allocated);
  }
}

void AnchArena_ShrinkToFit(AnchArena *self) {
  assert(self != NULL);

  if(self->size == 0) {
    if(self->allocated == 0) return;
    self->allocated = 0;
    AnchAllocator_Free(self->allocator, self->data);
    self->data = NULL;
  }

  self->allocated = self->size;
  self->data = AnchAllocator_Realloc(self->allocator, self->data, self->allocated);
}
