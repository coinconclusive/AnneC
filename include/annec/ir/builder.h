#ifndef ANNEC_IR_BUILDER_HEADER_
#define ANNEC_IR_BUILDER_HEADER_

#include <annec/common.h>
#include <annec/ir/function.h>

/** Function builder. */
typedef struct {
  /** Target function. */
  AcirFunction *target;
  /** Allocator. */
  AnchAllocator *allocator;
  /** Local pointer to instructions (same as @ref AcirFunction::instrs). */
  AcirInstr *instrs;
} AcirBuilder;

/** Initialize instruction builder. */
void AcirBuilder_Init(AcirBuilder *self, AcirFunction *target, AnchAllocator *allocator);

/** Free instruction builder. */
void AcirBuilder_Free(AcirBuilder *self);

/** Add instruction to a builders target function at specified index. Return pointer to it. */
AcirInstr *AcirBuilder_Add(AcirBuilder *self, size_t index);

/** Initialize and read instructions from a text file (in a human-readable form). */
void AcirBuilder_BuildFromText(AcirBuilder *self, AnchCharReadStream *inp, AnchAllocator *allocator);
/** Initiailize and read instructions from a binary file. */
void AcirBuilder_BuildFromBinary(AcirBuilder *self, AnchByteReadStream *inp, AnchAllocator *allocator);
/** Copy instructions to a different builder with normalized indices. */
void AcirBuilder_BuildToNormalized(const AcirBuilder *self, AcirBuilder *target);

#endif // ANNEC_IR_BUILDER_HEADER_
