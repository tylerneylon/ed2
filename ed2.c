// ed2.c
//
// An ed-like text editor.
//
// Usage:
//   ed2 [filename]
//
// Opens filename if present, or a new buffer if no filename is given.
// Edit/save the buffer with essentially the same commands as the original
// ed text editor. See plan.md for a summary of which ed commands have been
// implemented.
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

// This parses out any initial line range from a command, returning the number
// of characters parsed. If a range is successfully parsed, then current_line is
// updated to the end of this range.
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

// This enters multi-line input mode. It accepts lines of input, including
// meaningful blank lines, until a line with a single period is given.
// The lines are appended to the end of the given `lines` Array.
void read_in_lines(Array lines) {
  while (1) {
    char *line = readline("");
    if (strcmp(line, ".") == 0) return;
    array__new_val(lines, char *) = strdup(line);
  }
}

// This inserts all of `sub` into `arr` so that arr[index] = sub[0].
// TODO Modify cstructs to make this easier.
void insert_subarr_into_arr(Array sub, Array arr, int index) {
  assert(arr->item_size == sub->item_size);
  int arr_count = arr->count;
  int sub_count = sub->count;
  array__add_zeroed_items(arr, sub->count);
  for (int i = arr_count - 1; i >= index; --i) {
    // arr[i + sub_count] = arr[i]
    memcpy(array__item_ptr(arr, i + sub_count),  // dst
           array__item_ptr(arr, i),              // src
           arr->item_size);                      // size
  }
  for (int i = 0; i < sub_count; ++i) {
    // arr[i + index] = sub[i]
    memcpy(array__item_ptr(arr, i + index),  // dst
           array__item_ptr(sub, i),          // src
           arr->item_size);                  // size
  }
}

// Enters line-reading mode and inserts the lines at the given 0-based index.
void read_and_insert_lines_at(int index) {
  // Silently clamp the index to legal values.
  if (index < 0)            index = 0;
  if (index > lines->count) index = lines->count;
  Array new_lines = array__new(16, sizeof(char *));
  read_in_lines(new_lines);
  insert_subarr_into_arr(new_lines, lines, index);
  current_line += new_lines->count;
  array__delete(new_lines);
}

// Print out the given lines; useful for the p or empty commands.
// This simply produces an error if the range is invalid.
void print_range(int start, int end) {
  if (start < 1 || end > last_line()) {
    error("invalid address");
    return;
  }
  for (int i = start; i <= end; ++i) print_line(i);
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
  if (*command == '\0') {
    print_range(current_line, current_line);
  }

  // TODO Clean up this command parsing bit.
  //  * Consider using a switch.
  //  * Design carefully about treating the command suffix.

  if (strcmp(command, "=") == 0) {
    if (num_range_chars == 0) {
      printf("%d\n", last_line());
    } else {
      printf("%d\n", end);
    }
  }

  if (strcmp(command, "p") == 0) {
    dbg_printf("Range parsed as [%d, %d]. Command as 'p'.\n", start, end);
    print_range(start, end);
  }

  if (strcmp(command, "h") == 0) {
    if (last_error[0]) printf("%s\n", last_error);
  }

  if (strcmp(command, "H") == 0) {
    do_print_errors = !do_print_errors;
  }

  if (strcmp(command, "a") == 0) {
    read_and_insert_lines_at(current_line);
  }

  if (strcmp(command, "i") == 0) {
    read_and_insert_lines_at(current_line - 1);
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
