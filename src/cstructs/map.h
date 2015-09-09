// map.h
//
// https://github.com/tylerneylon/cstructs
//
// C-based hash map.
// Lookups are fast, sizing grows as needed.
//

#pragma once

#include "array.h"

#include <stdlib.h>

typedef int    ( *map__Hash  )(void *);
typedef int    ( *map__Eq    )(void *, void*);
typedef void * ( *map__Alloc )(size_t);

typedef struct {
  int        count;
  Array      buckets;
  map__Hash  hash;
  map__Eq    eq;
  Releaser   key_releaser;
  Releaser   value_releaser;
  map__Alloc pair_alloc;  // Default=malloc; customize to add fields per item.
} MapStruct;

typedef MapStruct *Map;

typedef struct {
  void *key;
  void *value;
} map__key_value;


Map              map__new    (map__Hash hash, map__Eq eq);
void             map__delete (Map map);

map__key_value * map__set    (Map map, void *key, void *value);
void             map__unset  (Map map, void *key);
map__key_value * map__get    (Map map, void *needle);

void             map__clear  (Map map);

// This is for use with map__for.
map__key_value * map__next   (Map map, int *i, void **p);

// The variable var has type map__key_value *.
#define map__for(var, map) \
  for (int    __tmp_i = -1  ; __tmp_i == -1  ;) \
  for (void * __tmp_p = NULL; __tmp_p == NULL;) \
  for (map__key_value *var = map__next(map, &__tmp_i, &__tmp_p); \
       var; var = map__next(map, &__tmp_i, &__tmp_p))
