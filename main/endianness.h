#pragma once

#if defined(__GLIBC__) || defined(__GNU_LIBRARY__) || defined(__ANDROID__)
  #include <endian.h>
#elif defined(__APPLE__) && defined(__MACH__)
  #include <machine/endian.h>
#elif defined(BSD) || defined(_SYSTYPE_BSD)
  #if defined(__OpenBSD__)
    #include <machine/endian.h>
  #else
    #include <sys/endian.h>
  #endif
#endif

#if defined(__BYTE_ORDER)
  #if defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)
    #define BIGENDIAN
  #elif defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)
    #define LITTLEENDIAN
  #endif
#elif defined(_BYTE_ORDER)
  #if defined(_BIG_ENDIAN) && (_BYTE_ORDER == _BIG_ENDIAN)
    #define BIGENDIAN
  #elif defined(_LITTLE_ENDIAN) && (_BYTE_ORDER == _LITTLE_ENDIAN)
    #define LITTLEENDIAN
  #endif
#elif defined(__BIG_ENDIAN__)
  #define BIGENDIAN
#elif defined(__LITTLE_ENDIAN__)
  #define LITTLEENDIAN
#else
  #if defined(__ARMEL__) || \
      defined(__THUMBEL__) || \
      defined(__AARCH64EL__) || \
      defined(_MIPSEL) || \
      defined(__MIPSEL) || \
      defined(__MIPSEL__) || \
      defined(__ia64__) || defined(_IA64) || \
      defined(__IA64__) || defined(__ia64) || \
      defined(_M_IA64) || defined(__itanium__) || \
      defined(i386) || defined(__i386__) || \
      defined(__i486__) || defined(__i586__) || \
      defined(__i686__) || defined(__i386) || \
      defined(_M_IX86) || defined(_X86_) || \
      defined(__THW_INTEL__) || defined(__I86__) || \
      defined(__INTEL__) || \
      defined(__x86_64) || defined(__x86_64__) || \
      defined(__amd64__) || defined(__amd64) || \
      defined(_M_X64) || \
      defined(__bfin__) || defined(__BFIN__) || \
      defined(bfin) || defined(BFIN)

      #define LITTLEENDIAN
  #elif defined(__m68k__) || defined(M68000) || \
        defined(__hppa__) || defined(__hppa) || defined(__HPPA__) || \
        defined(__sparc__) || defined(__sparc) || \
        defined(__370__) || defined(__THW_370__) || \
        defined(__s390__) || defined(__s390x__) || \
        defined(__SYSC_ZARCH__)

      #define BIGENDIAN

  #elif defined(__arm__) || defined(__arm64) || defined(__thumb__) || \
        defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB) || \
        defined(__ARM_ARCH) || \
        defined(_M_ARM) || defined(_M_ARM64)

      #if defined(_WIN32) || defined(_WIN64) || \
          defined(__WIN32__) || defined(__TOS_WIN__) || \
          defined(__WINDOWS__)

        #define LITTLEENDIAN

      #else
        #error "Cannot determine system endianness."
      #endif
  #endif
#endif


#if defined(BIGENDIAN)
  // Try to use compiler intrinsics
  #if defined(__INTEL_COMPILER) || defined(__ICC)
    #define betole16(x) _bswap16(x)
    #define betole32(x) _bswap(x)
    #define betole64(x) _bswap64(x)
  #elif defined(__GNUC__) // GCC and CLANG
    #define betole16(x) __builtin_bswap16(x)
    #define betole32(x) __builtin_bswap32(x)
    #define betole64(x) __builtin_bswap64(x)
  #elif defined(_MSC_VER) // MSVC
    #include <stdlib.h>
    #define betole16(x) _byteswap_ushort(x)
    #define betole32(x) _byteswap_ulong(x)
    #define betole64(x) _byteswap_uint64(x)
  #else
    #define FALLBACK_SWAP
    #define betole16(x) swap_u16(x)
    #define betole32(x) swap_u32(x)
    #define betole64(x) swap_u64(x)
  #endif
  #define betole128(x) swap_u128(x)
  #define betole256(x) swap_u256(x)
#else
  #define betole16(x) (x)
  #define betole32(x) (x)
  #define betole64(x) (x)
  #define betole128(x) (x)
  #define betole256(x) (x)
#endif // BIGENDIAN

#if defined(BIGENDIAN)
  #include <emmintrin.h>
  #include <smmintrin.h>
  #include <immintrin.h>
  #include <tmmintrin.h>

  inline __m128i swap_u128(__m128i value) {
    const __m128i shuffle = _mm_set_epi64x(0x0001020304050607, 0x08090a0b0c0d0e0f);
    return _mm_shuffle_epi8(value, shuffle);
  }

  inline __m256i swap_u256(__m256i value) {
    const __m256i shuffle = _mm256_set_epi64x(0x0001020304050607, 0x08090a0b0c0d0e0f, 0x0001020304050607, 0x08090a0b0c0d0e0f);
    return _mm256_shuffle_epi8(value, shuffle);
  }
#endif // BIGENDIAN

#if defined(FALLBACK_SWAP)
  #include <stdint.h>
  inline uint16_t swap_u16(uint16_t value)
  {
    return
        ((value & 0xFF00u) >>  8u) |
        ((value & 0x00FFu) <<  8u);
  }
  inline uint32_t swap_u32(uint32_t value)
  {
    return
        ((value & 0xFF000000u) >> 24u) |
        ((value & 0x00FF0000u) >>  8u) |
        ((value & 0x0000FF00u) <<  8u) |
        ((value & 0x000000FFu) << 24u);
  }
  inline uint64_t swap_u64(uint64_t value)
  {
    return
        ((value & 0xFF00000000000000u) >> 56u) |
        ((value & 0x00FF000000000000u) >> 40u) |
        ((value & 0x0000FF0000000000u) >> 24u) |
        ((value & 0x000000FF00000000u) >>  8u) |
        ((value & 0x00000000FF000000u) <<  8u) |
        ((value & 0x0000000000FF0000u) << 24u) |
        ((value & 0x000000000000FF00u) << 40u) |
        ((value & 0x00000000000000FFu) << 56u);
  }
#endif // FALLBACK_SWAP
