#pragma once
#include <stdint.h>

typedef struct {
  uint8_t *ip;
} VM;

typedef struct {
} Chunk;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);

