#include <acir/acir.h>
#include <annec_anchor.h>
#include "../cli.h"

AnchAllocator *allocator;
AnchCharWriteStream *wsStdout;
AnchCharWriteStream *wsStderr;

int main(int argc, char *argv[]) {
  AnchFileWriteStream valueWsStdout = {0};
  AnchFileWriteStream_InitWith(&valueWsStdout, stdout);
  wsStdout = &valueWsStdout.stream;
  
  AnchFileWriteStream valueWsStderr = {0};
  AnchFileWriteStream_InitWith(&valueWsStderr, stderr);
  wsStderr = &valueWsStderr.stream;

  AnchDefaultAllocator defaultAllocator;
  AnchDefaultAllocator_Init(&defaultAllocator);

  AnchStatsAllocator statsAllocator;
  AnchStatsAllocator_Init(&statsAllocator, &defaultAllocator);

  allocator = &statsAllocator.base;

  AcirValueType valueTuint64 = { .type = ACIR_VALUE_TYPE_BASIC, .basic = ACIR_BASIC_VALUE_TYPE_UINT64 };
  const AcirValueType *Tuint64 = &valueTuint64;
  AcirValueType valueTvoid = { .type = ACIR_VALUE_TYPE_BASIC, .basic = ACIR_BASIC_VALUE_TYPE_VOID };
  const AcirValueType *Tvoid = &valueTvoid;
  (void)Tvoid;
  
  AcirInstr instrs[] = {
    (AcirInstr){ 0, ACIR_OPCODE_SET, Tuint64, 1,
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 0 },
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_IMMEDIATE, {{Tuint64}}, .imm.uint64 = 64 }, },
    (AcirInstr){ 1, ACIR_OPCODE_SET, Tuint64, 2,
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 1 },
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_IMMEDIATE, {{Tuint64}}, .imm.uint64 = 12 }, },
    (AcirInstr){ 2, ACIR_OPCODE_ADD, Tuint64, 3,
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 2 },
      .lhs = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 1 },
      .rhs = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 0 }, },
    (AcirInstr){ 3, ACIR_OPCODE_EFF, Tuint64, 4,
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 0 },
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 3 }, },
    (AcirInstr){ 4, ACIR_OPCODE_SET, Tuint64, 5,
      .out = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 4 },
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_IMMEDIATE, {{Tuint64}}, .imm.uint64 = 15 }, },
    (AcirInstr){ 5, ACIR_OPCODE_RET, Tuint64, ACIR_INSTR_NULL_INDEX,
      .val = (AcirOperand){ ACIR_OPERAND_TYPE_BINDING, .idx = 2 }, },
  };

  AcirFunction inputFunc = {
    .type = NULL,
    .code = 0,
    .name = "main",
    .instrs = instrs,
    .instrCount = sizeof(instrs) / sizeof(AcirInstr)
  };

  WRITE_SEPARATOR("Generated IR");
  AcirFunction_Print(&inputFunc, wsStdout);

  WRITE_SEPARATOR1("Validation", "=");
  int errorCount = AcirFunction_Validate(&inputFunc, allocator);

  if(errorCount > 0) {
    AnchWriteFormat(wsStderr, ANSI_RED "\n%d Errors, Aborting.\n" ANSI_RESET, errorCount);
    return 0;
  } else {
    AnchWriteFormat(wsStdout, ANSI_GREEN "\nNo Errors.\n" ANSI_RESET);
  }

  AcirFunction optimizerFunc = { .type = NULL, .name = "main" };
  AcirBuilder optimizerBuilder;
  AcirBuilder_Init(&optimizerBuilder, &optimizerFunc, allocator);

  AcirFunction outputFunc = { .type = NULL, .name = "main" };
  AcirBuilder outputBuilder;
  AcirBuilder_Init(&outputBuilder, &outputFunc, allocator);

  AcirOptimizer optimizer;
  AcirOptimizer_Init(&optimizer, &(const AcirOptimizer_InitInfo){
    .allocator = allocator,
    .source = &inputFunc,
    .builder = &optimizerBuilder
  });

  AcirOptimizer_Analyze(&optimizer);
  AcirOptimizer_ConstantFold(&optimizer);
  AcirOptimizer_DeadCode(&optimizer);

  AcirBuilder_BuildNormalized(&optimizerBuilder, &outputBuilder);

  AcirBuilder_Free(&optimizerBuilder);
  AcirOptimizer_Free(&optimizer);

  WRITE_SEPARATOR("Optimized IR");
  AcirFunction_Print(&outputFunc, wsStdout);

  AcirBuilder_Free(&outputBuilder);
}
