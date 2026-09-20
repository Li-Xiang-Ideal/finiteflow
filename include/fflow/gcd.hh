#ifndef FFLOW_GCD_HH
#define FFLOW_GCD_HH

#include <fflow/common.hh>

namespace fflow {

  // multiplicative inverse a^(-1) mod n
  inline UInt mul_inv(UInt a, Mod n)
  {
    return ffMulInverseMod(a, n.mod_);
  }

  // a/b mod n
  inline UInt div_mod(UInt a, UInt b, Mod n)
  {
    b = mul_inv(b, n);
    return mul_mod(a, b, n);
  }

  // rational reconstruction: num,den such that num/den = a mod b
  void rat_rec(UInt a, UInt b, Int & num, Int & den);


  // Rational

  inline Rational rat_rec(UInt a, UInt b)
  {
    Rational res;
    rat_rec(a,b,res.num,res.den);
    return res;
  }

} // namespace fflow

#endif // FFLOW_GCD_HH
