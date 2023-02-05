#ifndef ANNEC_IR_OPCODE_HEADER_
#define ANNEC_IR_OPCODE_HEADER_

#include <annec/common.h>

// O(NAME, MNEMONIC, OPERAND_COUNT, SIGNATURE, DESCRIPTION, CONSTEVAL)
#define ACIR_OPCODES_ENUM(O, S) \
  O(SET, <b>, 2, ".T!void T, wT", "init binding", true) S \
  O(RET, ret, 1, ".T T", "return, must be last in block", false) S \
  O(REF, ref, 2, ".T lT, w*T", "address of, reference", false) S \
  O(DER, der, 2, ".T!void *T, wT", "dereference", false) S \
  O(ADD, add, 3, ".T!void T, T, wT", "add two numbers", true) S \
  O(SUB, sub, 3, ".T!void T, T, wT", "substract two numbers", true) S \
  O(MUL, mul, 3, ".T!void T, T, wT", "multiply two numbers", true) S \
  O(DIV, div, 3, ".T!void T, T, wT", "divide two numbers", true) S \
  O(MOD, mod, 3, ".T!void!float32!float64 T, T, wT", "\"modulo\" two numbers", true) S \
  O(NEG, neg, 2, ".T!void T, wT", "negate number", true) S \
  O(EQL, eql, 3, ".T!void T, T, wbool", "check if two numbers are equal", true) S \
  O(NEQ, neq, 3, ".T!void T, T, wbool", "inverse of EQL", true) S \
  O(LTH, lth, 3, ".T!void T, T, wbool", "check if a number is less", true) S \
  O(GTH, gth, 3, ".T!void T, T, wbool", "check if a number is greater", true) S \
  O(LEQ, leq, 3, ".T!void T, T, wbool", "check if a number is less or equal", true) S \
  O(GEQ, geq, 3, ".T!void T, T, wbool", "check if a number is greater or equal", true) S \
  O(AND, and, 3, " bool, bool, wbool", "\"and\" booleans", true) S \
  O(COR, cor, 3, " bool, bool, wbool", "\"or\" booleans", true) S \
  O(XOR, xor, 3, " bool, bool, wbool", "\"xor\" booleans", true) S \
  O(NOT, not, 2, " bool, wbool", "negate boolean", true) S \
  O(BIT_AND, bitand, 3, ".T!float32!float64!void T, T, wT", "bitwise \"and\" booleans", true) S \
  O(BIT_COR, bitcor, 3, ".T!float32!float64!void T, T, wT", "bitwise \"or\" booleans", true) S \
  O(BIT_XOR, bitxor, 3, ".T!float32!float64!void T, T, wT", "bitwise \"xor\" booleans", true) S \
  O(BIT_NOT, bitnot, 2, ".T!float32!float64!void T, wT", "bitwise negate boolean", true) S \
  O(EFF, eff, 2, ".T any, wT", "perform side effect", false) S \
  O(PHI, phi, 0, NULL, "phi", false) S \
  O(BR, br, 1, " bool", "branch", false)

typedef uint8_t AcirOpcode;

#define COMMA ,
#define TMP(NAME, ...) ACIR_OPCODE_##NAME
enum AcirOpcodes { ACIR_OPCODES_ENUM(TMP, COMMA), ACIR_OPCODE_MAX_ };
#undef TMP
#undef COMMA

/** Retrieve opcode name by the opcode value. */
const char *AcirOpcode_Name(AcirOpcode opcode);

/** Retrieve opcode mnemonic by the opcode value. */
const char *AcirOpcode_Mnemonic(AcirOpcode opcode);

/** Retrieve opcode mnemonic by the opcode value. */
int AcirOpcode_OperandCount(AcirOpcode opcode);

/** Retrieve opcode signature by the opcode value. May be NULL. */
ANCH_NULLABLE(const char *) AcirOpcode_Signature(AcirOpcode opcode);

/** Retrieve opcode description by the opcode value. */
const char *AcirOpcode_Description(AcirOpcode opcode);

/** Retrieve whether the opcode if constevaluatable. */
bool AcirOpcode_ConstEval(AcirOpcode opcode);

#endif
