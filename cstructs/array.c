// array.c
//
// https://github.com/tylerneylon/cstructs
//

#include "array.h"

#ifdef DEBUG
#include "memprofile.h"
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


Array array__new(int capacity, size_t item_size) {
  Array array = malloc(sizeof(ArrayStruct));
  return array__init(array, capacity, item_size);
}

Array array__init(Array array, int capacity, size_t item_size) {
  if (capacity < 1) capacity = 1;
  array->count = 0;
  array->capacity = capacity;
  array->item_size = item_size;
  array->releaser = NULL;
  if (capacity) {
    array->items = malloc((int)item_size * capacity);
  } else {
    array->items = NULL;
  }
  return array;
}

void array__clear_with_context(Array array, void *context) {
  if (array->releaser) {
    for (int i = 0; i < array->count; ++i) {
      array->releaser(array__item_ptr(array, i), context);
    }
  }
  array->count = 0;
}

void array__release_with_context(void *array, void *context) {
  Array a = (Array)array;
  array__clear_with_context(a, context);
  free(a->items);
  a->capacity = 0;
}

void array__delete_with_context(Array array, void *context) {
  array__release_with_context(array, context);
  free(array);
}

void array__clear(Array array) {
  array__clear_with_context(array, NULL);   // NULL --> context
}

void array__release(void *array) {
  array__release_with_context(array, NULL);  // NULL --> context
}

void array__delete(Array array) {
  array__delete_with_context(array, NULL);  // NULL --> context
}

void *array__item_ptr(Array array, int index) {
  return (void *)(array->items + index * array->item_size);
}

void array__add_item_ptr(Array array, void *item) {
  memcpy(array__new_ptr(array), item, array->item_size);
}

void *array__new_ptr(Array array) {
  if (array->count == array->capacity) {
    array->capacity *= 2;
    if (array->capacity == 0) array->capacity = 1;
    array->items = realloc(array->items, array->capacity * (int)array->item_size);
  }
  array->count++;
  return array__item_ptr(array, array->count - 1);
}

void array__append_array(Array dst, Array src) {
  // We avoid using array__for since we don't know which type of pointer to use.
  for (int i = 0; i < src->count; ++i) {
    void *item = array__item_ptr(src, i);
    array__add_item_ptr(dst, item);
  }
}

int array__index_of(Array array, void *item) {
  ptrdiff_t byte_dist = (char *)item - array->items;
  return (int)(byte_dist / array->item_size);
}

void array__remove_item(Array array, void *item) {
  if (array->releaser) array->releaser(item, NULL);
  int num_left = --(array->count);
  char *item_byte = (char *)item;
  ptrdiff_t byte_dist = item_byte - array->items;
  int index = (int)(byte_dist / array->item_size);
  if (index == num_left) return;
  memmove(item_byte, item_byte + array->item_size, (num_left - index) * array->item_size);
}

void array__add_zeroed_items(Array array, int num_items) {
  int new_count = array->count + num_items;
  int resize_needed = 0;
  while (array->capacity < new_count) {
    array->capacity *= 2;
    if (array->capacity == 0) array->capacity = 1;
    resize_needed = 1;
  }
  if (resize_needed) {
    array->items = realloc(array->items, array->capacity * (int)array->item_size);
  }
  void *bytes_to_zero = array__item_ptr(array, array->count);
  memset(bytes_to_zero, 0, num_items * array->item_size);
  array->count = new_count;
}

static int compare_as_ints(void *item_size, const void *item1, const void *item2) {
  size_t s = *(size_t *)item_size;
  unsigned char *i1 = (unsigned char *)item1;
  unsigned char *i2 = (unsigned char *)item2;
  for (size_t i = 0; i < s; ++i) {
    int diff = *(i1 + i) - *(i2 + i);
    if (diff != 0) return diff;
  }
  return 0;
}

typedef int (*CompareFn)(void *context, const void *item1, const void *item2);

static CompareFn user_compare;
static void *user_context;

static int custom_compare(const void *item1, const void *item2) {
  return user_compare(user_context, item1, item2);
}

void array__sort(Array array, array__CompareFunction compare, void *compare_context) {
  CompareFn old_compare = user_compare;
  void *old_context = user_context;

  if (compare == NULL) {
    user_compare = compare_as_ints;
    user_context = &(array->item_size);
  } else {
    user_compare = compare;
    user_context = compare_context;
  }
  qsort(array->items, array->count, array->item_size, custom_compare);

  user_compare = old_compare;
  user_context = old_context;
}

static size_t item_size;

static int compare_for_find(const void *a, const void *b) {
  return memcmp(a, b, item_size);
}

void *array__find(Array array, void *item) {
  size_t old_size = item_size;
  item_size = array->item_size;
  void *found_val = bsearch(item, array->items, array->count, array->item_size, compare_for_find);
  item_size = old_size;
  return found_val;
}
