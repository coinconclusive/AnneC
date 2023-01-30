#include <acir/acir.h>
#include <annec_anchor.h>
#include "cli.h"

AnchCharWriteStream *wsStdout;
AnchAllocator *allocator;

int main(int argc, char *argv[]) {
  AnchFileWriteStream valueWsStdout = {0};
  AnchFileWriteStream_InitWith(&valueWsStdout, stdout);
  wsStdout = &valueWsStdout.stream;

  AnchDefaultAllocator defaultAllocator;
  AnchDefaultAllocator_Init(&defaultAllocator);

  AnchStatsAllocator statsAllocator;
  AnchStatsAllocator_Init(&statsAllocator, &defaultAllocator);

  allocator = &statsAllocator.base;

  AcirValueType valueTuint64 = { .type = ACIR_VALUE_TYPE_BASIC, .basic = ACIR_BASIC_VALUE_TYPE_UINT64 };
  AcirValueType valueTuint32 = { .type = ACIR_VALUE_TYPE_BASIC, .basic = ACIR_BASIC_VALUE_TYPE_UINT32 };

  const AcirValueType *Tuint64 = &valueTuint64;
  const AcirValueType *Tuint32 = &valueTuint32;
  
  AcirInstr instrs[] = {
    (AcirInstr){ 0, ACIR_OPCODE_SET, Tuint64, &instrs[1],
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 0 },
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_IMMEDIATE, Tuint64, .imm.uint64 = 64 }, },
    (AcirInstr){ 1, ACIR_OPCODE_SET, Tuint64, &instrs[2],
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 1 },
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_IMMEDIATE, Tuint32, .imm.uint64 = 12 }, },
    (AcirInstr){ 2, ACIR_OPCODE_ADD, Tuint64, &instrs[3],
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 2 },
      .lhs = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 1 },
      .rhs = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 0 }, },
    (AcirInstr){ 3, ACIR_OPCODE_RET, Tuint64, &instrs[4],
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 2 }, },
    (AcirInstr){ 4, ACIR_OPCODE_SET, Tuint64, NULL,
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 0 },
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_IMMEDIATE, Tuint64, .imm.uint64 = 3 }, },
  };

  puts(ANSI_GRAY "\n============[ " ANSI_MAGENTA "IR" ANSI_GRAY " ]============" ANSI_RESET);
  for(int i = 0; i < 5; ++i) {
    AcirInstr_Print(wsStdout, &instrs[i]);
    AnchWriteString(wsStdout, "\n");
  }

  AcirFunction func = { .type = NULL, .code = instrs, .name = "main" };
  AcirFunction_Validate(&func, allocator);
}
