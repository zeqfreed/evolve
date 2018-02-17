#pragma once

#ifdef __cplusplus
  #define C_LINKAGE extern "C"
#else
  #define C_LINKAGE extern
#endif

#ifdef DEBUG
  #define ASSERT(x) if (!(x)) { printf("Assertion at %s, line %d failed: %s\n", __FILE__, __LINE__, #x); *((uint32_t *)1) = 0xDEADCAFE; }
#else
  #define ASSERT(...)
#endif

#ifdef _MSC_VER
  #define EXPORT __declspec(dllexport)
#else
  #define EXPORT
#endif

#ifdef _MSC_VER
  #define PACK_START(n) __pragma(pack(push, n))
  #define PACK_END(...) __pragma(pack(pop))
  #define PACKED
#else
  #define PACK_START(...)
  #define PACK_END(...)
  #define PACKED __attribute__((packed))
#endif
