#include <annec/ir.h>
#include <annec/anchor.h>
#include "cli.h"

#include <assert.h>

void AcirBuilder_Init(AcirBuilder *self, AcirFunction *target, AnchAllocator *allocator) {
  assert(self != NULL);
  assert(target != NULL);
  self->target = target;
  self->allocator = allocator;
  self->target->instrCount = 0;
  self->target->instrs = NULL;
  self->instrs = NULL;
}

void AcirBuilder_Free(AcirBuilder *self) {
  assert(self != NULL);
  if(self->target->instrCount > 0)
    AnchAllocator_Free(self->allocator, self->instrs);
  self->target->instrCount = 0;
  self->target->instrs = NULL;
  self->target->code = ACIR_INSTR_NULL_INDEX;
  self->allocator = NULL;
  self->target = NULL;
  self->instrs = NULL;
}

AcirInstr *AcirBuilder_Add(AcirBuilder *self, size_t index) {
  assert(self != NULL);

  if(index < self->target->instrCount) return &self->instrs[index];

  size_t oldInstrCount = self->target->instrCount;
  self->target->instrCount = index + 1;

  if(oldInstrCount == 0) {
    self->target->instrs = self->instrs =
      AnchAllocator_AllocZero(self->allocator, sizeof(AcirInstr));
    
    self->target->code = index;
  } else {
    self->target->instrs = self->instrs =
      AnchAllocator_Realloc(self->allocator, self->instrs,
        sizeof(AcirInstr) * self->target->instrCount);
    
    memset(self->instrs + oldInstrCount, 0,
      sizeof(AcirInstr) * (self->target->instrCount - oldInstrCount));
  }
  
  return &self->instrs[index];
}

void AcirBuilder_BuildToNormalized(const AcirBuilder *self, AcirBuilder *target) {
  size_t index = 0;
  for(AcirInstr *instr = &self->instrs[self->target->code]; ; instr = &self->instrs[instr->next.idx]) {
    AcirInstr *tinstr = AcirBuilder_Add(target, index);
    *tinstr = *instr;
    tinstr->index = index;
    if(instr->next.idx == ACIR_INSTR_NULL_INDEX) break;
    tinstr->next.idx = ++index;
  }
}

void AcirBuilder_BuildFromText(AcirBuilder *self, AnchCharReadStream *inp, AnchAllocator *allocator);
void AcirBuilder_BuildFromBinary(AcirBuilder *self, AnchByteReadStream *inp, AnchAllocator *allocator);
