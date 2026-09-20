#include <fflow/capi.h>
#include <fflow/gcd.hh>
#include <fflow/mp_gcd.hh>
#include <cstdio>
#include <random>
#include <string>

using namespace fflow;

namespace {
  unsigned checks = 0, failures = 0;
  void check(bool ok, const char * message)
  {
    ++checks;
    if (!ok) {
      ++failures;
      std::fprintf(stderr, "FAIL: %s\n", message);
    }
  }

  // The reference uses decimal GMP conversion, not the compatibility layer.
  void set_reference(mpz_ptr out, UInt value)
  {
    mpz_set_str(out, std::to_string(value).c_str(), 10);
  }

  bool equals_reference(UInt actual, mpz_srcptr expected)
  {
    mpz_t z;
    mpz_init(z);
    set_reference(z, actual);
    const bool equal = mpz_cmp(z, expected) == 0;
    mpz_clear(z);
    return equal;
  }

  void test_modular_arithmetic()
  {
    std::mt19937_64 random(20260920);
    mpz_t a, b, p, expected;
    mpz_inits(a, b, p, expected, nullptr);
    const unsigned prime_indexes[] = {0, 100, 1000};
    for (unsigned index : prime_indexes) {
      const UInt prime = ffPrimeNo(index);
      const Mod mod(prime);
      set_reference(p, prime);
      for (unsigned j=0; j<128; ++j) {
        const UInt x = j == 0 ? prime-1 : random() % prime;
        const UInt y = j == 0 ? prime-1 : random() % (prime-1) + 1;
        set_reference(a, x);
        set_reference(b, y);
        mpz_mul(expected, a, b);
        mpz_mod(expected, expected, p);
        check(equals_reference(mul_mod(x, y, mod), expected), "ffmod multiplication agrees with GMP");
        check(equals_reference(mul_mod_shoup(x, y, precomp_mul_shoup(y, mod), mod), expected),
              "Shoup multiplication agrees with GMP");
        mpz_add(expected, a, b);
        mpz_mod(expected, expected, p);
        check(equals_reference(add_mod(x, y, mod), expected), "ffmod addition agrees with GMP");
        mpz_invert(expected, b, p);
        check(equals_reference(mul_inv(y, mod), expected), "modular inverse agrees with GMP");
      }
    }
    mpz_clears(a, b, p, expected, nullptr);
  }

  void test_chinese_remainder()
  {
    const UInt q = ffPrimeNo(1);
    const MPInt p(std::to_string(ffPrimeNo(0)).c_str());
    MPInt c1, c2, modulus, result;
    chinese_remainder_coeffs(p, q, c1, c2, modulus);
    const MPInt a("4294967297");
    const UInt b = q-1;
    chinese_remainder_from_coeffs(a, b, c1, c2, modulus, result);
    mpz_t qz, expected, rem;
    mpz_inits(qz, expected, rem, nullptr);
    set_reference(qz, q);
    mpz_mul(expected, p.get(), qz);
    check(mpz_cmp(modulus.get(), expected) == 0, "CRT modulus keeps 63-bit factors");
    mpz_mod(rem, result.get(), p.get());
    check(mpz_cmp(rem, a.get()) == 0, "CRT first residue is preserved");
    mpz_mod(rem, result.get(), qz);
    set_reference(expected, b);
    check(mpz_cmp(rem, expected) == 0, "CRT second residue is preserved");
    check(result.sign() >= 0 && result.cmp(modulus) < 0, "CRT result is canonical");
    mpz_clears(qz, expected, rem, nullptr);
  }

  void test_aliased_arithmetic()
  {
    MPInt value("3");
    const UInt max = UINT64_MAX;
    detail::mpz_mul_u64(value.get(), value.get(), max);
    check(value.cmp(MPInt("55340232221128654845")) == 0,
          "in-place multiplication keeps full 64-bit factor");
    detail::mpz_u64_sub(value.get(), max, value.get());
    check(value.cmp(MPInt("-36893488147419103230")) == 0,
          "in-place subtraction keeps full 64-bit minuend");
    const MPInt negative("-18446744073709551617");
    check(detail::mpz_fdiv_u64(negative.get(), max) == UINT64_C(18446744073709551613),
          "negative dividend has nonnegative 64-bit remainder");
  }

  void test_expression_parser()
  {
    const char * expressions[] = {
      "4294967297*x", "-4294967297*x", "1/4294967297",
      "4611686018427387903+x", "4611686018427387904+x",
      "9223372036854775807+x", "18446744073709551615+x",
      "4294967297/4294967299*x"
    };
    const char * expected_values[] = {
      "12884901891", "-12884901891", "1/4294967297",
      "4611686018427387906", "4611686018427387907",
      "9223372036854775810", "18446744073709551618",
      "12884901891/4294967299"
    };
    const unsigned count = sizeof(expressions)/sizeof(expressions[0]);
    const char * vars[] = {"x"};
    FFNode input;
    const FFGraph graph = ffNewGraphWithInput(1, &input);
    const FFNode node = ffAlgRatExprEval(graph, input, vars, 1, nullptr, expressions, count);
    check(!ffIsError(node), "expression graph parses large coefficients");
    if (ffIsError(node)) {
      ffDeleteGraph(graph);
      return;
    }
    check(ffSetOutputNode(graph, node) == FF_SUCCESS, "expression output is selected");
    const FFUInt x[] = {3};
    FFUInt * output = ffEvaluateGraph(graph, x, 0);
    check(output != nullptr, "expression graph evaluates");
    if (output) {
      mpq_t rational;
      mpz_t p, inverse, expected;
      mpq_init(rational);
      mpz_inits(p, inverse, expected, nullptr);
      set_reference(p, ffPrimeNo(0));
      for (unsigned j=0; j<count; ++j) {
        mpq_set_str(rational, expected_values[j], 10);
        mpq_canonicalize(rational);
        mpz_invert(inverse, mpq_denref(rational), p);
        mpz_mul(expected, mpq_numref(rational), inverse);
        mpz_mod(expected, expected, p);
        check(equals_reference(output[j], expected), expressions[j]);
      }
      mpq_clear(rational);
      mpz_clears(p, inverse, expected, nullptr);
      ffFreeMemoryU64(output);
    }
    ffDeleteGraph(graph);
  }
}

int main()
{
  ffInit();
  test_modular_arithmetic();
  test_chinese_remainder();
  test_aliased_arithmetic();
  test_expression_parser();
  ffDeinit();
  std::printf("%u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
