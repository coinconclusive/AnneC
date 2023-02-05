#ifndef ANNEC_COMMON_HEADER_
#define ANNEC_COMMON_HEADER_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint64_t AnnecUInt64;
typedef int64_t AnnecInt64;
typedef uint32_t AnnecUInt32;
typedef int32_t AnnecInt32;
typedef uint16_t AnnecUInt16;
typedef int16_t AnnecInt16;
typedef uint8_t AnnecUInt8;
typedef int8_t AnnecInt8;
typedef float AnnecFloat32;
typedef double AnnecFloat64;

typedef size_t AnnecSize;

#if !defined(ANNEC_COMMON_NO_ANCHOR) || ANNEC_COMMON_NO_ANCHOR != 0
#include <annec/anchor.h>
#endif

#endif // ANNEC_COMMON_HEADER_
