#include <annec_anchor.h>
#include <acir/acir.h>
#include <assert.h>
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

void AcirOptimizer_Analyze(AcirOptimizer *self) {
  assert(self->allocator != NULL);
  assert(self->builder != NULL);
  assert(self->source != NULL);

  const AcirInstr *instr = self->source->code;
  AllocAtLeast_Instrs_(self, instr->index + 1);
  self->instrs[instr->index].prev = NULL;
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
    if(instr->next != ACIR_INSTR_INDEX_LAST) {
      AllocAtLeast_Instrs_(self, instr->next + 1);
      self->instrs[instr->next].prev = instr;
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
    } else if(operandCount > 1) { // for instructions with output
      assert(oinstr->out.type == ACIR_OPERAND_TYPE_BINDING);
      // AnchWriteFormat(wsStdout, ANSI_BLUE "BindingGetOrCreate_" ANSI_RESET "(self, %zu)\n", oinstr->out.idx);
      AcirOptimizer_Binding *binding = BindingGetOrCreate_(self, oinstr->out.idx);
      // AnchWriteFormat(wsStdout, "  -> %p\n", binding);
      assert(!binding->exists);
      binding->exists = true;
    }

    if(instr->next == ACIR_INSTR_INDEX_LAST) break;
    instr = &self->source->instrs[instr->next];
  }

  self->didAnalyze = true;
}

void AcirOptimizer_ConstantFold(AcirOptimizer *self) {
  assert(self != NULL);

  {
    const AcirInstr *instr = self->source->code;
    while(instr) {
    }
  }
}

void AcirOptimizer_DeadCode(AcirOptimizer *self) {

}
