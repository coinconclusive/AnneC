#ifndef ANNEC_IR_OPTIMIZER_HEADER_
#define ANNEC_IR_OPTIMIZER_HEADER_

#include <annec/common.h>
#include <annec/ir/instr.h>
#include <annec/ir/function.h>
#include <annec/ir/builder.h>

/** Holds information required for IR optimization. */
typedef struct AcirOptimizer {
  /** Source function to optimize. */
  const AcirFunction *source;
  /** New instruction builder. */
  AcirBuilder *builder;
  /** Allocator. */
  AnchAllocator *allocator;
  /** Set to true after @ref AcirOptimizer_Analyze is called. */
  bool didAnalyze;
  /** Implementation defined fields. */
  struct AcirOptimizer_Private_ *private;
} AcirOptimizer;

/** Initialize all the private fields. self->source, self->builder, self->allocator must be valid. */
void AcirOptimizer_Init(AcirOptimizer *self);

/** Free all the private fields. */
void AcirOptimizer_Free(AcirOptimizer *self);

/** Analyze the source instructions. @ref AcirOptimizer::didAnalyze is set to true. */
void AcirOptimizer_Analyze(AcirOptimizer *self);

#endif // ANNEC_IR_OPTIMIZER_HEADER_
