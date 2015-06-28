// ed2.c
//
// An ed-like text editor.
//
// There's not much here yet.
// (TODO Add stuff here.)
//

// Library includes.
#include <readline/readline.h>

// Standard includes.
#include <stdio.h>
#include <string.h>

// Globals.

char filename[1024];


int main(int argc, char **argv) {

  // Initialization.

  if (argc < 2) {
    // The empty string indicates no filename has been given yet.
    filename[0] = '\0';
  } else {
    strcpy(filename, argv[1]);
    // TODO Load the file.
  }

  // Enter our read-eval-print loop (REPL).
  while (1) {
    char *line = readline("");
    if (strcmp(line, "q") == 0) return 0;
  }

  return 0;
}
