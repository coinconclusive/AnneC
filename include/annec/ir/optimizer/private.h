#ifndef ANNEC_IR_OPTIMIZER_PRIVATE_HEADER_
#define ANNEC_IR_OPTIMIZER_PRIVATE_HEADER_

#include <annec/common.h>
#include <annec/ir/instr.h>

typedef AnnecUInt8 AcirOptimizerBindingFlags;
enum AcirOptimizerBindingFlags {
  ACIR_OPTIMIZER_BINDING_FLAG_NONE = 0,
  ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT = 1 << 0,
  ACIR_OPTIMIZER_BINDING_FLAG_MAX_ = 0xFF,
};

typedef struct {
  bool exists, used;
  AcirOptimizerBindingFlags flags;
  AcirImmediateValue constant;
} AcirOptimizer_Binding;

typedef struct {
  bool exists;
  bool used;
  size_t prev;
} AcirOptimizer_Instr;

typedef struct AcirOptimizer_Private_ {
  size_t instrCount;
  AcirOptimizer_Instr *instrs;
  size_t bindingCount;
  AcirOptimizer_Binding *bindings;
  size_t lastInstr;
} AcirOptimizer_Private_;

struct AcirOptimizer;

AcirOptimizer_Binding *AcirOptPriv_BindingGetOrCreate_(AcirOptimizer_Private_ *self, size_t index);
void AcirOptPriv_AllocAtLeast_Instrs_(AcirOptimizer_Private_ *self, size_t count, AnchAllocator *allocator);
int AcirOptPriv_GetOperands_(const AcirInstr *instr, const AcirOperand **ops);
const AcirImmediateValue *AirOptPriv_MaybeGetImmValue_(const AcirOptimizer_Private_ *self, const AcirOperand *op);
void AcirOptPriv_ConstEvalInstr_(const struct AcirOptimizer *self, AcirImmediateValue *result, const AcirInstr *instr);

#define SWITCH_EVAL_CASE_(CASE, O, ARG, ...) case CASE: O(CASE, ARG __VA_OPT__(,) __VA_ARGS__); break
#define SWITCH_EVAL_TYPEI_(TYPE, O, ...) switch(TYPE) { \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_SINT64, O, sint64 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_UINT64, O, uint64 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_SINT32, O, sint32 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_UINT32, O, uint32 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_SINT16, O, sint16 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_UINT16, O, uint16 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_SINT8, O, sint8 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_UINT8, O, uint8 __VA_OPT__(,) __VA_ARGS__); \
  default: assert(false && "switch case not implemented"); break; }
#define SWITCH_EVAL_TYPE_(TYPE, O, ...) switch(TYPE) { \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_SINT64, O, sint64 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_UINT64, O, uint64 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_SINT32, O, sint32 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_UINT32, O, uint32 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_SINT16, O, sint16 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_UINT16, O, uint16 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_SINT8, O, sint8 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_UINT8, O, uint8 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_FLOAT32, O, float32 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_FLOAT64, O, float64 __VA_OPT__(,) __VA_ARGS__); \
  SWITCH_EVAL_CASE_(ACIR_BASIC_VALUE_TYPE_BOOL, O, boolean __VA_OPT__(,) __VA_ARGS__); \
  default: assert(false && "switch case not implemented"); break; }

#endif // ANNEC_IR_OPTIMIZER_PRIVATE_HEADER_
