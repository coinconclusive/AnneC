#include <annec/ir.h>
#include <annec/anchor.h>
#include "cli.h"

#include <assert.h>

void AcirInstr_PrintIndent(const AcirInstr *self, AnchCharWriteStream *out, int indent) {
  if(!self) { AnchWriteFormat(out, ANSI_RED "<null instruction>" ANSI_RESET); return; }
  
  if(self->opcode == ACIR_OPCODE_SET) {
    AnchWriteFormat(out, ANSI_GRAY "%zu" ANSI_RESET " | ", self->index);
    for(int i = 0; i < indent; ++i) AnchWriteString(wsStdout, "  ");
    AcirOperand_Print(&self->out, out);
    AnchWriteString(out, " =.");
    AcirValueType_Print(self->type, out);
    AnchWriteString(out, " ");
    AcirOperand_Print(&self->val, out);
  } else if(self->opcode == ACIR_OPCODE_PHI) {
    AnchWriteFormat(out, ANSI_GRAY "%zu" ANSI_RESET " | ", self->index);
    for(int i = 0; i < indent; ++i) AnchWriteString(wsStdout, "  ");
    AcirOperand_Print(&self->out, out);
    AnchWriteString(out, " = " ANSI_BLUE "phi." ANSI_RESET);
    AcirValueType_Print(self->type, out);
    for(size_t i = 0; i < self->phi.count; ++i) {
      AnchWriteFormat(out, " $" ANSI_YELLOW "%zu" ANSI_RESET, self->phi.idxs[i]);
    }
  } else {
    AnchWriteFormat(out, ANSI_GRAY "%zu" ANSI_RESET " | ", self->index);
    for(int i = 0; i < indent; ++i) AnchWriteString(wsStdout, "  ");

    int operandCount = AcirOpcode_OperandCount(self->opcode);
    if(operandCount == 1) {
      AnchWriteFormat(out, ANSI_BLUE "%s" ANSI_RESET, AcirOpcode_Mnemonic(self->opcode));
      assert(AcirOpcode_Signature(self->opcode) != NULL);
      if(AcirOpcode_Signature(self->opcode)[0] == '.') {
        // only print type if function is generic.
        AnchWriteString(out, ".");
        AcirValueType_Print(self->type, out);
      }
      AnchWriteString(out, " ");
      AcirOperand_Print(&self->val, out);
    } else if(operandCount == 2) {
      AcirOperand_Print(&self->out, out);
      AnchWriteString(out, " = ");
      AnchWriteFormat(out, ANSI_BLUE "%s" ANSI_RESET ".", AcirOpcode_Mnemonic(self->opcode));
      AcirValueType_Print(self->type, out);
      AnchWriteString(out, " ");
      AcirOperand_Print(&self->val, out);
    } else if(operandCount == 3) {
      AcirOperand_Print(&self->out, out);
      AnchWriteString(out, " = ");
      AnchWriteFormat(out, ANSI_BLUE "%s" ANSI_RESET ".", AcirOpcode_Mnemonic(self->opcode));
      AcirValueType_Print(self->type, out);
      AnchWriteString(out, " ");
      AcirOperand_Print(&self->lhs, out);
      AnchWriteString(out, " ");
      AcirOperand_Print(&self->rhs, out);
    } else {
      AnchWriteFormat(out, ANSI_RED "<bad operand count (%d)>" ANSI_RESET, operandCount);
    }
  }

  if(self->opcode == ACIR_OPCODE_BR) {
    AnchWriteFormat(out, ANSI_GRAY " -> %zu | %zu" ANSI_RESET, self->next.bt, self->next.bf);
  } else {
    if(self->next.idx == ACIR_INSTR_NULL_INDEX)
      AnchWriteString(out, ANSI_GRAY  " -> " ANSI_CYAN "end" ANSI_RESET);
    else if(self->next.idx != self->index + 1)
      AnchWriteFormat(out, ANSI_GRAY " -> %zu" ANSI_RESET, self->next.idx);
  }
}

void AcirOperand_Print(const AcirOperand *self, AnchCharWriteStream *out) {
  if(!self) { AnchWriteFormat(out, ANSI_RED "<null operand>" ANSI_RESET); return; } 
  if(self->type == ACIR_OPERAND_TYPE_IMMEDIATE) {
    assert(self->imm.type != NULL);
    assert(self->imm.type->type == ACIR_VALUE_TYPE_BASIC);

    if(self->imm.type->basic != ACIR_BASIC_VALUE_TYPE_BOOL
    && self->imm.type->basic != ACIR_BASIC_VALUE_TYPE_VOID) {
      AcirValueType_Print(self->imm.type, out);
      AnchWriteString(out, "#" ANSI_GREEN);
    }
    if(!self->imm.type) {
      AnchWriteFormat(out, ANSI_RED "0x%016x", self->imm);
    } else if(self->imm.type->type != ACIR_VALUE_TYPE_BASIC) {
      AnchWriteFormat(out, ANSI_RED "<not basic type (%d)>", self->imm.type->type);
      AnchWriteFormat(out, ANSI_RED "0x%016x", self->imm);
    } else switch(self->imm.type->basic) {
    case ACIR_BASIC_VALUE_TYPE_SINT64: AnchWriteFormat(out, "%zu", self->imm.sint64); break;
    case ACIR_BASIC_VALUE_TYPE_UINT64: AnchWriteFormat(out, "%zu", self->imm.uint64); break;
    case ACIR_BASIC_VALUE_TYPE_SINT32: AnchWriteFormat(out, "%d", self->imm.sint32); break;
    case ACIR_BASIC_VALUE_TYPE_UINT32: AnchWriteFormat(out, "%u", self->imm.uint32); break;
    case ACIR_BASIC_VALUE_TYPE_SINT16: AnchWriteFormat(out, "%d", self->imm.sint16); break;
    case ACIR_BASIC_VALUE_TYPE_UINT16: AnchWriteFormat(out, "%u", self->imm.uint16); break;
    case ACIR_BASIC_VALUE_TYPE_SINT8: AnchWriteFormat(out, "%d", self->imm.sint8); break;
    case ACIR_BASIC_VALUE_TYPE_UINT8: AnchWriteFormat(out, "%u", self->imm.uint8); break;
    case ACIR_BASIC_VALUE_TYPE_FLOAT32: AnchWriteFormat(out, "%f", self->imm.float32); break;
    case ACIR_BASIC_VALUE_TYPE_FLOAT64: AnchWriteFormat(out, "%f", self->imm.float64); break;
    case ACIR_BASIC_VALUE_TYPE_BOOL: AnchWriteFormat(out, ANSI_CYAN "%s" ANSI_RESET, self->imm.boolean ? "true" : "false"); break;
    case ACIR_BASIC_VALUE_TYPE_VOID: AnchWriteString(out, ANSI_CYAN "void" ANSI_RESET); break;
    default:
      AnchWriteFormat(out, ANSI_RED "<bad basic value type (%d)>", self->imm.type->basic);
      AnchWriteFormat(out, ANSI_RED "0x%016x", self->imm);
    }
    AnchWriteString(out, ANSI_RESET);
  } else if(self->type == ACIR_OPERAND_TYPE_BINDING) {
    AnchWriteFormat(out, "$" ANSI_YELLOW "%zu" ANSI_RESET, self->idx);
  } else {
    AnchWriteFormat(out, ANSI_RED "<bad operand type (%d)>" ANSI_RESET, self->type);
  }
}

void AcirValueType_Print(const AcirValueType *self, AnchCharWriteStream *out) {
  if(!self) { AnchWriteFormat(out, ANSI_RED "<null value type>" ANSI_RESET); return; } 
  if(self->type == ACIR_VALUE_TYPE_BASIC) {
    AnchWriteFormat(out, ANSI_GRAY "%s" ANSI_RESET, AcirBasicValueType_Mnemonic(self->basic));
  } else if(self->type == ACIR_VALUE_TYPE_POINTER) {
    AnchWriteFormat(out, ANSI_GRAY "*" ANSI_RESET);
    AcirValueType_Print(self->pointer, out);
  } else {
    AnchWriteFormat(out, ANSI_RED "<bad value type (%d)>" ANSI_RESET, self->type);
  }
}

AcirInstrIndex Function_Print_(const AcirFunction *self, AnchCharWriteStream *out, AcirInstrIndex start, int indent) {
  for(const AcirInstr *instr = &self->instrs[start]; ; instr = &self->instrs[instr->next.idx]) {
    if(instr->opcode == ACIR_OPCODE_PHI) return instr->index;
    AcirInstr_PrintIndent(instr, wsStdout, indent);
    AnchWriteString(wsStdout, "\n");
    if(instr->opcode == ACIR_OPCODE_BR) {
      for(int i = 0; i < indent; ++i) AnchWriteString(wsStdout, "  ");
      AnchWriteString(wsStdout, ANSI_GRAY "--" ANSI_RESET ">" ANSI_GRAY " true:\n" ANSI_RESET);
      AcirInstrIndex ti = Function_Print_(self, out, instr->next.bt, indent + 1);
      for(int i = 0; i < indent; ++i) AnchWriteString(wsStdout, "  ");
      AnchWriteString(wsStdout, ANSI_GRAY "--" ANSI_RESET ">" ANSI_GRAY " false:\n" ANSI_RESET);
      AcirInstrIndex fi = Function_Print_(self, out, instr->next.bf, indent + 1);
      assert(fi == ti);
      AnchWriteString(wsStdout, ANSI_GRAY "--" ANSI_RESET ">" ANSI_GRAY "\n" ANSI_RESET);
      instr = &self->instrs[fi];
      AcirInstr_PrintIndent(instr, wsStdout, indent);
      AnchWriteString(wsStdout, "\n");
    }
    if(instr->next.idx == ACIR_INSTR_NULL_INDEX) return instr->index;
  }
}

void AcirFunction_Print(const AcirFunction *self, AnchCharWriteStream *out) {
  Function_Print_(self, out, self->code, 0);
}
