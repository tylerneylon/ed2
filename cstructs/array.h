// array.h
//
// https://github.com/tylerneylon/cstructs
//
// A C structure for flexibly working with a sequence
// of items that are kept contiguously in memory.
// The array length is flexible, and is designed to
// support nesting of data structures.
//

#pragma once

#include <stdlib.h>

typedef void (*Releaser)(void *item, void *context);

typedef struct {
  int      count;
  int      capacity;
  size_t   item_size;
  Releaser releaser;
  char *   items;
} ArrayStruct;

typedef ArrayStruct *Array;


// Constant-time operations.

// Allocates and initializes a new array.
Array array__new  (int capacity, size_t item_size);

// For use on an allocated but uninitialized array struct.
Array array__init (Array array, int capacity, size_t item_size);


// The next three methods are O(1) if there's no releaser; O(n) if there is.
void  array__clear   (Array array);  // Releases all items and sets count to 0.
void  array__release (void *array);  // Releases/frees all mem but array itself.
void  array__delete  (Array array);  // Releases array and frees array itself.

// These do the same job as the above ones and send a context to the releaser.
void array__clear_with_context   (Array array, void *context);
void array__release_with_context (void *array, void *context);
void array__delete_with_context  (Array array, void *context);

void *  array__item_ptr(Array array, int index);
#define array__item_val(array, i, type) (*(type *)array__item_ptr(array, i))

// Amortized constant-time operations (usually constant-time, sometimes linear).

void    array__add_item_ptr(Array array, void *item);
#define array__add_item_val(a, i) array__add_item_ptr(a, &i);
void *  array__new_ptr(Array array);
#define array__new_val(a, type) (*(type *)array__new_ptr(a))

// Possibly linear time operations.

void array__insert_items (Array array, int index, void *items, int num_items);
void array__append_array (Array dst, Array src);  // Expects dst != src.
int  array__index_of     (Array array, void *item);

// The item is expected to be an object already within the array, i.e.,
// the location of item should be in the array->items memory buffer.
void array__remove_item      (Array array, void *item);
void array__add_zeroed_items (Array array, int num_items);

// Loop over an array.
// Example: array__for(item_type *, item_ptr, array, index) { /* loop body */ }
// Think:   type item_ptr = &array[index];  // for each index in the array.

// If you used sizeof(x) to set up the array, then the type for array__for is
// expected to be "x *" (a pointer to x). It is safe to continue or break, edit
// the index, and to edit the array itself, although array changes - including
// *any* additions at all - may invalidate item_ptr until the start of the next
// iteration.
#define array__for(type, item_ptr, array, index)            \
  for (int index = 0, __tmpvar = 1; __tmpvar--;)            \
  for (type item_ptr = (type)array__item_ptr(array, index); \
       index < array->count;                                \
       item_ptr = (type)array__item_ptr(array, ++index))
// The (type) cast in array__for is required by C++.

typedef int (*array__CompareFunction)(void *, const void *, const void *);

void array__sort(Array array,
                 array__CompareFunction compare,
                 void *compare_context);

// Assumes the array is sorted in ascending memcmp order; does a memcmp of
// each item in the array, using a binary search.
void *array__find(Array array, void *item);
