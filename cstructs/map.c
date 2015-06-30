// map.c
//
// https://github.com/tylerneylon/cstructs
//
// Internal structure:
// An array of buckets of size s = 2^n, where n grows to keep
// the average load below 8.
// We look up x by finding i = hash(x), then checking buckets
// [i % s, i % (s/2), i % (s/4), ... ] down to % MIN_BUCKETS.
// If avg load is > 8 just after an addition, we double the
// number of buckets.
//

#include "map.h"

#ifdef DEBUG
#include "memprofile.h"
#endif

#include "list.h"

#define MIN_BUCKETS 16
#define MAX_LOAD 2.5


// Internal function declarations.
// ===============================

List *find_with_hash(Map map, void *needle, int h);
List *bucket_find(List *bucket, void *needle, map__Eq eq);
void double_size(Map map);
void release_and_free_pair(Map map, map__key_value *pair);

// This will be called from the array module.
void release_bucket(void *bucket, void *map);

// This will be called from the list module.
void release_key_value_pair(void *pair, void *map);


// Public functions.
// =================

Map map__new(map__Hash hash, map__Eq eq) {
  Map map = malloc(sizeof(MapStruct));
  map->count = 0;

  map->buckets = array__new(MIN_BUCKETS, sizeof(void *));
  map->buckets->releaser = release_bucket;
  array__add_zeroed_items(map->buckets, MIN_BUCKETS);

  map->hash = hash;
  map->eq = eq;
  map->key_releaser = NULL;
  map->value_releaser = NULL;
  map->pair_alloc = malloc;
  return map;
}

void map__delete(Map map) {
  array__delete_with_context(map->buckets, map);
  free(map);
}

map__key_value *map__set(Map map, void *key, void *value) {
  int h = map->hash(key);
  List *entry = find_with_hash(map, key, h);
  map__key_value *pair;
  if (entry) {
    pair = (*entry)->item;
    if (map->key_releaser && pair->key != key) {
      map->key_releaser(pair->key, NULL);
    }
    pair->key = key;
    if (map->value_releaser && pair->value != value) {
      map->value_releaser(pair->value, NULL);
    }
    pair->value = value;
    return pair;
  } else {
    // New pair.
    pair = map->pair_alloc(sizeof(map__key_value));
    pair->key = key;
    pair->value = value;

    double load = (map->count + 1) / (map->buckets->count);
    if (load > MAX_LOAD) double_size(map);

    int n = map->buckets->count;
    int index = ((unsigned int)h) % n;
    List *bucket = (List *)array__item_ptr(map->buckets, index);
    list__insert(bucket, pair);
    map->count++;
  }
  return pair;
}

void map__unset(Map map, void *key) {
  int h = map->hash(key);
  List *entry = find_with_hash(map, key, h);
  if (entry == NULL) return;
  release_and_free_pair(map, (*entry)->item);
  list__remove_first(entry);
  map->count--;
}

map__key_value *map__get(Map map, void *needle) {
  List *entry = find_with_hash(map, needle, map->hash(needle));
  return entry ? (*entry)->item : NULL;
}

void map__clear(Map map) {
  array__for(void **, elt_ptr, map->buckets, index) {
    List *list_ptr = (List *)elt_ptr;
    list__delete_and_release(list_ptr, release_key_value_pair, map);
  }
  map->count = 0;
}

map__key_value *map__next(Map map, int *i, void **p) {
  // *i is the bucket index.
  // *p is the List entry in that bucket.
  List entry = (List)(*p);
  while (entry == NULL && *i < (map->buckets->count - 1)) {
    (*i)++;
    entry = *(List *)array__item_ptr(map->buckets, *i);
  }
  if (entry == NULL && *i == (map->buckets->count - 1)) {
    *p = (void *)(1);  // A token non-NULL pointer to end the outer loops.
    return NULL;
  }
  void *item = entry->item;
  *p = (void *)entry->next;
  return item;
}

// private functions
// =================

List *find_with_hash(Map map, void *needle, int h) {
  int n = map->buckets->count;
  int index = ((unsigned int)h) % n;
  List *bucket = (List *)array__item_ptr(map->buckets, index);
  return bucket_find(bucket, needle, map->eq);
}

typedef struct {
  void *needle;
  map__Eq eq;
} needle_info;

int pair_matches_needle_info(void *p, void *i) {
  map__key_value *pair = (map__key_value *)p;
  needle_info *info = (needle_info *)i;
  return info->eq(pair->key, info->needle);
}

List *bucket_find(List *bucket, void *needle, map__Eq eq) {
  needle_info info;
  info.needle = needle;
  info.eq = eq;
  return list__find_entry(bucket, &info, pair_matches_needle_info);
}

void double_size(Map map) {
  array__add_zeroed_items(map->buckets, map->buckets->count);
  int n = map->buckets->count;
  array__for(List *, bucket, map->buckets, index) {
    List *entry = bucket;
    while (*entry) {
      map__key_value *pair = (*entry)->item;
      unsigned int h = map->hash(pair->key);
      int bucket_index = h % n;
      if (bucket_index == index) {
        entry = &((*entry)->next);
        continue;
      }
      list__remove_first(entry);
      List *new_bucket = (List *)array__item_ptr(map->buckets, bucket_index);
      list__insert(new_bucket, pair);
    }
    // The last half of the list is all new; no need to look at it.
    if (index >= n / 2) break;
  }
}

void release_and_free_pair(Map map, map__key_value *pair) {
  if (map->key_releaser)   map->key_releaser  (pair->key,   NULL);
  if (map->value_releaser) map->value_releaser(pair->value, NULL);
  free(pair);
}

void release_key_value_pair(void *pair, void *map) {
  release_and_free_pair((Map)map, (map__key_value *)pair);
}

void release_bucket(void *bucket, void *map) {
  list__delete_and_release((List *)bucket, release_key_value_pair, map);
}
