// list.h
//
// https://github.com/tylerneylon/cstructs
//
// C-based singly-linked list.
// A NULL pointer is equivalent to an empty list.
//

#pragma once

#include "array.h"

#include <stdlib.h>

typedef struct ListStruct {
  void *              item;
  struct ListStruct * next;
} ListStruct;
typedef  ListStruct * List;

void  list__insert       (List *list, void *item);

// Returns the removed item; NULL on empty lists.
void *list__remove_first (List *list);

// Returns the moved item; NULL on empty lists.
void *list__move_first   (List *from, List *to);

void list__delete             (List *list);
void list__delete_and_release (List *list, Releaser releaser, void *context);

List *list__find_entry (List *list,
                        void *needle,
                        int (*val_eq_needle)(void *, void *));
void *list__find_value (List *list,
                        void *needle,
                        int (*val_eq_needle)(void *, void *));

int list__reverse (List *list);
int list__count   (List *list);

// These macros are to be able to get a unique token within other macros.
// See http://stackoverflow.com/questions/1597007/
#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)
#define UNIQUE TOKENPASTE2(unique, __LINE__)

#define list__for(type, var, list) \
  List UNIQUE = list; \
  for (type var = (type)(UNIQUE ? UNIQUE->item : NULL); \
      UNIQUE; \
      UNIQUE = UNIQUE->next, var = (type)(UNIQUE ? UNIQUE->item : NULL))

