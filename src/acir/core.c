#include <annec/ir.h>
#include <annec/anchor.h>
#include "cli.h"

#include <assert.h>

#define COMMA ,

#define TMP(A, B, C, D, E, F) { #A, #B, C, D, E, F }
static const struct {
  const char *name;
  const char *mnemonic;
  int operandCount;
  const char *signature;
  const char *description;
  bool consteval;
} AcirOpcode_Info[] = { ACIR_OPCODES_ENUM(TMP, COMMA) };
#undef TMP

#define TMP(A, B, C) { #A, #B, C }
static const struct {
  const char *name;
  const char *mnemonic;
  const char *description;
} AcirBasicValueType_Info[] = { ACIR_BASIC_VALUE_TYPES_ENUM(TMP, COMMA) };
#undef TMP

#undef COMMA

const char *AcirOperandType_Name(AcirOperandType type) {
  switch(type) {
  case ACIR_OPERAND_TYPE_BINDING: return "binding";
  case ACIR_OPERAND_TYPE_IMMEDIATE: return "immediate";
  default: return NULL;
  }
}

const char *AcirOpcode_Name(AcirOpcode opcode) {
  assert(opcode < ACIR_OPCODE_MAX_);
  return AcirOpcode_Info[opcode].name;
}

const char *AcirOpcode_Mnemonic(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return NULL; // ANSI_RED "<bad opcode>" ANSI_RESET;
  return AcirOpcode_Info[opcode].mnemonic;
}

int AcirOpcode_OperandCount(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return -1;
  return AcirOpcode_Info[opcode].operandCount;
}

const char *AcirOpcode_Signature(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return NULL; // ANSI_RED "<bad opcode>" ANSI_RESET;
  return AcirOpcode_Info[opcode].signature;
}

const char *AcirOpcode_Description(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return NULL; // ANSI_RED "<bad opcode>" ANSI_RESET;
  return AcirOpcode_Info[opcode].description;
}

bool AcirOpcode_ConstEval(AcirOpcode opcode) {
  if(opcode >= ACIR_OPCODE_MAX_) return NULL; // false;
  return AcirOpcode_Info[opcode].consteval;
}


const char *AcirBasicValueType_Name(AcirBasicValueType type) {
  if(type >= ACIR_BASIC_VALUE_TYPE_MAX_) NULL; // return ANSI_RED "<bad value type>" ANSI_RESET;
  return AcirBasicValueType_Info[type].name;
}

const char *AcirBasicValueType_Mnemonic(AcirBasicValueType type) {
  if(type >= ACIR_BASIC_VALUE_TYPE_MAX_) return NULL; // ANSI_RED "<bad value type>" ANSI_RESET;
  return AcirBasicValueType_Info[type].mnemonic;
}

const char *AcirBasicValueType_Description(AcirBasicValueType type) {
  if(type >= ACIR_BASIC_VALUE_TYPE_MAX_) return NULL; // ANSI_RED "<bad value type>" ANSI_RESET;
  return AcirBasicValueType_Info[type].description;
}
