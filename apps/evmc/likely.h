#pragma once

#undef LIKELY
#undef UNLIKELY
#if defined(__GNUC__) && __GNUC__ >= 4
#define LIKELY(x)   (__builtin_expect(((bool)(x)), 1))
#define UNLIKELY(x) (__builtin_expect(((bool)(x)), 0))
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif
