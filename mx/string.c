#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "string.h"

typedef struct _header_t {
  size_t volume;
  size_t length;
  char data[];
} header_t;

mx_string_t header_to_string(header_t *header) {
  return (mx_string_t) header->data;
}

header_t *string_to_header(mx_string_t string) {
  return (header_t *) (string - sizeof(header_t));
}

mx_string_t mx_string_create(char *source, size_t length) {
  header_t *header;
  size_t size;

  if (source != NULL && length == 0)
    length = strlen(source);

  if (mx_addz_overflow(length, sizeof(header_t) + 1, &size))
    return NULL;

  if ((header = malloc(size)) == NULL)
    return NULL;

  header->volume = length;
  header->length = length;

  if (source != NULL)
    memcpy(header->data, source, length);
  else
    memset(header->data, 0, length);

  header->data[length] = '\0';

  return header_to_string(header);
}

mx_string_t mx_string_duplicate(mx_string_t source) {
  header_t *header;

  size_t volume = mx_string_volume(source);
  size_t length = mx_string_length(source);

  if ((header = malloc(sizeof(header_t) + 1 + volume)) == NULL) {
    if (length == volume)
      return NULL;
    if ((header = malloc(sizeof(header_t) + 1 + length)) == NULL)
      return NULL;
    header->volume = length;
  } else
    header->volume = volume;

  header->length = length;
  memcpy(header->data, source, length);

  header->data[length] = '\0';

  return header_to_string(header);
}

void mx_string_delete(mx_string_t string) {
  free(string_to_header(string));
}

size_t mx_string_volume(mx_string_t string) {
  return string_to_header(string)->volume;
}

size_t mx_string_length(mx_string_t string) {
  return string_to_header(string)->length;
}

mx_string_t mx_string_resize(mx_string_t string, size_t volume) {
  header_t *header = string_to_header(string);
  size_t size;

  // calculate size and check for overflow
  if (mx_addz_overflow(volume, sizeof(header_t) + 1, &size))
    return NULL;

  if ((header = realloc(header, size)) == NULL)
    return NULL;
  header->volume = volume;

  // set length and null terminate on truncation
  if (header->length > volume)
    header->data[header->length = volume] = '\0';

  return header_to_string(header);
}

mx_string_t mx_string_shrink(mx_string_t string) {
  mx_string_t shrunk;

  shrunk = mx_string_resize(string, mx_string_length(string));

  return shrunk != NULL ? shrunk : string;
}

mx_string_t mx_string_ensure(mx_string_t string, size_t length) {
  if (length > mx_string_volume(string)) {
    // volume = (length * 8 + 3) / 5 avoiding intermediate overflow
    size_t volume = length / 5 * 8 + ((length % 5) * 8 + 3) / 5;

    // if the volume doesn't overflow then attempt to allocate it
    if (volume > length) {
      mx_string_t resize;
      if ((resize = mx_string_resize(string, volume)) != NULL)
        return resize;
    }

    // if either the volume overflows or the allocation failed then attempt to
    // resize to just the length
    return mx_string_resize(string, length);
  } else return string;
}

mx_string_t mx_string_insert(mx_string_t string, size_t i, char c) {
  return mx_string_inject(string, i, &c, 1);
}

mx_string_t mx_string_inject(mx_string_t string, size_t i, char *c, size_t n) {
  size_t length;

  if (mx_addz_overflow(mx_string_length(string), n, &length))
    return NULL;

  if ((string = mx_string_ensure(string, length)) == NULL)
    return NULL;

  // move the existing characters n bytes toward the tail
  memmove(string + i + n, string + i, mx_string_length(string) - i + 1);

  // use memcpy instead of strncpy because strncpy stops on NUL
  memcpy(string + i, c, n);

  // increase the length
  string_to_header(string)->length = length;

  return string;
}

mx_string_t mx_string_remove(mx_string_t string, size_t i) {
  return mx_string_excise(string, i, 1);
}

mx_string_t mx_string_excise(mx_string_t string, size_t i, size_t n) {
  size_t length = mx_string_length(string) - n;

  // move the existing characters n bytes toward the head
  memmove(string + i, string + i + n, length - i);

  if (length <= (mx_string_volume(string) - 1) / 2) {
    mx_string_t resize;
    // just volume = (length * 6 + 4) / 5 avoiding intermediate overflow
    size_t volume = length / 5 * 6 + ((length % 5) * 6 + 4) / 5;

    if ((resize = mx_string_resize(string, volume)) != NULL)
      string = resize;
  }

  // decrease the length
  string_to_header(string)->length = length;

  return string;
}

mx_string_t mx_string_append(mx_string_t string, char c) {
  return mx_string_insert(string, mx_string_length(string), c);
}

mx_string_t mx_string_extend(mx_string_t string, char *c, size_t n) {
  return mx_string_inject(string, mx_string_length(string), c, n);
}

__attribute__((__format__(__printf__, 2, 3)))
mx_string_t mx_string_catf(mx_string_t string, char *format, ...) {
  va_list argptr1, argptr2;
  size_t buffer; // bytes available with null terminator
  size_t demand; // bytes required without null terminator

  va_start(argptr1, format);
  va_copy(argptr2, argptr1);

  buffer = mx_string_volume(string) - mx_string_length(string) + 1;
  demand = vsnprintf(string + mx_string_length(string), buffer, format, argptr1);
  va_end(argptr1);

  if (demand < buffer) {
    string_to_header(string)->length += demand;
    va_end(argptr2);
    return string;
  }

  string = mx_string_ensure(string, mx_string_length(string) + demand);
  if (string == NULL) {
    va_end(argptr2);
    return NULL;
  }

  buffer = mx_string_volume(string) - mx_string_length(string) + 1;
  demand = vsnprintf(string + mx_string_length(string), buffer, format, argptr2);
  va_end(argptr2);

  if (demand < buffer) {
    string_to_header(string)->length += demand;
    return string;
  }

  string[mx_string_length(string)] = '\0';

  return NULL;
}

char *mx_string_tail(mx_string_t string) {
  return string + mx_string_length(string) - 1;
}

bool mx_string_eq(const void *a, const void *b) {
  mx_string_t ra = (mx_string_t) a;
  mx_string_t rb = (mx_string_t) b;
  if (mx_string_length(ra) != mx_string_length(rb))
    return false;
  return memcmp(ra, rb, mx_string_length(ra)) == 0;
}

bool mx_string_ne(const void *a, const void *b) {
  return !mx_string_eq(a, b);
}

char *mx_string_in(mx_string_t string, char c, char *ante) {
  char *stop = string + mx_string_length(string);
  ante = ante == NULL ? string : ante + 1;

  for (; ante < stop; ante++) if (*ante == c) return ante;

  return NULL;
}

uint64_t mx_string_hash(const void *x) {
  mx_string_t string = (mx_string_t) x;
  return mx_fnv1a(string, mx_string_length(string));
}

void mx_string_debug(mx_string_t string) {
  fprintf(stderr, "\"%s\"", string);
}
