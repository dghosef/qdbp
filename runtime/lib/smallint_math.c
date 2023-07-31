#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "runtime.h"
static int64_t int_max = 4611686018427387903;   // biggest 63 bit signed int
static int64_t int_min = -4611686018427387904;  // smallest 63 bit signed int

static _qdbp_object_ptr bigint_op(_qdbp_bigint_arith_fn op, uint64_t l,
                                  uint64_t r) {
  l = _qdbp_sign_extend(l);
  r = _qdbp_sign_extend(r);
  mpz_t r_mpz;
  mpz_init_set_si(r_mpz, r);
  _qdbp_object_ptr ret = _qdbp_make_boxed_int();
  mpz_set_si(ret->data.boxed_int->value, l);
  op(ret->data.boxed_int->value, ret->data.boxed_int->value, r_mpz);
  mpz_clear(r_mpz);
  return ret;
}

/*
 * A collection of safe math functions * These functions implement signed 63-bit
 * arithmetic * for the qdbp language.  * Each function is of the form:
 *   `uint64_t _qdbp_<operation>(uint64_t a, uint64_t b)`
 * where <operation> is the name of the operation.  * `a` and `b` must be 63 bit
 * integers. i.e. their top * bit must be `0`. The top bit will be used for
 * overflow checking
 */

#define CHECK_ARITH_INPUT(a, b)           \
  do {                                    \
    _qdbp_assert((((a) >> 63) & 1) == 0); \
    _qdbp_assert((((b) >> 63) & 1) == 0); \
  } while (0)

#define DO_ARITH(op, a, b) (((_qdbp_sign_extend(a) op _qdbp_sign_extend(b))))

static void math_assert(bool cond, const char* fmt, ...) {
  if (!cond) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    _qdbp_abort();
  }
}

static uint64_t zero_highbit(int64_t x) { return (uint64_t)x & ~(1ULL << 63); }

int64_t _qdbp_sign_extend(uint64_t a) {
  if ((a >> 62) & 1) {
    return a | (1ULL << 63);
  } else {
    return a;
  }
}

static bool fits_in_63_bits(int64_t x) { return x <= int_max && x >= int_min; }

_qdbp_object_ptr _qdbp_smallint_add(uint64_t a, uint64_t b) {
  CHECK_ARITH_INPUT(a, b);
  int64_t res = DO_ARITH(+, a, b);
  if (!fits_in_63_bits(res)) {
    return bigint_op(mpz_add, a, b);
  }
  res = zero_highbit(res);
  return _qdbp_make_unboxed_int(res);
}

_qdbp_object_ptr _qdbp_smallint_sub(uint64_t a, uint64_t b) {
  CHECK_ARITH_INPUT(a, b);
  int64_t res = DO_ARITH(-, a, b);
  if (!fits_in_63_bits(res)) {
    return bigint_op(mpz_sub, a, b);
  }
  res = zero_highbit(res);
  return _qdbp_make_unboxed_int(res);
}

_qdbp_object_ptr _qdbp_smallint_mul(uint64_t a, uint64_t b) {
  CHECK_ARITH_INPUT(a, b);
  int64_t a_signed = _qdbp_sign_extend(a);
  int64_t b_signed = _qdbp_sign_extend(b);
  int64_t res = a_signed * b_signed;
  if (!(fits_in_63_bits(res) && ((a == 0 || b == 0) || res / a == b))) {
    return bigint_op(mpz_mul, a, b);
  }
  res = zero_highbit(res);
  return _qdbp_make_unboxed_int(res);
}

_qdbp_object_ptr _qdbp_smallint_div(uint64_t a, uint64_t b) {
  CHECK_ARITH_INPUT(a, b);
  math_assert(b != 0, "Division of %lli by zero",
              (int64_t)_qdbp_sign_extend(a));
  uint64_t res = DO_ARITH(/, a, b);
  if (!fits_in_63_bits(res)) {
    return bigint_op(mpz_tdiv_q, a, b);
  }
  return _qdbp_make_unboxed_int(res);
}

_qdbp_object_ptr _qdbp_smallint_mod(uint64_t a, uint64_t b) {
  CHECK_ARITH_INPUT(a, b);
  math_assert(b != 0, "Modulo of %lli by zero", (int64_t)_qdbp_sign_extend(b));
  uint64_t res = DO_ARITH(%, a, b);
  if (!fits_in_63_bits(res)) {
    return bigint_op(mpz_tdiv_r, a, b);
  }
  return _qdbp_make_unboxed_int(res);
}