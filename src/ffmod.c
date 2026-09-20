#if FFLOW_USE_FLINT
# include <flint.h>
# include <ulong_extras.h>
#endif
#include "fflow/ffmod.h"

#if FFLOW_USE_FLINT

FFU64 ffMulInverseMod(FFU64 z, FFMod p)
{
  return n_invmod(z, p.n);
}

#else

FFU64 ffMulInverseMod(FFU64 z, FFMod p)
{
  FFI64 t=0, newt = 1;
  FFU64 r=p.n, newr = z;

  while (newr) {
    const FFU64 q = r / newr;
    const FFI64 tmp_t = t - q*newt;
    t = newt;
    newt = tmp_t;
    const FFU64 tmp_r = r - q * newr;
    r = newr;
    newr = tmp_r;
  }

  if (t<0)
    t += p.n;

  return t;
}

#endif

FFU64 ffDivMod(FFU64 num, FFU64 den, FFMod p)
{
  return ffMulMod(num, ffMulInverseMod(den, p), p);
}


// Instantiate inline functions
extern inline FFMod ffPrecomputedReciprocalMod(FFU64 n);
extern inline FFU64 ffDiv2Mod1(FFU128 z, FFMod p);
extern inline void ffDiv2By1(FFU128 z, FFMod p, FFU64 * quo, FFU64 * rem);
extern inline FFU64 ffAddMod(FFU64 a, FFU64 b, FFMod p);
extern inline FFU64 ffSubMod(FFU64 a, FFU64 b, FFMod p);
extern inline FFU64 ffMulMod(FFU64 a, FFU64 b, FFMod p);
extern inline FFU64 ffAPBCMod(FFU64 a, FFU64 b, FFU64 c, FFMod p);
extern inline FFU64 ffPrecomputedMulShoup(FFU64 w, FFMod p);
extern inline FFU64 ffMulShoup(FFU64 t, FFU64 w, FFU64 wp, FFMod p);
