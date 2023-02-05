#ifndef ANNEC_IR_FUNCTION_HEADER_
#define ANNEC_IR_FUNCTION_HEADER_

#include <annec/ir/value_type.h>
#include <annec/ir/instr.h>

typedef struct {
  const AcirValueType *type;
  size_t code;
  const char *name;
  size_t instrCount;
  const AcirInstr *instrs;
} AcirFunction;

int AcirFunction_Validate(AcirFunction *self, AnchAllocator *allocator);
void AcirFunction_Print(const AcirFunction *self, AnchCharWriteStream *out);

#endif // ANNEC_IR_FUNCTION_HEADER_
