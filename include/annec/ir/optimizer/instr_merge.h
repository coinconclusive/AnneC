#ifndef ANNEC_IR_OPTIMIZER_INSTR_MERGE_HEADER_
#define ANNEC_IR_OPTIMIZER_INSTR_MERGE_HEADER_

#include <annec/ir/optimizer.h>

/** "Instruction merge" optimization pass. @ref AcirOptimizer::didAnalyze must be true.
  * Multiple instructions that can be merged into one are combined.
  * Prefer to run this after dead code elimination. */
void AcirOptimizer_InstrMerge(AcirOptimizer *self);

#endif // ANNEC_IR_OPTIMIZER_INSTR_MERGE_HEADER_
