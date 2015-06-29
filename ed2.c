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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


///////////////////////////////////////////////////////////////////////////
// Globals.
///////////////////////////////////////////////////////////////////////////

// The current filename.
char   filename[1024];

// The buffer that holds the text.
char * buffer;
size_t buffer_size;  // This excludes a final NULL terminator.

// 0-indexed line n has its 1st character at line_indexes[n] and its last
// at line_indexes[n + 1] - 1.
// The last line begines at line_indexes[num_lines - 1].
int *  line_indexes;
int    num_lines;


///////////////////////////////////////////////////////////////////////////
// Internal functions.
///////////////////////////////////////////////////////////////////////////

void find_lines() {
  char *buf_copy = strdup(buffer);
  char *buf_copy_ptr = buf_copy;
  num_lines = 0;
  char *line;

  // TODO Correctly allocate line_indexes; maybe using cstructs is good.
  line_indexes = malloc(128 * sizeof(int));

  while ((line = strsep(&buf_copy_ptr, "\n"))) {
    printf("line:\n%s\n", line);
    line_indexes[num_lines++] = (line - buf_copy);
    printf("%x\n", line_indexes[num_lines - 1]);
  }
  line_indexes[num_lines] = strlen(buffer);
  printf("%x\n", line_indexes[num_lines]);
  printf("num_lines=%d\n", num_lines);

  free(buf_copy);
}

// Load the file at the global `filename` into the global `buffer`.
void load_file() {
  FILE *f = fopen(filename, "rb");
  // TODO Handle the case that f is NULL.
  struct stat file_stats;
  int is_err = fstat(fileno(f), &file_stats);
  // TODO Handle is_err != 0.

  buffer_size = file_stats.st_size;
  buffer = malloc(buffer_size + 1);  // + 1 for the final NULL character.

  is_err = fread(buffer,       // buffer ptr
                 1,            // item size
                 buffer_size,  // num items
                 f);           // stream
  buffer[buffer_size] = '\0';  // Manually add a final NULL character.

  // TODO Handle is_err != 0.

  fclose(f);

  find_lines();
}


///////////////////////////////////////////////////////////////////////////
// Main.
///////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {

  // Initialization.

  buffer_size = 0;  // Indicates no buffer is loaded.

  if (argc < 2) {
    // The empty string indicates no filename has been given yet.
    filename[0] = '\0';
  } else {
    strcpy(filename, argv[1]);
    load_file();

    // TEMP
    printf("File contents:'''\n%s'''\n", buffer);
  }

  // Enter our read-eval-print loop (REPL).
  while (1) {
    char *line = readline("");
    if (strcmp(line, "q") == 0) return 0;
  }

  return 0;
}
