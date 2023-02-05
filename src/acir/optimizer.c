#include <annec/ir.h>
#include <annec/anchor.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "cli.h"
#include <annec/ir/optimizer/private.h>
#include "optimizations/private.h"

void AcirOptimizer_Init(AcirOptimizer *self) {
  assert(self != NULL);
  self->didAnalyze = false;
  self->private = AnchAllocator_Alloc(self->allocator, sizeof(AcirOptimizer_Private_));
  self->private->bindingCount = 0;
  self->private->bindings = NULL;
  self->private->instrCount = 0;
  self->private->instrs = NULL;
}

void AcirOptimizer_Free(AcirOptimizer *self) {
  assert(self != NULL);
  assert(self->allocator != NULL);
  assert(self->private != NULL);

  if(self->private->instrCount > 0)
    AnchAllocator_Free(self->allocator, self->private->instrs);
  if(self->private->bindingCount > 0)
    AnchAllocator_Free(self->allocator, self->private->bindings);
  
  self->private->instrCount = 0;
  self->private->instrs = NULL;
  self->private->bindingCount = 0;
  self->private->bindings = NULL;
  AnchAllocator_Free(self->allocator, self->private);

  self->allocator = NULL;
  self->source = NULL;
  self->builder = NULL;
  self->didAnalyze = false;
}

void AcirOptimizer_Analyze(AcirOptimizer *self) {
  assert(self->allocator != NULL);
  assert(self->builder != NULL);
  assert(self->source != NULL);
  assert(self->private != NULL);

  const AcirInstr *instr = &self->source->instrs[self->source->code];
  AcirOptPriv_AllocAtLeast_Instrs_(self->private, instr->index + 1, self->allocator);
  self->private->instrs[instr->index].prev = ACIR_INSTR_NULL_INDEX;
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
    if(instr->next.idx != ACIR_INSTR_NULL_INDEX) {
      AcirOptPriv_AllocAtLeast_Instrs_(self->private, instr->next.idx + 1, self->allocator);
      self->private->instrs[instr->next.idx].prev = instr->index;
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

    int operandCount = AcirOpcode_OperandCount(instr->opcode);
    assert(operandCount <= 3);
    assert(operandCount > 0);

    {
      const AcirOperand *iops[3] = {0};
      AcirOperand *oops[3] = {0};
      AcirOptPriv_GetOperands_(instr, iops);
      AcirOptPriv_GetOperands_(oinstr, (const AcirOperand**)oops); // weird cast
      
      for(int i = 0; i < operandCount; ++i) {
        oops[i]->type = iops[i]->type;
        if(oops[i]->type == ACIR_OPERAND_TYPE_BINDING) {
          oops[i]->idx = iops[i]->idx;
          if(i != operandCount - 1 || i == 0) { // output operand is ignored.
            assert(iops[i]->idx < self->private->bindingCount);
            AcirOptimizer_Binding *binding = &self->private->bindings[iops[i]->idx];
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
      AcirOptimizer_Binding *binding = AcirOptPriv_BindingGetOrCreate_(self->private, oinstr->out.idx);
      // AnchWriteFormat(wsStdout, "  -> %p\n", binding);
      assert(!binding->exists);
      binding->flags |= ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT;
      binding->constant = instr->val.imm;
      binding->exists = true;
    } else if(AcirOpcode_ConstEval(oinstr->opcode)) {
      AcirOptimizer_Binding *binding = AcirOptPriv_BindingGetOrCreate_(self->private, oinstr->out.idx);
      assert(!binding->exists);
      binding->exists = true;
      if(operandCount == 2 && AirOptPriv_MaybeGetImmValue_(self->private, &oinstr->val)) {
        binding->flags |= ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT;
        AcirOptPriv_ConstEvalInstr_(self, &binding->constant, oinstr);
        oinstr->opcode = ACIR_OPCODE_SET;
        oinstr->val = (AcirOperand){ .type = ACIR_OPERAND_TYPE_IMMEDIATE, .imm = binding->constant };
      } else if(operandCount == 3 && AirOptPriv_MaybeGetImmValue_(self->private, &oinstr->lhs)
                && AirOptPriv_MaybeGetImmValue_(self->private, &oinstr->rhs)) {
        binding->flags |= ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT;
        AcirOptPriv_ConstEvalInstr_(self, &binding->constant, oinstr);
        oinstr->opcode = ACIR_OPCODE_SET;
        oinstr->val = (AcirOperand){ .type = ACIR_OPERAND_TYPE_IMMEDIATE, .imm = binding->constant };
      }
    } else if(oinstr->opcode == ACIR_OPCODE_SET && oinstr->val.type == ACIR_OPERAND_TYPE_IMMEDIATE) {
      // AnchWriteFormat(wsStdout, ANSI_BLUE "BindingGetOrCreate_" ANSI_RESET "(self, %zu)\n", oinstr->out.idx);
      AcirOptimizer_Binding *binding = AcirOptPriv_BindingGetOrCreate_(self->private, oinstr->out.idx);
      // AnchWriteFormat(wsStdout, "  -> %p\n", binding);
      assert(!binding->exists);
      binding->flags |= ACIR_OPTIMIZER_BINDING_FLAG_CONSTANT;
      binding->constant = instr->val.imm;
      binding->exists = true;
    } else if(operandCount > 1) { // for instructions with output
      assert(oinstr->out.type == ACIR_OPERAND_TYPE_BINDING);
      // AnchWriteFormat(wsStdout, ANSI_BLUE "BindingGetOrCreate_" ANSI_RESET "(self, %zu)\n", oinstr->out.idx);
      AcirOptimizer_Binding *binding = AcirOptPriv_BindingGetOrCreate_(self->private, oinstr->out.idx);
      // AnchWriteFormat(wsStdout, "  -> %p\n", binding);
      assert(!binding->exists);
      binding->exists = true;
    }

    self->private->lastInstr = instr->index;
    if(instr->next.idx == ACIR_INSTR_NULL_INDEX) break;
    instr = &self->source->instrs[instr->next.idx];
  }

  self->didAnalyze = true;
}

void AcirOptimizer_InstrMerge(AcirOptimizer *self) {
  assert(self != NULL);
}
