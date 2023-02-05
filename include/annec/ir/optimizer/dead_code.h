#ifndef ANNEC_IR_OPTIMIZER_DEAD_CODE_HEADER_
#define ANNEC_IR_OPTIMIZER_DEAD_CODE_HEADER_

#include <annec/ir/optimizer.h>

/** "Dead code elimination" optimization pass. @ref AcirOptimizer::didAnalyze must be true.
  * Instructions that are deemed un-effectful are removed. */
void AcirOptimizer_DeadCode(AcirOptimizer *self);

#endif // ANNEC_IR_OPTIMIZER_DEAD_CODE_HEADER_
