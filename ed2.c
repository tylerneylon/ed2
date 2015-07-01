// ed2.c
//
// An ed-like text editor.
//
// There's not much here yet.
// (TODO Add stuff here.)
//

// Local library includes.
#include "cstructs/cstructs.h"

// Library includes.
#include <readline/readline.h>

// Standard includes.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


///////////////////////////////////////////////////////////////////////////
// Globals.
///////////////////////////////////////////////////////////////////////////

// The current filename.
char   filename[1024];

// The lines are held in an array. The array frees removed lines for us.
// The byte stream can be formed by joining this array with "\n".
Array  lines = NULL;


///////////////////////////////////////////////////////////////////////////
// Internal functions.
///////////////////////////////////////////////////////////////////////////

void line_releaser(void *line_vp, void *context) {
  char *line = *(char **)line_vp;
  assert(line);
  free(line);
}

// Initialize our data structures.
void init() {
  lines = array__new(64, sizeof(char *));
  lines->releaser = line_releaser;
}

// Separate a raw buffer into a sequence of indexed lines.
// Destroys the buffer in the process.
void find_lines(char *buffer) {
  assert(lines);  // Check that lines has been initialized.
  assert(buffer);

  char *buffer_ptr = buffer;
  char *line;
  while ((line = strsep(&buffer_ptr, "\n"))) {
    array__new_val(lines, char *) = strdup(line);
  }
}

// Load the file at the global `filename` into the global `buffer`.
void load_file() {
  FILE *f = fopen(filename, "rb");
  // TODO Handle the case that f is NULL.
  struct stat file_stats;
  int is_err = fstat(fileno(f), &file_stats);
  // TODO Handle is_err != 0.

  size_t buffer_size = file_stats.st_size;
  char * buffer = malloc(buffer_size + 1);  // + 1 for the final NULL character.

  is_err = fread(buffer,       // buffer ptr
                 1,            // item size
                 buffer_size,  // num items
                 f);           // stream
  buffer[buffer_size] = '\0';  // Manually add a final NULL character.

  // TODO Handle is_err != 0.

  fclose(f);

  find_lines(buffer);
  free(buffer);
}


///////////////////////////////////////////////////////////////////////////
// Main.
///////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {

  // Initialization.
  init();

  if (argc < 2) {
    // The empty string indicates no filename has been given yet.
    filename[0] = '\0';
  } else {
    strcpy(filename, argv[1]);
    load_file();

    // TEMP
    printf("File contents:'''\n");
    array__for(char **, line, lines, i) printf("%s%s", (i ? "\n" : ""), *line);
    printf("'''\n");
  }

  // Enter our read-eval-print loop (REPL).
  while (1) {
    char *line = readline("");
    if (strcmp(line, "q") == 0) return 0;
  }

  return 0;
}
