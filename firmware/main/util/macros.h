#ifndef MACROS_H
#define MACROS_H

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(v, a, b) (MAX(MIN((v), (b)), (a)))

#define FEQUAL(f, v) (fabsf((f) - (v)) < 1e-6)

#define UNUSED(x) ((void)(x))
#define TODO(x) (assert(0 && "todo"))
#define UNREACHABLE() (__builtin_unreachable())

#define CONTAINER_OF(ptr, type, member) (type *)((char *)(ptr) - offsetof(type, member))

#endif
