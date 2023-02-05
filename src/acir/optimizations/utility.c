#include "annec/anchor.h"
#include "annec/ir/optimizer/private.h"
#include "private.h"

#include <assert.h>

AcirOptimizer_Binding *AcirOptPriv_BindingGetOrCreate_(AcirOptimizer_Private_ *self, size_t index) {
  assert(self != NULL);
  if(index < self->bindingCount) return &self->bindings[index];

  size_t oldCount = self->bindingCount;
  self->bindingCount = index + 1;

  if(oldCount == 0) {
    // AnchWriteFormat(wsStderr, ANSI_GRAY "Bindings allocation of %zu bytes\n" ANSI_RESET,
    //   sizeof(AcirOptimizer_Binding) * self->bindingCount);
    
    self->bindings = calloc(1, // AnchAllocator_AllocZero(self->allocator,
      sizeof(AcirOptimizer_Binding) * self->bindingCount);
    
    // AnchWriteFormat(wsStderr, ANSI_GRAY "    -> %p\n" ANSI_RESET, self->bindings);
  } else {
    // AnchWriteFormat(wsStderr, ANSI_GRAY "Bindings reallocation of %zu bytes (%p)\n" ANSI_RESET,
    //   sizeof(AcirOptimizer_Binding) * self->bindingCount, self->bindings);
    
    self->bindings = realloc(self->bindings, // AnchAllocator_Realloc(self->allocator, self->bindings,
      sizeof(AcirOptimizer_Binding) * self->bindingCount);
    
    // AnchWriteFormat(wsStderr, ANSI_GRAY "    -> %p\n" ANSI_RESET, self->bindings);
    
    memset(self->bindings + oldCount, 0, sizeof(AcirOptimizer_Binding)
      * (self->bindingCount - oldCount));
  }

  return &self->bindings[index];
}

void AcirOptPriv_AllocAtLeast_Instrs_(AcirOptimizer_Private_ *self, size_t count, AnchAllocator *allocator) {
  assert(self != NULL);
  if(count <= self->instrCount) return;

  size_t oldCount = self->instrCount;
  self->instrCount = count;

  if(oldCount == 0) {
    self->instrs = AnchAllocator_AllocZero(allocator,
      sizeof(AcirOptimizer_Instr) * self->instrCount);
  } else {
    self->instrs = AnchAllocator_Realloc(allocator,
      self->instrs, sizeof(AcirOptimizer_Instr) * self->instrCount);
    memset(self->instrs + oldCount, 0, sizeof(AcirOptimizer_Instr)
      * (self->instrCount - oldCount));
  }
}

int AcirOptPriv_GetOperands_(const AcirInstr *instr, const AcirOperand **ops) {
  assert(ops != NULL);
  switch(AcirOpcode_OperandCount(instr->opcode)) {
    case 1:
      ops[0] = &instr->val;
      return 1;
    case 2:
      ops[0] = &instr->val;
      ops[1] = &instr->out;
      return 2;
    case 3:
      ops[0] = &instr->lhs;
      ops[1] = &instr->rhs;
      ops[2] = &instr->out;
      return 3;
    default: return 0;
  }
}

const AcirImmediateValue *AirOptPriv_MaybeGetImmValue_(const AcirOptimizer_Private_ *self, const AcirOperand *op) {
  assert(self != NULL);
  assert(op != NULL);
  if(op->type == ACIR_OPERAND_TYPE_IMMEDIATE) return &op->imm;
  if(op->type == ACIR_OPERAND_TYPE_BINDING) {
    assert(op->idx < self->bindingCount); // TODO: BindingExists_(self, op->idx);
    assert(self->bindings[op->idx].exists);
    if(self->bindings[op->idx].flags & ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT)
      return &self->bindings[op->idx].constant;
  }
  return NULL;
}

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

void AcirOptPriv_ConstEvalInstr_(const AcirOptimizer *self, AcirImmediateValue *result, const AcirInstr *instr) {
  assert(self != NULL);
  assert(instr != NULL);
  assert(result != NULL);

  const AcirImmediateValue *val = NULL, *lhs = NULL, *rhs = NULL;
  
  int operandCount = AcirOpcode_OperandCount(instr->opcode);
  if(operandCount == 2) {
    val = AirOptPriv_MaybeGetImmValue_(self->private, &instr->val);
  } else if(operandCount == 3) {
    lhs = AirOptPriv_MaybeGetImmValue_(self->private, &instr->lhs);
    rhs = AirOptPriv_MaybeGetImmValue_(self->private, &instr->rhs);
  }

  result->type = instr->type;

  switch(instr->opcode) {
#define BINOP(TYPE, FIELD, OP) result->FIELD = lhs->FIELD OP rhs->FIELD;
#define BBINOP(TYPE, FIELD, OP) result->boolean = lhs->FIELD OP rhs->FIELD;
#define BUNOP(TYPE, FIELD, OP) result->boolean = OP(val->FIELD);
#define UNOP(TYPE, FIELD, OP) result->FIELD = OP(val->FIELD);
    case ACIR_OPCODE_ADD: SWITCH_EVAL_TYPE_(instr->type->basic, BINOP, +) break;
    case ACIR_OPCODE_SUB: SWITCH_EVAL_TYPE_(instr->type->basic, BINOP, -) break;
    case ACIR_OPCODE_MUL: SWITCH_EVAL_TYPE_(instr->type->basic, BINOP, *) break;
    case ACIR_OPCODE_DIV: SWITCH_EVAL_TYPE_(instr->type->basic, BINOP, /) break;
    case ACIR_OPCODE_MOD: SWITCH_EVAL_TYPEI_(instr->type->basic, BINOP, %) break;
    case ACIR_OPCODE_NEG: SWITCH_EVAL_TYPE_(instr->type->basic, UNOP, -) break;
    case ACIR_OPCODE_EQL: SWITCH_EVAL_TYPE_(instr->type->basic, BBINOP, ==) break;
    case ACIR_OPCODE_NEQ: SWITCH_EVAL_TYPE_(instr->type->basic, BBINOP, !=) break;
    case ACIR_OPCODE_LTH: SWITCH_EVAL_TYPE_(instr->type->basic, BBINOP, <) break;
    case ACIR_OPCODE_GTH: SWITCH_EVAL_TYPE_(instr->type->basic, BBINOP, >) break;
    case ACIR_OPCODE_LEQ: SWITCH_EVAL_TYPE_(instr->type->basic, BBINOP, <=) break;
    case ACIR_OPCODE_GEQ: SWITCH_EVAL_TYPE_(instr->type->basic, BBINOP, >=) break;
    case ACIR_OPCODE_AND: BBINOP(, boolean, &&) break;
    case ACIR_OPCODE_COR: BBINOP(, boolean, ||) break;
    case ACIR_OPCODE_XOR: BBINOP(, boolean, ^) break;
    case ACIR_OPCODE_NOT: BUNOP(, boolean, !) break;
    case ACIR_OPCODE_BIT_AND: SWITCH_EVAL_TYPEI_(instr->type->basic, BINOP, &) break;
    case ACIR_OPCODE_BIT_COR: SWITCH_EVAL_TYPEI_(instr->type->basic, BINOP, |) break;
    case ACIR_OPCODE_BIT_XOR: SWITCH_EVAL_TYPEI_(instr->type->basic, BINOP, ^) break;
    case ACIR_OPCODE_BIT_NOT: SWITCH_EVAL_TYPEI_(instr->type->basic, UNOP, ~) break;
#undef BINOP
    default:
      assert(false && "non-consteval instruction");
      return;
  }
}
