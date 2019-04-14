#include "common.h"

int mx_voidp_eq(const void *a, const void *b) {
  return *(void **) a == b;
}

uint64_t mx_fnv1a(char *string, size_t length) {
  uint64_t hash = UINT64_C(14695981039346656037);
  for (size_t i = 0; i < length; i++) {
    hash = (hash ^ string[i]) * UINT64_C(1099511628211);
  }
  return hash;
}
