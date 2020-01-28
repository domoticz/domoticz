#pragma once

// TODO: remove this header once gcc requirement is >= gcc-7

#if __cplusplus < 201703L
extern "C++" {
namespace std {
enum class byte : unsigned char {};
}
}
#endif
