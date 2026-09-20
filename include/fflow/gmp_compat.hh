#ifndef FFLOW_GMP_COMPAT_HH
#define FFLOW_GMP_COMPAT_HH

#include <cstdint>
#include <limits>
#include <gmp.h>

namespace fflow {
  namespace detail {

    // GMP's _ui/_si interfaces use long, which is only 32 bits on LLP64.
    // Use public GMP APIs, independently of FLINT and GMP's limb layout.
    inline void mpz_set_u64(mpz_ptr out, std::uint64_t value)
    {
      if (sizeof(unsigned long) >= sizeof(value))
        mpz_set_ui(out, static_cast<unsigned long>(value));
      else
        mpz_import(out, 1, 1, sizeof(value), 0, 0, &value);
    }

    inline void mpz_init_set_u64(mpz_ptr out, std::uint64_t value)
    {
      mpz_init(out);
      mpz_set_u64(out, value);
    }

    inline void mpz_set_i64(mpz_ptr out, std::int64_t value)
    {
      const std::uint64_t magnitude = value < 0
        ? std::uint64_t(0) - static_cast<std::uint64_t>(value)
        : static_cast<std::uint64_t>(value);
      mpz_set_u64(out, magnitude);
      if (value < 0)
        mpz_neg(out, out);
    }

    // Match unsigned GMP/FLINT extraction: low 64 bits of the magnitude.
    inline std::uint64_t mpz_get_u64(mpz_srcptr value)
    {
      if (sizeof(unsigned long) >= sizeof(std::uint64_t))
        return static_cast<std::uint64_t>(mpz_get_ui(value));
      std::uint64_t result = 0;
      if (mpz_sizeinbase(value, 2) <= 64) {
        mpz_export(&result, nullptr, 1, sizeof(result), 0, 0, value);
      } else {
        // Bound the export to one word; mpz_export itself does not truncate.
        mpz_t low;
        mpz_init(low);
        mpz_tdiv_r_2exp(low, value, 64);
        mpz_export(&result, nullptr, 1, sizeof(result), 0, 0, low);
        mpz_clear(low);
      }
      return result;
    }

    // Retain native GMP extraction on LP64. On LLP64, preserve the Windows
    // FLINT signed low-word behavior without signed overflow or an
    // out-of-range unsigned-to-signed conversion.
    inline std::int64_t mpz_get_i64(mpz_srcptr value)
    {
      if (sizeof(long) >= sizeof(std::int64_t))
        return static_cast<std::int64_t>(mpz_get_si(value));
      std::uint64_t bits = mpz_get_u64(value);
      if (mpz_sgn(value) < 0)
        bits = std::uint64_t(0) - bits;
      if (bits <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return static_cast<std::int64_t>(bits);
      return -1 - static_cast<std::int64_t>(~bits);
    }

    inline int mpz_cmp_u64(mpz_srcptr value, std::uint64_t other)
    {
      if (sizeof(unsigned long) >= sizeof(other))
        return mpz_cmp_ui(value, static_cast<unsigned long>(other));
      mpz_t temp;
      mpz_init_set_u64(temp, other);
      const int result = mpz_cmp(value, temp);
      mpz_clear(temp);
      return result;
    }

    inline void mpz_mul_u64(mpz_ptr out, mpz_srcptr value, std::uint64_t factor)
    {
      if (sizeof(unsigned long) >= sizeof(factor)) {
        mpz_mul_ui(out, value, static_cast<unsigned long>(factor));
        return;
      }
      mpz_t temp;
      mpz_init_set_u64(temp, factor);
      mpz_mul(out, value, temp);
      mpz_clear(temp);
    }

    // divisor must be nonzero, as with mpz_fdiv_ui.
    inline std::uint64_t mpz_fdiv_u64(mpz_srcptr value, std::uint64_t divisor)
    {
      if (sizeof(unsigned long) >= sizeof(divisor))
        return mpz_fdiv_ui(value, static_cast<unsigned long>(divisor));
      mpz_t temp;
      mpz_init_set_u64(temp, divisor);
      mpz_mod(temp, value, temp);
      const std::uint64_t result = mpz_get_u64(temp);
      mpz_clear(temp);
      return result;
    }

    inline void mpz_u64_sub(mpz_ptr out, std::uint64_t value, mpz_srcptr other)
    {
      if (sizeof(unsigned long) >= sizeof(value)) {
        mpz_ui_sub(out, static_cast<unsigned long>(value), other);
        return;
      }
      mpz_t temp;
      mpz_init_set_u64(temp, value);
      mpz_sub(out, temp, other);
      mpz_clear(temp);
    }

  } // namespace detail
} // namespace fflow

#endif // FFLOW_GMP_COMPAT_HH
