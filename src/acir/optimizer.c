#include <annec_anchor.h>
#include <acir/acir.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "cli.h"

void AcirOptimizer_Init(AcirOptimizer *self, const AcirOptimizer_InitInfo *info) {
  assert(self != NULL);
  assert(info != NULL);
  self->allocator = info->allocator;
  self->source = info->source;
  self->builder = info->builder;
  self->didAnalyze = false;
  self->bindingCount = 0;
  self->bindings = NULL;
  self->instrCount = 0;
  self->instrs = NULL;
}

void AcirOptimizer_Free(AcirOptimizer *self) {
  if(self->instrCount > 0)
    AnchAllocator_Free(self->allocator, self->instrs);
  if(self->bindingCount > 0)
    AnchAllocator_Free(self->allocator, self->bindings);
  
  self->allocator = NULL;
  self->source = NULL;
  self->builder = NULL;
  self->didAnalyze = false;
  self->instrCount = 0;
  self->instrs = NULL;
  self->bindingCount = 0;
  self->bindings = NULL;
}

static AcirOptimizer_Binding *BindingGetOrCreate_(AcirOptimizer *self, size_t index) {
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

static void AllocAtLeast_Instrs_(AcirOptimizer *self, size_t count) {
  assert(self != NULL);
  if(count <= self->instrCount) return;

  size_t oldCount = self->instrCount;
  self->instrCount = count;

  if(oldCount == 0) {
    self->instrs = AnchAllocator_AllocZero(self->allocator,
      sizeof(AcirOptimizer_Instr) * self->instrCount);
  } else {
    self->instrs = AnchAllocator_Realloc(self->allocator,
      self->instrs, sizeof(AcirOptimizer_Instr) * self->instrCount);
    memset(self->instrs + oldCount, 0, sizeof(AcirOptimizer_Instr)
      * (self->instrCount - oldCount));
  }
}

int GetOperands_(const AcirInstr *instr, const AcirOperand **ops) {
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

const AcirImmediateValue *MaybeGetImmValue_(const AcirOptimizer *self, const AcirOperand *op) {
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

void ConstEvalInstr_(const AcirOptimizer *self, AcirImmediateValue *result, const AcirInstr *instr) {
  assert(self != NULL);
  assert(instr != NULL);
  assert(result != NULL);

  const AcirImmediateValue *val = NULL, *lhs = NULL, *rhs = NULL;
  
  int operandCount = AcirOpcode_OperandCount(instr->opcode);
  if(operandCount == 2) {
    val = MaybeGetImmValue_(self, &instr->val);
  } else if(operandCount == 3) {
    lhs = MaybeGetImmValue_(self, &instr->lhs);
    rhs = MaybeGetImmValue_(self, &instr->rhs);
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

void AcirOptimizer_Analyze(AcirOptimizer *self) {
  assert(self->allocator != NULL);
  assert(self->builder != NULL);
  assert(self->source != NULL);

  const AcirInstr *instr = &self->source->instrs[self->source->code];
  AllocAtLeast_Instrs_(self, instr->index + 1);
  self->instrs[instr->index].prev = ACIR_INSTR_NULL_INDEX;
  // AnchWriteString(wsStdout, ANSI_BLUE "\nInitial:\n" ANSI_RESET);
  // AnchWriteFormat(wsStdout,
  //   ANSI_MAGENTA "Self" ANSI_RESET " = {\n"
  //   "  " ANSI_BLUE "instrCount" ANSI_RESET " = %zu\n"
  //   "  " ANSI_BLUE "instrs" ANSI_RESET " = %p\n"
  //   "  " ANSI_BLUE "bindingCount" ANSI_RESET " = %zu\n"
  //   "  " ANSI_BLUE "bindings" ANSI_RESET " = %p\n"
  //   "}\n\n",
  //   self->instrCount, self->instrs,
  //   self->bindingCount, self->bindings
  // );
  
  while(instr != NULL) {
    if(instr->next != ACIR_INSTR_NULL_INDEX) {
      AllocAtLeast_Instrs_(self, instr->next + 1);
      self->instrs[instr->next].prev = instr->index;
    }

    // AnchWriteString(wsStdout, ANSI_BLUE "Reading instruction:\n" ANSI_RESET);
    // AcirInstr_Print(wsStdout, instr);
    // AnchWriteFormat(wsStdout,
    //   "\n"
    //   ANSI_MAGENTA "Self" ANSI_RESET " = {\n"
    //   "  " ANSI_BLUE "instrCount" ANSI_RESET " = %zu\n"
    //   "  " ANSI_BLUE "instrs" ANSI_RESET " = %p\n"
    //   "  " ANSI_BLUE "bindingCount" ANSI_RESET " = %zu\n"
    //   "  " ANSI_BLUE "bindings" ANSI_RESET " = %p\n"
    //   "}\n\n",
    //   self->instrCount, self->instrs,
    //   self->bindingCount, self->bindings
    // );

    AcirInstr *oinstr = AcirBuilder_Add(self->builder, instr->index);
    oinstr->opcode = instr->opcode;
    oinstr->index = instr->index;
    oinstr->type = instr->type;
    oinstr->next = instr->next;

    /************ Constant propagation ************/
    int operandCount = AcirOpcode_OperandCount(instr->opcode);
    assert(operandCount <= 3);
    assert(operandCount > 0);

    {
      const AcirOperand *iops[3] = {0};
      AcirOperand *oops[3] = {0};
      GetOperands_(instr, iops);
      GetOperands_(oinstr, (const AcirOperand**)oops); // weird cast
      
      for(int i = 0; i < operandCount; ++i) {
        oops[i]->type = iops[i]->type;
        if(oops[i]->type == ACIR_OPERAND_TYPE_BINDING) {
          oops[i]->idx = iops[i]->idx;
          if(i != operandCount - 1 || i == 0) { // output operand is ignored.
            assert(iops[i]->idx < self->bindingCount);
            AcirOptimizer_Binding *binding = &self->bindings[iops[i]->idx];
            assert(binding->exists);
            if(binding->flags & ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT) {
              oops[i]->type = ACIR_OPERAND_TYPE_IMMEDIATE;
              oops[i]->imm = binding->constant;
            }
          }
        } else oops[i]->imm = iops[i]->imm;
      }
    }

    if(oinstr->opcode == ACIR_OPCODE_SET && oinstr->val.type == ACIR_OPERAND_TYPE_IMMEDIATE) {
      // AnchWriteFormat(wsStdout, ANSI_BLUE "BindingGetOrCreate_" ANSI_RESET "(self, %zu)\n", oinstr->out.idx);
      AcirOptimizer_Binding *binding = BindingGetOrCreate_(self, oinstr->out.idx);
      // AnchWriteFormat(wsStdout, "  -> %p\n", binding);
      assert(!binding->exists);
      binding->flags |= ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT;
      binding->constant = instr->val.imm;
      binding->exists = true;
    } else if(AcirOpcode_ConstEval(oinstr->opcode)) {
      AcirOptimizer_Binding *binding = BindingGetOrCreate_(self, oinstr->out.idx);
      assert(!binding->exists);
      binding->exists = true;
      if(operandCount == 2 && MaybeGetImmValue_(self, &oinstr->val)) {
        binding->flags |= ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT;
        ConstEvalInstr_(self, &binding->constant, oinstr);
      } else if(operandCount == 3 && MaybeGetImmValue_(self, &oinstr->lhs) && MaybeGetImmValue_(self, &oinstr->rhs)) {
        binding->flags |= ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT;
        ConstEvalInstr_(self, &binding->constant, oinstr);
      }

      oinstr->opcode = ACIR_OPCODE_SET;
      oinstr->val = (AcirOperand){ .type = ACIR_OPERAND_TYPE_IMMEDIATE, .imm = binding->constant };
    } else if(oinstr->opcode == ACIR_OPCODE_SET && oinstr->val.type == ACIR_OPERAND_TYPE_IMMEDIATE) {
      // AnchWriteFormat(wsStdout, ANSI_BLUE "BindingGetOrCreate_" ANSI_RESET "(self, %zu)\n", oinstr->out.idx);
      AcirOptimizer_Binding *binding = BindingGetOrCreate_(self, oinstr->out.idx);
      // AnchWriteFormat(wsStdout, "  -> %p\n", binding);
      assert(!binding->exists);
      binding->flags |= ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT;
      binding->constant = instr->val.imm;
      binding->exists = true;
    } else if(operandCount > 1) { // for instructions with output
      assert(oinstr->out.type == ACIR_OPERAND_TYPE_BINDING);
      // AnchWriteFormat(wsStdout, ANSI_BLUE "BindingGetOrCreate_" ANSI_RESET "(self, %zu)\n", oinstr->out.idx);
      AcirOptimizer_Binding *binding = BindingGetOrCreate_(self, oinstr->out.idx);
      // AnchWriteFormat(wsStdout, "  -> %p\n", binding);
      assert(!binding->exists);
      binding->exists = true;
    }

    self->lastInstr = instr->index;
    if(instr->next == ACIR_INSTR_NULL_INDEX) break;
    instr = &self->source->instrs[instr->next];
  }

  self->didAnalyze = true;
}

void AcirOptimizer_ConstantFold(AcirOptimizer *self) {
  assert(self != NULL);
}

void AcirOptimizer_DeadCode(AcirOptimizer *self) {
  assert(self != NULL);
  AcirInstr *instr = &self->builder->instrs[self->lastInstr];
  assert(instr->opcode == ACIR_OPCODE_RET);
  assert(instr->next == ACIR_INSTR_NULL_INDEX);

  while(true) {
    bool shouldRemove = false;

    int operandCount = AcirOpcode_OperandCount(instr->opcode);
    if(!AcirOpcode_ConstEval(instr->opcode)) {
      const AcirOperand *ops[3];
      GetOperands_(instr, ops);
      int maxOperandIndex = operandCount == 1 ? 1 : operandCount - 1;
      for(int i = 0; i < maxOperandIndex; ++i) {
        if(ops[i]->type == ACIR_OPERAND_TYPE_BINDING) {
          assert(ops[i]->idx < self->bindingCount);
          self->bindings[ops[i]->idx].used = true;
        }
      }
    } else {
      if(operandCount > 1 && instr->out.type == ACIR_OPERAND_TYPE_BINDING) {
        assert(instr->out.idx < self->bindingCount);
        bool used = self->bindings[instr->out.idx].used;
        if(!used) shouldRemove = true;
        else {
          const AcirOperand *ops[3];
          GetOperands_(instr, ops);
          for(int i = 0; i < operandCount - 1; ++i) {
            if(ops[i]->type == ACIR_OPERAND_TYPE_BINDING) {
              assert(ops[i]->idx < self->bindingCount);
              self->bindings[ops[i]->idx].used = true;
            }
          }
        }
      }
    }

    if(shouldRemove) {
      if(self->instrs[instr->index].prev == ACIR_INSTR_NULL_INDEX) {
        self->builder->target->code = instr->next;
      } else {
        self->builder->instrs[self->instrs[instr->index].prev].next = instr->next;
      }
    }
    
    if(self->instrs[instr->index].prev == ACIR_INSTR_NULL_INDEX) break;
    instr = &self->builder->instrs[self->instrs[instr->index].prev];
  }
}
