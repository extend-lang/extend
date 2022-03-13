/* extend - expansible programming language
 * Copyright (C) 2022 Vladimir Liutov vs@lutov.net
 * Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.
 * Source: https://github.com/Tencent/rapidjson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/** This is a C++ header-only implementation of Grisu2 algorithm from the
 * publication: Loitsch, Florian. "Printing floating-point numbers quickly and
 * accurately with integers." ACM Sigplan Notices 45.6 (2010): 233-243.
 */

#include <cassert>
#include <cstdint>
#include <cstring> // memcpy
#include <limits>

namespace extend::log {

namespace {
static const char8_t cDigitsLut[200] = {
  u8'0', u8'0', u8'0', u8'1', u8'0', u8'2', u8'0', u8'3', u8'0', u8'4', u8'0',
  u8'5', u8'0', u8'6', u8'0', u8'7', u8'0', u8'8', u8'0', u8'9', u8'1', u8'0',
  u8'1', u8'1', u8'1', u8'2', u8'1', u8'3', u8'1', u8'4', u8'1', u8'5', u8'1',
  u8'6', u8'1', u8'7', u8'1', u8'8', u8'1', u8'9', u8'2', u8'0', u8'2', u8'1',
  u8'2', u8'2', u8'2', u8'3', u8'2', u8'4', u8'2', u8'5', u8'2', u8'6', u8'2',
  u8'7', u8'2', u8'8', u8'2', u8'9', u8'3', u8'0', u8'3', u8'1', u8'3', u8'2',
  u8'3', u8'3', u8'3', u8'4', u8'3', u8'5', u8'3', u8'6', u8'3', u8'7', u8'3',
  u8'8', u8'3', u8'9', u8'4', u8'0', u8'4', u8'1', u8'4', u8'2', u8'4', u8'3',
  u8'4', u8'4', u8'4', u8'5', u8'4', u8'6', u8'4', u8'7', u8'4', u8'8', u8'4',
  u8'9', u8'5', u8'0', u8'5', u8'1', u8'5', u8'2', u8'5', u8'3', u8'5', u8'4',
  u8'5', u8'5', u8'5', u8'6', u8'5', u8'7', u8'5', u8'8', u8'5', u8'9', u8'6',
  u8'0', u8'6', u8'1', u8'6', u8'2', u8'6', u8'3', u8'6', u8'4', u8'6', u8'5',
  u8'6', u8'6', u8'6', u8'7', u8'6', u8'8', u8'6', u8'9', u8'7', u8'0', u8'7',
  u8'1', u8'7', u8'2', u8'7', u8'3', u8'7', u8'4', u8'7', u8'5', u8'7', u8'6',
  u8'7', u8'7', u8'7', u8'8', u8'7', u8'9', u8'8', u8'0', u8'8', u8'1', u8'8',
  u8'2', u8'8', u8'3', u8'8', u8'4', u8'8', u8'5', u8'8', u8'6', u8'8', u8'7',
  u8'8', u8'8', u8'8', u8'9', u8'9', u8'0', u8'9', u8'1', u8'9', u8'2', u8'9',
  u8'3', u8'9', u8'4', u8'9', u8'5', u8'9', u8'6', u8'9', u8'7', u8'9', u8'8',
  u8'9', u8'9'
};

struct DiyFp
{
  DiyFp()
    : f()
    , e()
  {}

  DiyFp(uint64_t fp, int exp)
    : f(fp)
    , e(exp)
  {}

  explicit DiyFp(double d)
  {
    union
    {
      double d;
      uint64_t u64;
    } u = { d };

    int biased_e =
      static_cast<int>((u.u64 & kDpExponentMask) >> kDpSignificandSize);
    uint64_t significand = (u.u64 & kDpSignificandMask);
    if (biased_e != 0) {
      f = significand + kDpHiddenBit;
      e = biased_e - kDpExponentBias;
    } else {
      f = significand;
      e = kDpMinExponent + 1;
    }
  }

  DiyFp operator-(const DiyFp& rhs) const { return DiyFp(f - rhs.f, e); }

  DiyFp operator*(const DiyFp& rhs) const
  {
#if defined(_MSC_VER) && defined(_M_AMD64)
    uint64_t h;
    uint64_t l = _umul128(f, rhs.f, &h);
    if (l & (uint64_t(1) << 63)) // rounding
      h++;
    return DiyFp(h, e + rhs.e + 64);
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) &&              \
  defined(__x86_64__)
    __extension__ typedef unsigned __int128 uint128;
    uint128 p = static_cast<uint128>(f) * static_cast<uint128>(rhs.f);
    uint64_t h = static_cast<uint64_t>(p >> 64);
    uint64_t l = static_cast<uint64_t>(p);
    if (l & (uint64_t(1) << 63)) // rounding
      h++;
    return DiyFp(h, e + rhs.e + 64);
#else
    const uint64_t M32 = 0xFFFFFFFF;
    const uint64_t a = f >> 32;
    const uint64_t b = f & M32;
    const uint64_t c = rhs.f >> 32;
    const uint64_t d = rhs.f & M32;
    const uint64_t ac = a * c;
    const uint64_t bc = b * c;
    const uint64_t ad = a * d;
    const uint64_t bd = b * d;
    uint64_t tmp = (bd >> 32) + (ad & M32) + (bc & M32);
    tmp += 1U << 31; /// mult_round
    return DiyFp(ac + (ad >> 32) + (bc >> 32) + (tmp >> 32), e + rhs.e + 64);
#endif
  }

  DiyFp Normalize() const
  {
    int s = static_cast<int>(__builtin_clzll(f));
    return DiyFp(f << s, e - s);
  }

  DiyFp NormalizeBoundary() const
  {
    DiyFp res = *this;
    while (!(res.f & (kDpHiddenBit << 1))) {
      res.f <<= 1;
      res.e--;
    }
    res.f <<= (kDiySignificandSize - kDpSignificandSize - 2);
    res.e = res.e - (kDiySignificandSize - kDpSignificandSize - 2);
    return res;
  }

  void NormalizedBoundaries(DiyFp* minus, DiyFp* plus) const
  {
    DiyFp pl = DiyFp((f << 1) + 1, e - 1).NormalizeBoundary();
    DiyFp mi = (f == kDpHiddenBit) ? DiyFp((f << 2) - 1, e - 2)
                                   : DiyFp((f << 1) - 1, e - 1);
    mi.f <<= mi.e - pl.e;
    mi.e = pl.e;
    *plus = pl;
    *minus = mi;
  }

  double ToDouble() const
  {
    union
    {
      double d;
      uint64_t u64;
    } u;
    assert(f <= kDpHiddenBit + kDpSignificandMask);
    if (e < kDpDenormalExponent) {
      // Underflow.
      return 0.0;
    }
    if (e >= kDpMaxExponent) {
      // Overflow.
      return std::numeric_limits<double>::infinity();
    }
    const uint64_t be = (e == kDpDenormalExponent && (f & kDpHiddenBit) == 0)
                          ? 0
                          : static_cast<uint64_t>(e + kDpExponentBias);
    u.u64 = (f & kDpSignificandMask) | (be << kDpSignificandSize);
    return u.d;
  }

  static const int kDiySignificandSize = 64;
  static const int kDpSignificandSize = 52;
  static const int kDpExponentBias = 0x3FF + kDpSignificandSize;
  static const int kDpMaxExponent = 0x7FF - kDpExponentBias;
  static const int kDpMinExponent = -kDpExponentBias;
  static const int kDpDenormalExponent = -kDpExponentBias + 1;
  static const uint64_t kDpExponentMask = 0x7FF0000000000000ULL;
  static const uint64_t kDpSignificandMask = 0x000FFFFFFFFFFFFFULL;
  static const uint64_t kDpHiddenBit = 0x0010000000000000ULL;

  uint64_t f;
  int e;
};

struct Double
{
public:
  Double() {}
  Double(double d)
    : d_(d)
  {}
  Double(uint64_t u)
    : u_(u)
  {}

  double Value() const { return d_; }
  uint64_t Uint64Value() const { return u_; }

  double NextPositiveDouble() const
  {
    assert(!Sign());
    return Double(u_ + 1).Value();
  }

  bool Sign() const { return (u_ & kSignMask) != 0; }
  uint64_t Significand() const { return u_ & kSignificandMask; }
  int Exponent() const
  {
    return static_cast<int>(((u_ & kExponentMask) >> kSignificandSize) -
                            kExponentBias);
  }

  bool IsNan() const
  {
    return (u_ & kExponentMask) == kExponentMask && Significand() != 0;
  }
  bool IsInf() const
  {
    return (u_ & kExponentMask) == kExponentMask && Significand() == 0;
  }
  bool IsNanOrInf() const { return (u_ & kExponentMask) == kExponentMask; }
  bool IsNormal() const
  {
    return (u_ & kExponentMask) != 0 || Significand() == 0;
  }
  bool IsZero() const { return (u_ & (kExponentMask | kSignificandMask)) == 0; }

  uint64_t IntegerSignificand() const
  {
    return IsNormal() ? Significand() | kHiddenBit : Significand();
  }
  int IntegerExponent() const
  {
    return (IsNormal() ? Exponent() : kDenormalExponent) - kSignificandSize;
  }
  uint64_t ToBias() const
  {
    return (u_ & kSignMask) ? ~u_ + 1 : u_ | kSignMask;
  }

  static int EffectiveSignificandSize(int order)
  {
    if (order >= -1021)
      return 53;
    else if (order <= -1074)
      return 0;
    else
      return order + 1074;
  }

private:
  static const int kSignificandSize = 52;
  static const int kExponentBias = 0x3FF;
  static const int kDenormalExponent = 1 - kExponentBias;
  static const uint64_t kSignMask = 0x8000000000000000ULL;
  static const uint64_t kExponentMask = 0x7FF0000000000000ULL;
  static const uint64_t kSignificandMask = 0x000FFFFFFFFFFFFFULL;
  static const uint64_t kHiddenBit = 0x0010000000000000ULL;

  union
  {
    double d_;
    uint64_t u_;
  };
};

static inline DiyFp
GetCachedPowerByIndex(size_t index)
{
  // 10^-348, 10^-340, ..., 10^340
  static const uint64_t kCachedPowers_F[] = {
    0xfa8fd5a0081c0288ULL, 0xbaaee17fa23ebf76ULL, 0x8b16fb203055ac76ULL,
    0xcf42894a5dce35eaULL, 0x9a6bb0aa55653b2dULL, 0xe61acf033d1a45dfULL,
    0xab70fe17c79ac6caULL, 0xff77b1fcbebcdc4fULL, 0xbe5691ef416bd60cULL,
    0x8dd01fad907ffc3cULL, 0xd3515c2831559a83ULL, 0x9d71ac8fada6c9b5ULL,
    0xea9c227723ee8bcbULL, 0xaecc49914078536dULL, 0x823c12795db6ce57ULL,
    0xc21094364dfb5637ULL, 0x9096ea6f3848984fULL, 0xd77485cb25823ac7ULL,
    0xa086cfcd97bf97f4ULL, 0xef340a98172aace5ULL, 0xb23867fb2a35b28eULL,
    0x84c8d4dfd2c63f3bULL, 0xc5dd44271ad3cdbaULL, 0x936b9fcebb25c996ULL,
    0xdbac6c247d62a584ULL, 0xa3ab66580d5fdaf6ULL, 0xf3e2f893dec3f126ULL,
    0xb5b5ada8aaff80b8ULL, 0x87625f056c7c4a8bULL, 0xc9bcff6034c13053ULL,
    0x964e858c91ba2655ULL, 0xdff9772470297ebdULL, 0xa6dfbd9fb8e5b88fULL,
    0xf8a95fcf88747d94ULL, 0xb94470938fa89bcfULL, 0x8a08f0f8bf0f156bULL,
    0xcdb02555653131b6ULL, 0x993fe2c6d07b7facULL, 0xe45c10c42a2b3b06ULL,
    0xaa242499697392d3ULL, 0xfd87b5f28300ca0eULL, 0xbce5086492111aebULL,
    0x8cbccc096f5088ccULL, 0xd1b71758e219652cULL, 0x9c40000000000000ULL,
    0xe8d4a51000000000ULL, 0xad78ebc5ac620000ULL, 0x813f3978f8940984ULL,
    0xc097ce7bc90715b3ULL, 0x8f7e32ce7bea5c70ULL, 0xd5d238a4abe98068ULL,
    0x9f4f2726179a2245ULL, 0xed63a231d4c4fb27ULL, 0xb0de65388cc8ada8ULL,
    0x83c7088e1aab65dbULL, 0xc45d1df942711d9aULL, 0x924d692ca61be758ULL,
    0xda01ee641a708deaULL, 0xa26da3999aef774aULL, 0xf209787bb47d6b85ULL,
    0xb454e4a179dd1877ULL, 0x865b86925b9bc5c2ULL, 0xc83553c5c8965d3dULL,
    0x952ab45cfa97a0b3ULL, 0xde469fbd99a05fe3ULL, 0xa59bc234db398c25ULL,
    0xf6c69a72a3989f5cULL, 0xb7dcbf5354e9beceULL, 0x88fcf317f22241e2ULL,
    0xcc20ce9bd35c78a5ULL, 0x98165af37b2153dfULL, 0xe2a0b5dc971f303aULL,
    0xa8d9d1535ce3b396ULL, 0xfb9b7cd9a4a7443cULL, 0xbb764c4ca7a44410ULL,
    0x8bab8eefb6409c1aULL, 0xd01fef10a657842cULL, 0x9b10a4e5e9913129ULL,
    0xe7109bfba19c0c9dULL, 0xac2820d9623bf429ULL, 0x80444b5e7aa7cf85ULL,
    0xbf21e44003acdd2dULL, 0x8e679c2f5e44ff8fULL, 0xd433179d9c8cb841ULL,
    0x9e19db92b4e31ba9ULL, 0xeb96bf6ebadf77d9ULL, 0xaf87023b9bf0ee6bULL
  };
  static const int16_t kCachedPowers_E[] = {
    -1220, -1193, -1166, -1140, -1113, -1087, -1060, -1034, -1007, -980, -954,
    -927,  -901,  -874,  -847,  -821,  -794,  -768,  -741,  -715,  -688, -661,
    -635,  -608,  -582,  -555,  -529,  -502,  -475,  -449,  -422,  -396, -369,
    -343,  -316,  -289,  -263,  -236,  -210,  -183,  -157,  -130,  -103, -77,
    -50,   -24,   3,     30,    56,    83,    109,   136,   162,   189,  216,
    242,   269,   295,   322,   348,   375,   402,   428,   455,   481,  508,
    534,   561,   588,   614,   641,   667,   694,   720,   747,   774,  800,
    827,   853,   880,   907,   933,   960,   986,   1013,  1039,  1066
  };
  assert(index < 87);
  return DiyFp(kCachedPowers_F[index], kCachedPowers_E[index]);
}

static inline DiyFp
GetCachedPower(int e, int* K)
{

  // int k = static_cast<int>(ceil((-61 - e) * 0.30102999566398114)) + 374;
  double dk = (-61 - e) * 0.30102999566398114 +
              347; // dk must be positive, so can do ceiling in positive
  int k = static_cast<int>(dk);
  if (dk - k > 0.0)
    k++;

  unsigned index = static_cast<unsigned>((k >> 3) + 1);
  *K = -(-348 +
         static_cast<int>(index << 3)); // decimal exponent no need lookup table

  return GetCachedPowerByIndex(index);
}

static inline void
GrisuRound(char8_t* buffer,
           int len,
           uint64_t delta,
           uint64_t rest,
           uint64_t ten_kappa,
           uint64_t wp_w)
{
  while (rest < wp_w && delta - rest >= ten_kappa &&
         (rest + ten_kappa < wp_w || /// closer
          wp_w - rest > rest + ten_kappa - wp_w)) {
    buffer[len - 1]--;
    rest += ten_kappa;
  }
}

static inline int
CountDecimalDigit32(uint32_t n)
{
  // Simple pure C++ implementation was faster than __builtin_clz version in
  // this situation.
  if (n < 10)
    return 1;
  if (n < 100)
    return 2;
  if (n < 1000)
    return 3;
  if (n < 10000)
    return 4;
  if (n < 100000)
    return 5;
  if (n < 1000000)
    return 6;
  if (n < 10000000)
    return 7;
  if (n < 100000000)
    return 8;
  // Will not reach 10 digits in DigitGen()
  // if (n < 1000000000) return 9;
  // return 10;
  return 9;
}

static inline void
DigitGen(const DiyFp& W,
         const DiyFp& Mp,
         uint64_t delta,
         char8_t* buffer,
         int* len,
         int* K)
{
  static const uint64_t kPow10[] = { 1U,
                                     10U,
                                     100U,
                                     1000U,
                                     10000U,
                                     100000U,
                                     1000000U,
                                     10000000U,
                                     100000000U,
                                     1000000000U,
                                     10000000000U,
                                     100000000000U,
                                     1000000000000U,
                                     10000000000000U,
                                     100000000000000U,
                                     1000000000000000U,
                                     10000000000000000U,
                                     100000000000000000U,
                                     1000000000000000000U,
                                     10000000000000000000U };
  const DiyFp one(uint64_t(1) << -Mp.e, Mp.e);
  const DiyFp wp_w = Mp - W;
  uint32_t p1 = static_cast<uint32_t>(Mp.f >> -one.e);
  uint64_t p2 = Mp.f & (one.f - 1);
  int kappa = CountDecimalDigit32(p1); // kappa in [0, 9]
  *len = 0;

  while (kappa > 0) {
    uint32_t d = 0;
    switch (kappa) {
      case 9:
        d = p1 / 100000000;
        p1 %= 100000000;
        break;
      case 8:
        d = p1 / 10000000;
        p1 %= 10000000;
        break;
      case 7:
        d = p1 / 1000000;
        p1 %= 1000000;
        break;
      case 6:
        d = p1 / 100000;
        p1 %= 100000;
        break;
      case 5:
        d = p1 / 10000;
        p1 %= 10000;
        break;
      case 4:
        d = p1 / 1000;
        p1 %= 1000;
        break;
      case 3:
        d = p1 / 100;
        p1 %= 100;
        break;
      case 2:
        d = p1 / 10;
        p1 %= 10;
        break;
      case 1:
        d = p1;
        p1 = 0;
        break;
      default:;
    }
    if (d || *len)
      buffer[(*len)++] = u8'0' + d;
    kappa--;
    uint64_t tmp = (static_cast<uint64_t>(p1) << -one.e) + p2;
    if (tmp <= delta) {
      *K += kappa;
      GrisuRound(buffer, *len, delta, tmp, kPow10[kappa] << -one.e, wp_w.f);
      return;
    }
  }

  // kappa = 0
  for (;;) {
    p2 *= 10;
    delta *= 10;
    char8_t d = p2 >> -one.e;
    if (d || *len)
      buffer[(*len)++] = u8'0' + d;
    p2 &= one.f - 1;
    kappa--;
    if (p2 < delta) {
      *K += kappa;
      int index = -kappa;
      GrisuRound(buffer,
                 *len,
                 delta,
                 p2,
                 one.f,
                 wp_w.f * (index < 20 ? kPow10[index] : 0));
      return;
    }
  }
}

static inline void
Grisu2(double x, char8_t* buffer, int* length, int* K)
{
  const DiyFp v(x);
  DiyFp w_m, w_p;
  v.NormalizedBoundaries(&w_m, &w_p);

  const DiyFp c_mk = GetCachedPower(w_p.e, K);
  const DiyFp W = v.Normalize() * c_mk;
  DiyFp Wp = w_p * c_mk;
  DiyFp Wm = w_m * c_mk;
  Wm.f++;
  Wp.f--;
  DigitGen(W, Wp, Wp.f - Wm.f, buffer, length, K);
}

}

int16_t
dtoa_prettify(char8_t* buffer, int16_t length, int16_t exponent)
{
  const int16_t kk = length + exponent; // 10^(kk-1) <= v < 10^kk

  if (exponent < 0) {
    if (-3 <= kk && kk <= 0) {
      // 1234e-6 -> 0.001234
      const int16_t offset = 2 - kk;
      memcpy(&buffer[offset], &buffer[0], length);
      buffer[0] = u8'0';
      buffer[1] = u8'.';
      for (int16_t i = 2; i < offset; i++)
        buffer[i] = u8'0';
      return length + offset;
    }

    if (0 < kk && kk <= 21) {
      // 1234e-2 -> 12.34
      memcpy(&buffer[kk + 1], &buffer[kk], length - kk);
      buffer[kk] = u8'.';
      return length + 1;
    }
  } else if (exponent <= 3) {
    // 1234e3 -> 1234000
    for (int16_t i = length; i < kk; i++)
      buffer[i] = u8'0';
    buffer[kk] = u8'.';
    buffer[kk + 1] = u8'0';
    return kk + 2;
  }

  if (length == 1) {
    // 1.0e+30
    constexpr char8_t ZERO_SUFFIX[] = u8".0e";
    memcpy(buffer + 1, ZERO_SUFFIX, sizeof(ZERO_SUFFIX) - 1);
    length += sizeof(ZERO_SUFFIX) - 1;
  } else {
    // 1234e30 -> 1.234e33
    memcpy(&buffer[2], &buffer[1], length - 1);
    buffer[1] = u8'.';
    buffer[length + 1] = u8'e';
    length += 2;
  }

  // Write exponent
  int16_t exp = kk - 1;
  if (exp < 0) {
    buffer[length++] = u8'-';
    exp = -exp;
  } else {
    buffer[length++] = u8'+';
  }

  if (exp >= 100) {
    buffer[length++] = u8'0' + static_cast<char8_t>(exp / 100);
    exp %= 100;
    memcpy(buffer + length, cDigitsLut + exp * 2, 2);
    return length + 2;
  } else if (exp >= 10) {
    memcpy(buffer + length, cDigitsLut + exp * 2, 2);
    return length + 2;
  } else {
    buffer[length++] = u8'0' + static_cast<char8_t>(exp);
    return length;
  }
}

int
dtoa(double x, char8_t* buffer)
{
  Double d(x);
  if (d.IsZero()) {
    bool s = d.Sign();
    if (s)
      *buffer++ = u8'-';
    constexpr char8_t ZERO_STR[] = u8"0.0";
    memcpy(buffer, ZERO_STR, sizeof(ZERO_STR) - 1);
    return sizeof(ZERO_STR) - 1 + s;
  }

  if (d.IsNan()) {
    constexpr char8_t NAN_STR[] = u8"nan";
    memcpy(buffer, NAN_STR, sizeof(NAN_STR) - 1);
    return sizeof(NAN_STR) - 1;
  }

  if (d.IsInf()) {
    bool s = d.Sign();
    if (s)
      *buffer++ = u8'-';
    constexpr char8_t INF_STR[] = u8"Infinity";
    memcpy(buffer, INF_STR, sizeof(INF_STR) - 1);
    return sizeof(INF_STR) - 1 + s;
  }

  if (x < 0) {
    *buffer++ = u8'-';
    x = -x;
  }
  int length, K;
  Grisu2(x, buffer, &length, &K);
  return dtoa_prettify(buffer, length, K);
}

} // namespace log
