#include "annec_anchor.h"
#include <acir/acir.h>
#include <assert.h>

static void AllocAtLeast_Instrs_(AcirOptimizer *self, size_t count) {
  assert(self);
  if(self->instrCount >= count) return;
  self->instrCount = count;
  if(self->instrs == NULL) {
    self->instrs = AnchAllocator_Alloc(self->allocator, sizeof(AcirOptimizerInstr) * count);
  } else {
    self->instrs = AnchAllocator_Realloc(self->allocator, self->instrs, sizeof(AcirOptimizerInstr) * count);
  }
}

void AcirOptimizer_Analyze(AcirOptimizer *self) {
  assert(self->allocator != NULL);
  assert(self->builder != NULL);
  assert(self->source != NULL);

  const AcirInstr *instr = self->source->code;
  AllocAtLeast_Instrs_(self, instr->index + 1);
  self->instrs[instr->index].prev = NULL;

  while(instr != NULL) {
    if(instr->next == NULL) break;

    AllocAtLeast_Instrs_(self, instr->next->index + 1);
    self->instrs[instr->next->index].prev = instr;

    if(instr->opcode == ACIR_OPCODE_SET) {
      // instr->out
    }
  }

  self->didAnalyze = true;
}

void AcirOptimizer_ConstantFold(AcirOptimizer *self) {

}

void AcirOptimizer_DeadCode(AcirOptimizer *self) {

}


