#include <stdio.h>

#include "runtime.h"

static void mpz_si_op(mpz_t dest, int64_t l, const mpz_t r,
                      _qdbp_bigint_arith_fn op) {
  mpz_t l_mpz;
  mpz_init_set_si(l_mpz, l);
  op(dest, l_mpz, r);
  mpz_clear(l_mpz);
}

static void mpz_op_si(mpz_t dest, const mpz_t l, int64_t r,
                      _qdbp_bigint_arith_fn op) {
  mpz_t r_mpz;
  mpz_init_set_si(r_mpz, r);
  op(dest, l, r_mpz);
  mpz_clear(r_mpz);
}

static void get_arith_fns(_qdbp_bigint_arith_fn **bigint_fn,
                          _qdbp_smallint_arith_fn **smallint_fn,
                          enum _QDBP_ARITH_OP op) {
  switch (op) {
    case _QDBP_ADD:
      *bigint_fn = mpz_add;
      *smallint_fn = _qdbp_smallint_add;
      break;
    case _QDBP_SUB:
      *bigint_fn = mpz_sub;
      *smallint_fn = _qdbp_smallint_sub;
      break;
    case _QDBP_MUL:
      *bigint_fn = mpz_mul;
      *smallint_fn = _qdbp_smallint_mul;
      break;
    case _QDBP_DIV:
      *bigint_fn = mpz_tdiv_q;
      *smallint_fn = _qdbp_smallint_div;
      break;
    case _QDBP_MOD:
      *bigint_fn = mpz_tdiv_r;
      *smallint_fn = _qdbp_smallint_mod;
      break;
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
}

// Perform a default mathematical operation on `l` and `r`
static _qdbp_object_ptr bigint_arith(_qdbp_bigint_arith_fn bigint_op,
                                     _qdbp_smallint_arith_fn smallint_op,
                                     _qdbp_object_ptr l, _qdbp_object_ptr r) {
  // If both are smallints, simply return smallint_op
  if (_qdbp_is_unboxed_int(l) && _qdbp_is_unboxed_int(r)) {
    return smallint_op(_qdbp_get_unboxed_int(l), _qdbp_get_unboxed_int(r));
  } else if (_qdbp_is_unboxed_int(l)) {
    // If l is unboxed, the new boxed int will have no extra labels
    _qdbp_assert(_qdbp_is_boxed_int(r));
    if (_qdbp_is_unique(r) && _QDBP_REUSE_ANALYSIS) {
      r->data.boxed_int->prototype.label_map = NULL;
      mpz_si_op(r->data.boxed_int->value, _qdbp_get_unboxed_int(l),
                r->data.boxed_int->value, bigint_op);
      return r;
    } else {
      _qdbp_drop(r, 1);
      _qdbp_object_ptr ret = _qdbp_make_boxed_int();

      mpz_si_op(ret->data.boxed_int->value, _qdbp_get_unboxed_int(l),
                r->data.boxed_int->value, bigint_op);
      return ret;
    }
  } else {
    // if l is boxed, the new bxoed int will keep l's labels so we have to be
    // careful to keep them
    _qdbp_object_ptr dest;
    if (_qdbp_is_unique(l) && _QDBP_REUSE_ANALYSIS) {
      dest = l;
    } else {
      _qdbp_drop(l, 1);
      if (_qdbp_is_unique(r) && _QDBP_REUSE_ANALYSIS) {
        dest = r;
        _qdbp_del_prototype(&dest->data.boxed_int->prototype);
        _qdbp_dup(r, 1);
      } else {
        dest = _qdbp_make_boxed_int();
      }
      _qdbp_copy_prototype(&l->data.boxed_int->prototype,
                           &dest->data.boxed_int->prototype);
      _qdbp_dup_prototype_captures(&l->data.boxed_int->prototype);
    }

    if (_qdbp_is_unboxed_int(r)) {
      mpz_op_si(dest->data.boxed_int->value, dest->data.boxed_int->value,
                _qdbp_get_unboxed_int(r), bigint_op);
      return dest;
    } else {
      _qdbp_drop(r, 1);
      bigint_op(dest->data.boxed_int->value, dest->data.boxed_int->value,
                r->data.boxed_int->value);
      return dest;
    }
  }
}

// return a negative number if l < r, 0 if l == r, and a positive number if l >
// r
static int bigint_compare(_qdbp_object_ptr l, _qdbp_object_ptr r) {
  int res;
  if (_qdbp_is_unboxed_int(l) && _qdbp_is_unboxed_int(r)) {
    res = _qdbp_get_unboxed_int(l) - _qdbp_get_unboxed_int(r);
  } else if (_qdbp_is_unboxed_int(l)) {
    res = -mpz_cmp_si(r->data.boxed_int->value, _qdbp_get_unboxed_int(l));
  } else if (_qdbp_is_unboxed_int(r)) {
    res = mpz_cmp_si(l->data.boxed_int->value, _qdbp_get_unboxed_int(r));
  } else {
    res = mpz_cmp(l->data.boxed_int->value, r->data.boxed_int->value);
  }
  _qdbp_drop(l, 1);
  _qdbp_drop(r, 1);
  return res;
}

static _qdbp_object_ptr bigint_cmp_op(_qdbp_object_ptr l, _qdbp_object_ptr r,
                                      enum _QDBP_ARITH_OP op) {
  switch (op) {
    case _QDBP_LT:
      return _qdbp_bool(bigint_compare(l, r) < 0);
      break;
    case _QDBP_LEQ:
      return _qdbp_bool(bigint_compare(l, r) <= 0);
      break;
    case _QDBP_EQ:
      return _qdbp_bool(bigint_compare(l, r) == 0);
      break;
    case _QDBP_NEQ:
      return _qdbp_bool(bigint_compare(l, r) != 0);
      break;
    case _QDBP_GT:
      return _qdbp_bool(bigint_compare(l, r) > 0);
      break;
    case _QDBP_GEQ:
      return _qdbp_bool(bigint_compare(l, r) >= 0);
      break;
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
}

static _qdbp_object_ptr bigint_arith_op(_qdbp_object_ptr l, _qdbp_object_ptr r,
                                        enum _QDBP_ARITH_OP op) {
  _qdbp_bigint_arith_fn *bigint_fn;
  _qdbp_smallint_arith_fn *smallint_fn;
  get_arith_fns(&bigint_fn, &smallint_fn, op);
  return bigint_arith(bigint_fn, smallint_fn, l, r);
}

_qdbp_object_ptr _qdbp_int_binary_op(_qdbp_object_ptr l, _qdbp_object_ptr r,
                                     enum _QDBP_ARITH_OP op) {
  _qdbp_assert(op < _QDBP_MAX_OP);
  _qdbp_assert(op >= _QDBP_ADD);
  if (op >= _QDBP_EQ) {
    return bigint_cmp_op(l, r, op);
  } else {
    return bigint_arith_op(l, r, op);
  }
}

_qdbp_object_ptr _qdbp_int_unary_op(_qdbp_object_ptr receiver,
                                    enum _QDBP_ARITH_OP op) {
  _qdbp_assert(op < _QDBP_ADD);
  switch (op) {
    case _QDBP_PRINT:
      if (_qdbp_is_unboxed_int(receiver)) {
        printf("%llu\n", _qdbp_get_unboxed_int(receiver));
      } else {
        _qdbp_assert(_qdbp_is_boxed_int(receiver));
        mpz_out_str(stdout, 10, receiver->data.boxed_int->value);
        _qdbp_drop(receiver, 1);
      }
      break;
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
  return _qdbp_empty_prototype();
}