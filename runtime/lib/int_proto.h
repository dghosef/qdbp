#ifndef INT_PROTO_H
#define INT_PROTO_H
#include "runtime.h"
// MUST KEEP IN SYNC WITH namesToInts.ml
enum LABELS {
  PLUS = 0,
  MINUS = 1,
  MUL = 2,
  DIV = 3,
  MOD = 4,
  EQ = 5,
  NEQ = 6,
  LT = 7,
  GT = 8,
  LEQ = 9,
  GEQ = 10,
  VAL = 11,
  ASSTR = 12,
  PRINT = 13,
  PROTO_SIZE // MUST be last
};
#endif