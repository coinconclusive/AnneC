#ifndef ANNEC_IR_INSTR_HEADER_
#define ANNEC_IR_INSTR_HEADER_

#include <annec/common.h>
#include <annec/ir/operand.h>
#include <annec/ir/opcode.h>

/** Phi node. */
typedef struct {
  AnnecSize count;
  AcirBindingValue *idxs;
} AcirPhi;

/** Represents a non-accessible index. */
#define ACIR_INSTR_NULL_INDEX ((AnnecSize)-1)

/** Represets a single instruction. */
typedef struct AcirInstr AcirInstr;

/** Index of an instruciton. */
typedef AnnecSize AcirInstrIndex;

struct AcirInstr {
  /** Index. */
  AcirInstrIndex index;

  /** Opcode. */
  AcirOpcode opcode;

  /** Bound type. */
  const AcirValueType *type;

  /** Reference to next instruction node. */
  union {
    /** For branch instruction. */
    struct { AcirInstrIndex bt, bf; };
    /** For simple instructions. */
    AcirInstrIndex idx;
  } next;

  /** Output operand. */
  AcirOperand out;

  union {
    /** Left and right input opreands. */
    struct { AcirOperand lhs, rhs; };
    /** Single input opreand. */
    AcirOperand val;
    /** Phi. */
    AcirPhi phi;
  };
};

/** Print instruction to stream with specified indent. */
void AcirInstr_PrintIndent(const AcirInstr *self, AnchCharWriteStream *out, int indent);

/** Print instruction to stream. */
static inline void AcirInstr_Print(const AcirInstr *self, AnchCharWriteStream *out) {
  AcirInstr_PrintIndent(self, out, 0);
}

#endif // ANNEC_IR_INSTR_HEADER_
