#include <fflow/mp_common.hh>
#include <cstdio>
#include <limits>
#include <string>

using namespace fflow;

namespace {
  unsigned failures = 0;
  unsigned checks = 0;

  void check(bool ok, const char * message)
  {
    ++checks;
    if (!ok) {
      ++failures;
      std::fprintf(stderr, "FAIL: %s\n", message);
    }
  }

  int sign(int n) { return (n > 0) - (n < 0); }

  void test_integers()
  {
    const UInt values[] = {0, 1, 2147483648ULL, 4294967295ULL,
      4294967296ULL, 4294967297ULL, 4611686018427387904ULL,
      9223372036854775783ULL, 9223372036854775808ULL,
      std::numeric_limits<UInt>::max()};
    for (UInt n : values) {
      MPInt expected(std::to_string(n).c_str());
      MPInt actual(n);
      check(actual.cmp(expected) == 0, "MPInt constructor preserves uint64");
      check(expected.to_uint() == n, "MPInt to_uint preserves uint64");
      check(expected.cmp(n) == 0, "MPInt comparison preserves uint64");
      actual = UInt(7);
      actual = n;
      check(actual.cmp(expected) == 0, "MPInt assignment preserves uint64");
    }
    const Int signed_values[] = {std::numeric_limits<Int>::min(),
      -4611686018427387904LL, -4294967297LL, -2147483648LL, -1, 0,
      4294967297LL, std::numeric_limits<Int>::max()};
    for (Int n : signed_values) {
      MPInt value(std::to_string(n).c_str());
      check(value.to_int() == n, "MPInt to_int preserves int64");
    }
    // GMP/FLINT unsigned extraction takes the low bits of the magnitude.
    check(MPInt("-18446744073709551617").to_uint() == 1,
          "unsigned extraction truncates magnitude to 64 bits");
    check(MPInt("18446744073709551617").to_int() == 1,
          "signed extraction truncates positive magnitude");
    check(MPInt("-18446744073709551617").to_int() == -1,
          "signed extraction preserves sign when truncating");
    const Int overflow_expected = sizeof(long) >= sizeof(Int)
      ? std::numeric_limits<Int>::max() : Int(-1);
    check(MPInt("18446744073709551615").to_int() == overflow_expected,
          "signed extraction preserves each platform's previous behavior");
  }

  void test_rationals()
  {
    const Int numerators[] = {std::numeric_limits<Int>::min(),
      -4611686018427387904LL, -4294967297LL, -1, 0, 1,
      2147483648LL, 4294967297LL, std::numeric_limits<Int>::max()};
    const Int denominators[] = {1, 3, 4294967297LL, 9223372036854775783LL};
    for (Int num : numerators) {
      for (Int den : denominators) {
        const std::string text = std::to_string(num) + "/" + std::to_string(den);
        MPRational expected(text.c_str());
        MPRational actual(num, den);
        check(actual == expected, "rational constructor preserves int64 numerator/denominator");
        check(expected.cmp(num, den) == 0, "rational comparison preserves int64 operands");
        Rational pair{num, den};
        MPRational from_pair(pair);
        check(from_pair == expected, "Rational conversion preserves int64");
        actual = pair;
        check(actual == expected, "Rational assignment preserves int64");
        check(expected.cmp(pair) == 0, "Rational comparison preserves int64");
      }
      MPRational assigned;
      assigned = num;
      MPRational expected(std::to_string(num).c_str());
      check(assigned == expected, "integer assignment to rational preserves int64");
      Int out_num, out_den;
      expected.to_int(out_num, out_den);
      check(out_num == num && out_den == 1, "rational extraction preserves int64");
    }
    MPRational large("4294967297");
    check(sign(large.cmp(Int(1) << 62)) == -1, "large coefficient is below positive 2^62 threshold");
    check(sign(large.cmp(-(Int(1) << 62))) == 1, "large coefficient is above negative 2^62 threshold");
    check(sign(MPRational("-4294967297").cmp(-1)) == -1,
          "negative large coefficient compares below -1");
    check(MPRational(1, 4294967296LL) == MPRational("1/4294967296"),
          "large denominator must not truncate to zero");
    check(MPRational(1, std::numeric_limits<Int>::min()) == MPRational("-1/9223372036854775808"),
          "INT64_MIN denominator is normalized without overflow");
    check(MPRational(-1, -4294967297LL) == MPRational("1/4294967297"),
          "negative numerator and denominator are normalized");
    check(MPRational("1/4294967297").cmp(-1, -4294967297LL) == 0,
          "comparison normalizes negative denominators");
  }
}

int main()
{
  std::printf("long=%u bits, Int=%u bits\n", unsigned(sizeof(long)*8), unsigned(sizeof(Int)*8));
  test_integers();
  test_rationals();
  std::printf("%u checks, %u failures\n", checks, failures);
  return failures ? 1 : 0;
}
