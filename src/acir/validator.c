#include <annec/ir.h>
#include <annec/anchor.h>
#include "cli.h"

#include <ctype.h>
#include <assert.h>

bool AcirBasicValueType_Equals(const AcirValueType *self, const AcirValueType *other) {
  assert(self != NULL);
  assert(other != NULL);
  if(self == other) return true;
  if(self->type != other->type) return false;
  if(self->type == ACIR_VALUE_TYPE_BASIC) return self->basic == other->basic;
  if(self->type == ACIR_VALUE_TYPE_POINTER) return AcirBasicValueType_Equals(self->pointer, other->pointer);
  if(self->type == ACIR_VALUE_TYPE_FUNCTION) {
    if(self->function->argumentCount != other->function->argumentCount) return false;
    if(!AcirBasicValueType_Equals(self->function->returnType, other->function->returnType)) return false;
    for(size_t i = 0; i < self->function->argumentCount; ++i) {
      if(!AcirBasicValueType_Equals(self->function->argumentTypes[i], other->function->argumentTypes[i]))
        return false;
    }
    return true;
  }
  return false;
}

static bool strprefix(const char *str, const char *pre) {
  return strncmp(pre, str, strlen(pre)) == 0;
}

typedef struct {
  bool exists;
  AcirValueType type;
} ValidationContext_Binding_;

typedef struct {
  size_t bindingCount;
  ValidationContext_Binding_ *bindings;
  AnchAllocator *allocator;
  int errorCount;
} ValidationContext_;

static ValidationContext_Binding_ *ValidationContext_GetBinding(const ValidationContext_ *self, size_t index) {
  assert(self != NULL);
  if(index >= self->bindingCount) return NULL;
  return &self->bindings[index];
}

static ValidationContext_Binding_ *ValidationContext_GetOrCreateBinding(ValidationContext_ *self, size_t index) {
  assert(self != NULL);
  if(index < self->bindingCount) return &self->bindings[index];

  size_t oldBindingCount = self->bindingCount;
  self->bindingCount = index + 1;

  if(self->bindings == NULL) {
    self->bindings = AnchAllocator_AllocZero(self->allocator,
      sizeof(ValidationContext_Binding_) * self->bindingCount);
  } else {
    self->bindings = AnchAllocator_Realloc(self->allocator,
      self->bindings, sizeof(ValidationContext_Binding_) * self->bindingCount);
    memset(self->bindings + oldBindingCount, 0, sizeof(ValidationContext_Binding_) * (self->bindingCount - oldBindingCount));
  }
  
  return &self->bindings[index];
}

static const AcirValueType *ValidationContext_TypeOf(const ValidationContext_ *self, const AcirOperand *op) {
  assert(self != NULL);
  assert(op != NULL);
  switch(op->type) {
    case ACIR_OPERAND_TYPE_BINDING: {
      ValidationContext_Binding_ *binding = ValidationContext_GetBinding(self, op->idx);
      if(binding == NULL) return NULL;
      return &binding->type;
    } break;
    case ACIR_OPERAND_TYPE_IMMEDIATE: return op->imm.type;
    default: return NULL;
  }
}

static void ValidationContext_Error_(ValidationContext_ *self, const AcirInstr *instr, const char *format, ...) {
  self->errorCount += 1;

  va_list va;
  va_start(va, format);

  const char *notes = NULL;
  if(*format == '!') {
    ++format;
    notes = format;
    while(*format != ';') ++format;
    ++format;
  }

  AnchWriteString(wsStderr, ANSI_RED "\nError: " ANSI_RESET);
  AnchWriteFormatV(wsStderr, format, va);
  AnchWriteString(wsStderr, "\n");
  AcirInstr_Print(instr, wsStderr);
  AnchWriteString(wsStderr, "\n");
  if(notes) AnchWriteString(wsStderr, "\n");
  while(notes && *notes != ';') {
    AnchWriteString(wsStderr, ANSI_GRAY "Note: " ANSI_RESET);
    switch(*notes) {
      case 'E':
        AnchWriteString(wsStderr, "expected `");
        AcirValueType_Print(va_arg(va, const AcirValueType *), wsStderr);
        AnchWriteString(wsStderr, "`, but got `");
        AcirValueType_Print(va_arg(va, const AcirValueType *), wsStderr);
        AnchWriteString(wsStderr, "` instead.");
        break;
      case 'G':
        AnchWriteString(wsStderr, "got `");
        AcirValueType_Print(va_arg(va, const AcirValueType *), wsStderr);
        AnchWriteString(wsStderr, "`");
        break;
      case 'A':
        AnchWriteString(wsStderr, "`");
        AcirValueType_Print(va_arg(va, const AcirValueType *), wsStderr);
        AnchWriteString(wsStderr, "` is not allowed.");
        break;
      case 'I':
        AnchWriteFormat(wsStderr, "Instruction count is %zu", va_arg(va, size_t));
        break;
      case 'O': {
        const AcirOperand *op = va_arg(va, const AcirOperand *);
        AnchWriteFormat(wsStderr, "got %s `", AcirOperandType_Name(op->type));
        AcirValueType_Print(ValidationContext_TypeOf(self, op), wsStderr);
        AnchWriteString(wsStderr, "`");
      } break;
      case 's':
        AnchWriteFormat(wsStderr, "instruction signature: `" ANSI_GRAY "%s" ANSI_RESET "`",
          AcirOpcode_Signature(instr->opcode));
        break;
      default:
        AnchWriteFormat(wsStderr, ANSI_RED "<bad note type ('%c' %d)>" ANSI_RESET, *notes, *notes);
    }
    AnchWriteString(wsStderr, "\n");
    ++notes;
  }

  if(notes) AnchWriteString(wsStderr, "\n");
  va_end(va);
}

static void ValidationContext_CheckInstr_(ValidationContext_ *self, const AcirInstr *instr) {
  if(instr->opcode == ACIR_OPCODE_PHI) {
    // ValidationContext_Binding_ *binding = .
    // if(binding != NULL && binding->exists)
    //     ValidationContext_Error_(self, instr,
    //       ANSI_RESET "binding $"
    //       ANSI_YELLOW "%d"
    //       ANSI_RESET " might be set multiple times.", op->idx);
    
    ValidationContext_Binding_ *binding = ValidationContext_GetOrCreateBinding(self, instr->out.idx);
    binding->exists = true;
    binding->type = *instr->type;
    return;
  }

  const char *sig = AcirOpcode_Signature(instr->opcode);
  assert(sig != NULL);

  while(isspace(*sig)) ++sig;

  char generic = 0;
  if(*sig == '.') {
    ++sig;
    generic = *sig;
    ++sig;
    while(isspace(*sig)) ++sig;
    while(*sig == '!') {
      ++sig;

      size_t length = 0;
      const char *start = sig;
      while(isalpha(*sig)) { ++sig; ++length; }
      while(isspace(*sig)) ++sig;

      for(size_t i = 0; i < ACIR_BASIC_VALUE_TYPE_MAX_; ++i) {
        const char *mnemonic = AcirBasicValueType_Mnemonic(i);
        assert(mnemonic != NULL);
        if(length == strlen(mnemonic) && strncmp(mnemonic, start, length) == 0) {
          AcirValueType type = { ACIR_VALUE_TYPE_BASIC, .basic = i };
          if(AcirBasicValueType_Equals(instr->type, &type)) {
            ValidationContext_Error_(self, instr, "!A;generic instruction does not allow provided type.", &type);
          }

          break;
        }
      }
    }
  }
  
  while(isspace(*sig)) ++sig;
  
  int index = 0;
  while(*sig) {
    while(isspace(*sig)) ++sig;

    bool writable = false;
    bool lvalue = false;
    bool pointer = false;
    
    if(*sig == 'w') { writable = true; ++sig; }
    if(*sig == 'l') { lvalue = true; ++sig; }
    if(*sig == '*') { pointer = true; ++sig; }
    
    size_t length = 0;
    const char *start = sig;
    while(isalpha(*sig)) { ++sig; ++length; }
    while(isspace(*sig)) ++sig;
    bool last = *sig != ',';
    ++sig;

    const AcirValueType *typeRef = NULL;
    AcirValueType typeValue;
    AcirValueType pointerTypeValue;
    
    if(length == 1 && generic && *start == generic) {
      typeRef = instr->type;
    } else if(strncmp("any", start, 3) != 0) {
      for(size_t i = 0; i < ACIR_BASIC_VALUE_TYPE_MAX_; ++i) {
        const char *mnemonic = AcirBasicValueType_Mnemonic(i);
        assert(mnemonic != NULL);
        if(length == strlen(mnemonic) && strncmp(mnemonic, start, length) == 0) {
          typeValue = (AcirValueType){ ACIR_VALUE_TYPE_BASIC, .basic = i };
          typeRef = &typeValue;
          break;
        }
      }
    }

    if(pointer) {
      pointerTypeValue = (AcirValueType){ ACIR_VALUE_TYPE_POINTER, .pointer = typeRef };
      typeRef = &pointerTypeValue;
    }

    const AcirOperand *op = NULL;
    const char *opname = "<unknown>";
    if(index == 0) { op = &instr->val; opname = "Value"; }
    if(index == 1 && last) { op = &instr->out; opname = "Output"; }
    if(index == 1 && !last) { op = &instr->rhs; opname = "Second value"; }
    if(index == 2) { op = &instr->out; opname = "Output"; }

    ValidationContext_Binding_ *binding = op->type == ACIR_OPERAND_TYPE_BINDING
      ? ValidationContext_GetBinding(self, op->idx) : NULL;
    
    if(writable || lvalue) {
      if(op->type != ACIR_OPERAND_TYPE_BINDING) {
        ValidationContext_Error_(self, instr, "!Os;%s argument (#%d) must be either writable or an lvalue.",
          opname, index + 1, op);
      } else {
        if(binding != NULL && binding->exists)
            ValidationContext_Error_(self, instr,
              ANSI_RESET "binding $"
              ANSI_YELLOW "%d"
              ANSI_RESET " might be set multiple times.", op->idx);
        
        binding = ValidationContext_GetOrCreateBinding(self, op->idx);
        binding->exists = true;
        binding->type = *typeRef;
      }
    } else {
      if(op->type == ACIR_OPERAND_TYPE_BINDING && (binding == NULL || !binding->exists))
          ValidationContext_Error_(self, instr, "binding $%d doesn't exist.", op->idx);
    }

    const AcirValueType *opType = ValidationContext_TypeOf(self, op);
    if(opType == NULL) {
      ValidationContext_Error_(self, instr, "%s argument (#%d) doesn't have a type.",
        opname, index + 1);
    } else if(typeRef != NULL && !AcirBasicValueType_Equals(typeRef, opType)) {
      ValidationContext_Error_(self, instr, "!Es;%s argument (#%d) did not match type.",
        opname, index + 1, typeRef, opType);
    }

    if(instr->opcode == ACIR_OPCODE_RET && instr->next.idx != ACIR_INSTR_NULL_INDEX) {
      ValidationContext_Error_(self, instr, "the `" ANSI_BLUE "ret" ANSI_RESET "` instruction has to be last.");
    }

    ++index;
    if(last) break;
  }
}

int AcirFunction_Validate(AcirFunction *self, AnchAllocator *allocator) {
  assert(self != NULL);
  
  ValidationContext_ context = {0};
  context.allocator = allocator;

  for(const AcirInstr *instr = &self->instrs[self->code]; instr != NULL;) {
    ValidationContext_CheckInstr_(&context, instr);
    if(instr->next.idx == ACIR_INSTR_NULL_INDEX) break;
    if(instr->next.idx >= self->instrCount)
      ValidationContext_Error_(&context, instr,
        "!I;Non-existent next instruction %zu.", instr->next.idx, self->instrCount);
    instr = &self->instrs[instr->next.idx];
  }

  if(context.bindingCount > 0)
    AnchAllocator_Free(context.allocator, context.bindings);
  
  return context.errorCount;
}
