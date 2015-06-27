// ed2.c
//
// An ed-like text editor.
//
// There's not much here yet.
// (TODO Add stuff here.)

#include <stdio.h>

int main(int argc, char **argv) {

  // Print usage without a filename to edit.
  if (argc < 2) {
    printf("usage\n");  // TODO
    return 0;
  }

  // Try to open the filename given as argv[1].
  printf("%s\n", argv[1]);  // TODO
}
