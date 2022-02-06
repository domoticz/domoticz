/*
MIT License

Copyright (c) 2018 Xavier "Crashoz" Launey
*/

#pragma once

#include <random>
#include <string>
#include <limits>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <memory>

#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>

#include "endianness.h"

namespace UUIDv4 {

/*
  Converts a 128-bits unsigned int to an UUIDv4 string representation.
  Uses SIMD via Intel's AVX2 instruction set.
 */
void inline m128itos(__m128i x, char* mem) {
  // Expand each byte in x to two bytes in res
  // i.e. 0x12345678 -> 0x0102030405060708
  // Then translate each byte to its hex ascii representation
  // i.e. 0x0102030405060708 -> 0x3132333435363738
  const __m256i mask = _mm256_set1_epi8(0x0F);
  const __m256i add = _mm256_set1_epi8(0x06);
  const __m256i alpha_mask = _mm256_set1_epi8(0x10);
  const __m256i alpha_offset = _mm256_set1_epi8(0x57);

  __m256i a = _mm256_castsi128_si256(x);
  __m256i as = _mm256_srli_epi64(a, 4);
  __m256i lo = _mm256_unpacklo_epi8(as, a);
  __m128i hi = _mm256_castsi256_si128(_mm256_unpackhi_epi8(as, a));
  __m256i c =  _mm256_inserti128_si256(lo, hi, 1);
  __m256i d = _mm256_and_si256(c, mask);
  __m256i alpha = _mm256_slli_epi64(_mm256_and_si256(_mm256_add_epi8(d, add), alpha_mask), 3);
  __m256i offset = _mm256_blendv_epi8(_mm256_slli_epi64(add, 3), alpha_offset, alpha);
  __m256i res = _mm256_add_epi8(d, offset);

  // Add dashes between blocks as specified in RFC-4122
  // 8-4-4-4-12
  const __m256i dash_shuffle = _mm256_set_epi32(0x0b0a0908, 0x07060504, 0x80030201, 0x00808080, 0x0d0c800b, 0x0a090880, 0x07060504, 0x03020100);
  const __m256i dash = _mm256_set_epi64x(0x0000000000000000ull, 0x2d000000002d0000ull, 0x00002d000000002d, 0x0000000000000000ull);

  __m256i resd = _mm256_shuffle_epi8(res, dash_shuffle);
  resd = _mm256_or_si256(resd, dash);

  _mm256_storeu_si256((__m256i*)mem, betole256(resd));
  *(uint16_t*)(mem+16) = betole16(_mm256_extract_epi16(res, 7));
  *(uint32_t*)(mem+32) = betole32(_mm256_extract_epi32(res, 7));
}

/*
  Converts an UUIDv4 string representation to a 128-bits unsigned int.
  Uses SIMD via Intel's AVX2 instruction set.
 */
__m128i inline stom128i(const char* mem) {
  // Remove dashes and pack hex ascii bytes in a 256-bits int
  const __m256i dash_shuffle = _mm256_set_epi32(0x80808080, 0x0f0e0d0c, 0x0b0a0908, 0x06050403, 0x80800f0e, 0x0c0b0a09, 0x07060504, 0x03020100);

  __m256i x = betole256(_mm256_loadu_si256((__m256i*)mem));
  x = _mm256_shuffle_epi8(x, dash_shuffle);
  x = _mm256_insert_epi16(x, betole16(*(uint16_t*)(mem+16)), 7);
  x = _mm256_insert_epi32(x, betole32(*(uint32_t*)(mem+32)), 7);

  // Build a mask to apply a different offset to alphas and digits
  const __m256i sub = _mm256_set1_epi8(0x2F);
  const __m256i mask = _mm256_set1_epi8(0x20);
  const __m256i alpha_offset = _mm256_set1_epi8(0x28);
  const __m256i digits_offset = _mm256_set1_epi8(0x01);
  const __m256i unweave = _mm256_set_epi32(0x0f0d0b09, 0x0e0c0a08, 0x07050301, 0x06040200, 0x0f0d0b09, 0x0e0c0a08, 0x07050301, 0x06040200);
  const __m256i shift = _mm256_set_epi32(0x00000000, 0x00000004, 0x00000000, 0x00000004, 0x00000000, 0x00000004, 0x00000000, 0x00000004);

  // Translate ascii bytes to their value
  // i.e. 0x3132333435363738 -> 0x0102030405060708
  // Shift hi-digits
  // i.e. 0x0102030405060708 -> 0x1002300450067008
  // Horizontal add
  // i.e. 0x1002300450067008 -> 0x12345678
  __m256i a = _mm256_sub_epi8(x, sub);
  __m256i alpha = _mm256_slli_epi64(_mm256_and_si256(a, mask), 2);
  __m256i sub_mask = _mm256_blendv_epi8(digits_offset, alpha_offset, alpha);
  a = _mm256_sub_epi8(a, sub_mask);
  a = _mm256_shuffle_epi8(a, unweave);
  a = _mm256_sllv_epi32(a, shift);
  a = _mm256_hadd_epi32(a, _mm256_setzero_si256());
  a = _mm256_permute4x64_epi64(a, 0b00001000);

  return _mm256_castsi256_si128(a);
}

/*
 * UUIDv4 (random 128-bits) RFC-4122
 */
class UUID {
  public:
    UUID()
    {}

    UUID(const UUID &other) {
      __m128i x = _mm_load_si128((__m128i*)other.data);
      _mm_store_si128((__m128i*)data, x);
    }

    /* Builds a 128-bits UUID */
    UUID(__m128i uuid) {
      _mm_store_si128((__m128i*)data, uuid);
    }

    UUID(uint64_t x, uint64_t y) {
      __m128i z = _mm_set_epi64x(x, y);
      _mm_store_si128((__m128i*)data, z);
    }

    UUID(const uint8_t* bytes) {
      __m128i x = _mm_loadu_si128((__m128i*)bytes);
      _mm_store_si128((__m128i*)data, x);
    }

    /* Builds an UUID from a byte string (16 bytes long) */
    explicit UUID(const std::string &bytes) {
      __m128i x = betole128(_mm_loadu_si128((__m128i*)bytes.data()));
      _mm_store_si128((__m128i*)data, x);
    }

    /* Static factory to parse an UUID from its string representation */
    static UUID fromStrFactory(const std::string &s) {
      return fromStrFactory(s.c_str());
    }

    static UUID fromStrFactory(const char* raw) {
      return UUID(stom128i(raw));
    }

    void fromStr(const char* raw) {
      _mm_store_si128((__m128i*)data, stom128i(raw));
    }

    UUID& operator=(const UUID &other) {
      if (&other == this) {
        return *this;
      }
      __m128i x = _mm_load_si128((__m128i*)other.data);
      _mm_store_si128((__m128i*)data, x);
      return *this;
    }

    friend bool operator==(const UUID &lhs, const UUID &rhs) {
      __m128i x = _mm_load_si128((__m128i*)lhs.data);
      __m128i y = _mm_load_si128((__m128i*)rhs.data);

      __m128i neq = _mm_xor_si128(x, y);
      return _mm_test_all_zeros(neq, neq);
    }

    friend bool operator<(const UUID &lhs, const UUID &rhs) {
      // There are no trivial 128-bits comparisons in SSE/AVX
      // It's faster to compare two uint64_t
      uint64_t *x = (uint64_t*)lhs.data;
      uint64_t *y = (uint64_t*)rhs.data;
      return *x < *y || (*x == *y && *(x + 1) < *(y + 1));
    }

    friend bool operator!=(const UUID &lhs, const UUID &rhs) { return !(lhs == rhs); }
    friend bool operator> (const UUID &lhs, const UUID &rhs) { return rhs < lhs; }
    friend bool operator<=(const UUID &lhs, const UUID &rhs) { return !(lhs > rhs); }
    friend bool operator>=(const UUID &lhs, const UUID &rhs) { return !(lhs < rhs); }

    /* Serializes the uuid to a byte string (16 bytes) */
    std::string bytes() const {
      std::string mem;
      bytes(mem);
      return mem;
    }

    void bytes(std::string &out) const {
      out.resize(sizeof(data));
      bytes((char*)out.data());
    }

    void bytes(char* bytes) const {
      __m128i x = betole128(_mm_load_si128((__m128i*)data));
      _mm_storeu_si128((__m128i*)bytes, x);
    }

    /* Converts the uuid to its string representation */
    std::string str() const {
      std::string mem;
      str(mem);
      return mem;
    }

    void str(std::string &s) const {
      s.resize(36);
      str((char*)s.data());
    }

    void str(char *res) const {
      __m128i x = _mm_load_si128((__m128i*)data);
      m128itos(x, res);
    }

    friend std::ostream& operator<< (std::ostream& stream, const UUID& uuid) {
      return stream << uuid.str();
    }

    friend std::istream& operator>> (std::istream& stream, UUID& uuid) {
      std::string s;
      stream >> s;
      uuid = fromStrFactory(s);
      return stream;
    }

    size_t hash() const {
        return static_cast<size_t>(*((uint64_t*)data) ^ *((uint64_t*)data + 8));
    }

  private:
    alignas(128) uint8_t data[16];
};

/*
  Generates UUIDv4 from a provided random generator (c++11 <random> module)
  std::mt19937_64 is highly recommended as it has a SIMD implementation that
  makes it very fast and it produces high quality randomness.
 */
template <typename RNG>
class UUIDGenerator {
  public:
    UUIDGenerator() : generator(new RNG(std::random_device()())), distribution(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max())
    {}

    UUIDGenerator(uint64_t seed) : generator(new RNG(seed)), distribution(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max())
    {}

    UUIDGenerator(RNG &gen) : generator(gen), distribution(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max())
    {}

    /* Generates a new UUID */
    UUID getUUID() {
      // The two masks set the uuid version (4) and variant (1)
      const __m128i and_mask = _mm_set_epi64x(0xFFFFFFFFFFFFFF3Full, 0xFF0FFFFFFFFFFFFFull);
      const __m128i or_mask =  _mm_set_epi64x(0x0000000000000080ull, 0x0040000000000000ull);
      __m128i n = _mm_set_epi64x(distribution(*generator), distribution(*generator));
      __m128i uuid = _mm_or_si128(_mm_and_si128(n, and_mask), or_mask);

      return UUID(uuid);
    }

  private:
    std::shared_ptr<RNG> generator;
    std::uniform_int_distribution<uint64_t> distribution;
};

}

namespace std {
  template <> struct hash<UUIDv4::UUID>
  {
    size_t operator()(const UUIDv4::UUID & uuid) const
    {
      return uuid.hash();
    }
  };
}
