#ifndef ANNEC_IR_OPERAND_HEADER_
#define ANNEC_IR_OPERAND_HEADER_

#include <annec/common.h>
#include <annec/ir/value_type.h>

/** Operand type (immediate or binding) */
typedef AnnecUInt8 AcirOperandType;

/** @ref AcirOperandType enum */
enum AcirOperandTypes {
  ACIR_OPERAND_TYPE_IMMEDIATE,
  ACIR_OPERAND_TYPE_BINDING,
};

/** Operand type (immediate or binding) */
typedef AnnecUInt8 AcirOperandKind;

/** Retrieve name of an operand type. */
const char *AcirOperandType_Name(AcirOperandType type);

/** @ref AcirOperandKind enum */
enum AcirOperandKinds {
  ACIR_OPERAND_KIND_READABLE,
  ACIR_OPERAND_KIND_WRITABLE,
};

/** Immediate value of an operand. */
typedef struct {
  /** Type of the immediate value represented. */
  const AcirValueType *type;
  union {
    /** Unsigned 64-bit integer, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_UINT64 */
    AnnecUInt64 uint64;
    /** Signed 64-bit integer, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_SINT64 */
    AnnecInt64 sint64;
    /** Unsigned 32-bit integer, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_UINT64 */
    AnnecUInt32 uint32;
    /** Signed 32-bit integer, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_SINT64 */
    AnnecInt32 sint32;
    /** Unsigned 16-bit integer, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_UINT64 */
    AnnecUInt16 uint16;
    /** Signed 16-bit integer, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_SINT64 */
    AnnecInt16 sint16;
    /** Unsigned 8-bit integer, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_UINT64 */
    AnnecUInt8 uint8;
    /** Signed 8-bit integer, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_SINT64 */
    AnnecInt8 sint8;
    /** Single precision float, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_FLOAT32 */
    AnnecFloat32 float32;
    /** Double precision float, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_FLOAT64 */
    AnnecFloat64 float64;
    /** Boolean, active when @ref type == @ref ACIR_BASIC_VALUE_TYPE_BOOL */
    bool boolean;
  };
} AcirImmediateValue;

/** Binding value of an operand (index). */
typedef AnnecUInt64 AcirBindingValue;

/** Operand of an instruction. @see AcirInstr */
typedef struct {
  /** Type of the operand (immediate or binding) */
  AcirOperandType type; 

  union {
    /** Immediate value, active when @ref type is @ref ACIR_OPERAND_TYPE_IMMEDIATE */
    AcirImmediateValue imm;
    /** Binding value, active when @ref type is @ref ACIR_OPERAND_TYPE_BINDING */
    AcirBindingValue idx;
  };
} AcirOperand;

/** Print operand to stream. */
void AcirOperand_Print(const AcirOperand *self, AnchCharWriteStream *out);

#endif // ANNEC_IR_OPERAND_HEADER_
