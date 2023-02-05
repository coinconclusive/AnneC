#ifndef ANNEC_IR_VALUE_TYPE_HEADER_
#define ANNEC_IR_VALUE_TYPE_HEADER_
#include <annec/common.h>

// O(NAME, MNEMONIC, DESCRIPTION)
#define ACIR_BASIC_VALUE_TYPES_ENUM(O, S) \
  O(SINT64, sint64, "signed 64-bit integer") S \
  O(UINT64, uint64, "unsigned 64-bit integer") S \
  O(SINT32, sint32, "signed 32-bit integer") S \
  O(UINT32, uint32, "unsigned 32-bit integer") S \
  O(SINT16, sint16, "signed 16-bit integer") S \
  O(UINT16, uint16, "unsigned 16-bit integer") S \
  O(SINT8, sint8, "signed 8-bit integer") S \
  O(UINT8, uint8, "unsigned 8-bit integer") S \
  O(FLOAT32, float32, "single-precision float") S \
  O(FLOAT64, float64, "double-precision float") S \
  O(BOOL, bool, "boolean type") S \
  O(VOID, void, "void type")

typedef uint8_t AcirBasicValueType;

#define COMMA ,
#define TMP(NAME, MNEMONIC, DESCRIPTION) ACIR_BASIC_VALUE_TYPE_##NAME
enum AcirBasicValueTypes { ACIR_BASIC_VALUE_TYPES_ENUM(TMP, COMMA), ACIR_BASIC_VALUE_TYPE_MAX_ };
#undef TMP
#undef COMMA

const char *AcirBasicValueType_Name(AcirBasicValueType type);
const char *AcirBasicValueType_Mnemonic(AcirBasicValueType type);
const char *AcirBasicValueType_Description(AcirBasicValueType type);

typedef struct AcirValueType AcirValueType;

typedef struct {
  AcirValueType *returnType;
  size_t argumentCount;
  AcirValueType *argumentTypes[];
} AcirFunctionValueType;

struct AcirValueType {
  enum {
    ACIR_VALUE_TYPE_BASIC,
    ACIR_VALUE_TYPE_POINTER,
    ACIR_VALUE_TYPE_FUNCTION,
  } type;
  union {
    AcirBasicValueType basic;
    const AcirValueType *pointer;
    const AcirFunctionValueType *function;
  };
};

/** Check if two types are equal. */
bool AcirBasicValueType_Equals(const AcirValueType *self, const AcirValueType *other);

/** Print value type to stream. */
void AcirValueType_Print(const AcirValueType *self, AnchCharWriteStream *out);

#endif // ANNEC_IR_VALUE_TYPE_HEADER_
