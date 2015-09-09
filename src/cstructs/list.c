// list.c
//
// https://github.com/tylerneylon/cstructs
//

#include "list.h"

#ifdef DEBUG
#include "memprofile.h"
#endif

void list__insert(List *list, void *item) {
  List next_list = *list;
  *list = malloc(sizeof(ListStruct));
  (*list)->item = item;
  (*list)->next = next_list;
}

void *list__remove_first(List *list) {
  if (*list == NULL) { return NULL; }  // See note [1] below.
  ListStruct removed_item = **list;
  free(*list);
  *list = removed_item.next;
  return removed_item.item;
}

void *list__move_first(List *from, List *to) {
  if (*from == NULL) { return NULL; }  // See note [1] below.

  // from starts [a, b, ..]; to starts [c, ..].
  List a = *from;
  List b = (*from)->next;
  List c = *to;
  *to = a;
  a->next = c;
  *from = b;

  return a->item;
}

void list__delete(List *list) {
  list__delete_and_release(list, NULL, NULL);
}

void list__delete_and_release(List *list, Releaser releaser, void *context) {
  while (*list) {
    List next = (*list)->next;
    if (releaser) releaser((*list)->item, context);
    free(*list);
    *list = next;
  }
  // This leaves *list == NULL, as we want.
}

List *list__find_entry(List *list,
                       void *needle,
                       int (*val_eq_needle)(void *, void *)) {
  for (List *iter = list; *iter; iter = &((*iter)->next)) {
    if (val_eq_needle((*iter)->item, needle)) return iter;
  }
  return NULL;
}

void *list__find_value(List *list,
                       void *needle,
                       int (*val_eq_needle)(void *, void *)) {
  list__for(void *, elt, *list) if (val_eq_needle(elt, needle)) return elt;
  return NULL;
}

int list__reverse(List *list) {
  int n = 0;
  List iter = NULL;
  for (List new_next, next = *list; next; next = new_next) {
    new_next = next->next;
    next->next = iter;
    iter = next;
    ++n;
  }
  *list = iter;
  return n;
}

int list__count(List *list) {
  int n = 0;
  for (List iter = *list; iter; iter = iter->next, ++n);
  return n;
}

// [1] There's a bug in visual studio 2013 where variable declarations after
//     a one-line code block without braces aren't recognized. The workaround is
//     to add braces to those one-liners. See the comments here:
//     http://stackoverflow.com/a/9903698/3561
