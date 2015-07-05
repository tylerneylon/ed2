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
// Debug.
///////////////////////////////////////////////////////////////////////////

#define show_debug_output 0

#if show_debug_output
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif


///////////////////////////////////////////////////////////////////////////
// Globals.
///////////////////////////////////////////////////////////////////////////

// The current filename.
char   filename[1024];

// The lines are held in an array. The array frees removed lines for us.
// The byte stream can be formed by joining this array with "\n".
Array  lines = NULL;

int    current_line;

char   last_error[1024];

int    do_print_errors = 0;

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

  // TODO This default only matters for new files. Study ed's behavior to
  //      determine what the current line effectively is for new files.
  //      This isn't obvious to me because I have the sense that 'a' will
  //      append after line 0, while 'i' will insert before line 1.
  // This variable is the same as what the user considers it; this is
  // non-obvious in that our lines array is 0-indexed, so we have to be careful
  // when indexing into `lines`.
  current_line = 0;

  strcpy(last_error, "");
}

int last_line() {
  // If the last entry in `lines` is the empty string, then the file ends in a
  // newline; pay attention to this to avoid an off-by-one error.
  if (*array__item_val(lines, lines->count - 1, char *) == '\0') {
    return lines->count - 1;
  }
  return lines->count;
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

  current_line = last_line();
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
  printf("%zd\n", buffer_size);  // Report how many bytes we read.
}

// TODO Test/debug this.
// This parses out any initial line range from a command, returning the number
// of characters parsed.
int parse_range(char *command, int *start, int *end) {

  // For now, we'll parse ranges of the following types:
  //  * <no range>
  //  * ,
  //  * %
  //  * <int>
  //  * <int>,
  //  * <int>,<int>

  // Set up the default range.
  *start = *end = current_line;

  int parsed = 0;

  // The ',' and '%' cases.
  if (*command == ',' || *command == '%') {
    *start = 1;
    current_line = *end = last_line();
    return 1;  // Parsed 1 character.
  }

  int num_chars_parsed;
  int num_parsed = sscanf(command, "%d%n", start, &num_chars_parsed);
  parsed += num_chars_parsed * (num_parsed > 0);

  // The <no range> case.
  if (num_parsed == 0) return parsed;

  // The <int> case.
  current_line = *end = *start;
  if (*(command + parsed) != ',') return parsed;

  parsed++;  // Skip over the ',' character.
  num_parsed = sscanf(command + parsed, "%d%n", end, &num_chars_parsed);
  parsed += num_chars_parsed * (num_parsed > 0);
  if (num_parsed > 0) current_line = *end;

  // The <int>,<int> and <int>, cases.
  return parsed;
}

void print_line(int line_index) {
  printf("%s\n", array__item_val(lines, line_index - 1, char *));
}

void error(const char *err_str) {
  strcpy(last_error, err_str);
  printf("?\n");
  if (do_print_errors) printf("%s\n", last_error);
}

void run_command(char *command) {

  dbg_printf("run command: \"%s\"\n", command);

  if (*command == '\0') {
    error("invalid address");
    return;
  }

  if (strcmp(command, "q") == 0) exit(0);

  int start, end;
  int num_range_chars = parse_range(command, &start, &end);
  command += num_range_chars;
  dbg_printf("After parse_range, s=%d e=%d c=\"%s\"\n", start, end, command);

  // The empty command updates `current_line` and prints it out.
  if (*command == '\0') print_line(current_line);

  if (strcmp(command, "p") == 0) {
    dbg_printf("Range parsed as [%d, %d]. Command as 'p'.\n", start, end);
    // Print the lines in the range [start,end].
    for (int i = start; i <= end; ++i) print_line(i);
  }

  if (strcmp(command, "h") == 0) {
    if (last_error[0]) printf("%s\n", last_error);
  }

  if (strcmp(command, "H") == 0) {
    do_print_errors = !do_print_errors;
  }
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

    if (show_debug_output) {
      printf("File contents:'''\n");
      array__for(char **, line, lines, i) printf(i ? "\n%s" : "%s", *line);
      printf("'''\n");
    }
  }

  // Enter our read-eval-print loop (REPL).
  while (1) {
    char *line = readline("");
    run_command(line);
  }

  return 0;
}
