#include <annec/ir.h>
#include <annec/anchor.h>
#include "../cli.h"
#include "private.h"
#include <assert.h>

void AcirOptimizer_DeadCode(AcirOptimizer *self) {
  assert(self != NULL);
  assert(self->private != NULL);
  assert(self->didAnalyze);

  AcirInstr *instr = &self->builder->instrs[self->private->lastInstr];
  assert(instr->opcode == ACIR_OPCODE_RET);
  assert(instr->next.idx == ACIR_INSTR_NULL_INDEX);

  while(true) {
    bool shouldRemove = false;

    int operandCount = AcirOpcode_OperandCount(instr->opcode);
    if(!AcirOpcode_ConstEval(instr->opcode)) {
      const AcirOperand *ops[3];
      AcirOptPriv_GetOperands_(instr, ops);
      int maxOperandIndex = operandCount == 1 ? 1 : operandCount - 1;
      for(int i = 0; i < maxOperandIndex; ++i) {
        if(ops[i]->type == ACIR_OPERAND_TYPE_BINDING) {
          assert(ops[i]->idx < self->private->bindingCount);
          self->private->bindings[ops[i]->idx].used = true;
        }
      }
    } else {
      if(operandCount > 1 && instr->out.type == ACIR_OPERAND_TYPE_BINDING) {
        assert(instr->out.idx < self->private->bindingCount);
        bool used = self->private->bindings[instr->out.idx].used;
        if(!used) shouldRemove = true;
        else {
          const AcirOperand *ops[3];
          AcirOptPriv_GetOperands_(instr, ops);
          for(int i = 0; i < operandCount - 1; ++i) {
            if(ops[i]->type == ACIR_OPERAND_TYPE_BINDING) {
              assert(ops[i]->idx < self->private->bindingCount);
              self->private->bindings[ops[i]->idx].used = true;
            }
          }
        }
      }
    }

    if(shouldRemove) {
      if(self->private->instrs[instr->index].prev == ACIR_INSTR_NULL_INDEX) {
        self->builder->target->code = instr->next.idx;
      } else {
        self->builder->instrs[self->private->instrs[instr->index].prev].next = instr->next;
      }
    }
    
    if(self->private->instrs[instr->index].prev == ACIR_INSTR_NULL_INDEX) break;
    instr = &self->builder->instrs[self->private->instrs[instr->index].prev];
  }
}
