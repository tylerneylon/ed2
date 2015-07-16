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
// Constants.
///////////////////////////////////////////////////////////////////////////

// TODO Define error strings here.


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
// Macros.
///////////////////////////////////////////////////////////////////////////

// This can be used for both setting and getting.
// Don't forget to free the old value if setting.
#define line_at(index) array__item_val(lines, index, char *)


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
  if (*line_at(lines->count - 1) == '\0') {
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

// This returns the number of characters scanned.
int scan_number(char *command, int *num) {
  int num_chars_parsed;
  int num_items_parsed = sscanf(command, "%d%n", num, &num_chars_parsed);
  return (num_items_parsed ? num_chars_parsed : 0);
}
// TODO Consider using scan_number from parse_range.

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
  printf("%s\n", line_at(line_index - 1));
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

// Returns true iff the range is bad.
int err_if_bad_range(int start, int end) {
  if (start < 1 || end > last_line()) {
    error("invalid address");
    return 1;
  }
  return 0;
}

// Returns true iff the new current line is bad.
int err_if_bad_current_line(int new_current_line) {
  if (new_current_line < 1 || new_current_line > last_line()) {
    error("invalid address");  // TODO Drop magic strings.
    return 1;
  }
  current_line = new_current_line;
  return 0;
}

// Print out the given lines; useful for the p or empty commands.
// This simply produces an error if the range is invalid.
void print_range(int start, int end) {
  if (err_if_bad_range(start, end)) return;
  for (int i = start; i <= end; ++i) print_line(i);
}

void delete_range(int start, int end) {
  if (err_if_bad_range(start, end)) return;
  for (int n = end - start + 1; n > 0; --n) {
    array__remove_item(lines, array__item_ptr(lines, start - 1));
  }
  current_line = (start <= last_line() ? start : last_line());
}

void join_range(int start, int end, int is_default_range) {

  // 1. Establish and check the validity of the range.
  if (is_default_range) {
    start = current_line;
    end   = current_line + 1;
  }
  if (err_if_bad_range(start, end)) return;
  if (start == end) return;

  // 2. Calculate the size we need.
  size_t joined_len = 1;  // Start at 1 for the null terminator.
  for (int i = start; i <= end; ++i) joined_len += strlen(line_at(i - 1));

  // 3. Allocate, join, and set the new line.
  char *new_line = malloc(joined_len);
  new_line[0] = '\0';
  for (int i = start; i <= end; ++i) strcat(new_line, line_at(i - 1));
  free(line_at(start - 1));
  line_at(start - 1) = new_line;
  // This method is valid because of the range checks at the function start.
  for (int i = start + 1; i <= end; ++i) {
    array__remove_item(lines, array__item_ptr(lines, i - 1));
  }

  current_line = start;  // The current line is the newly joined line.
}

// Moves the range [start, end] to after the text currently at line dst.
void move_lines(int start, int end, int dst) {
  // TODO Check that range, dst, and the pair are all valid.

  // 1. Deep copy the lines being moved so we can call delete_range later.
  Array moving_lines = array__new(end - start + 1, sizeof(char *));
  for (int i = start; i <= end; ++i) {
    array__new_val(moving_lines, char *) = strdup(line_at(i - 1));
  }

  // 2. Append the deep copy after dst.
  insert_subarr_into_arr(moving_lines, lines, dst);
  array__delete(moving_lines);

  // 3. Remove the original range.
  int range_len = end - start + 1;
  int offset    = (dst > end ? 0 : range_len);
  delete_range(start + offset, end + offset);

  current_line = dst + offset;
}

void run_command(char *command) {

  dbg_printf("run command: \"%s\"\n", command);

  int start, end;
  int num_range_chars = parse_range(command, &start, &end);
  command += num_range_chars;
  dbg_printf("After parse_range, s=%d e=%d c=\"%s\"\n", start, end, command);
  int is_default_range = (num_range_chars == 0);

  // First consider commands that may have a suffix.
  // This way we can easily give an error to an unexpected suffix in later code.

  switch(*command) {
    case 'm':
      {
        int dst_line;
        int num_chars_parsed = scan_number(command + 1, &dst_line);
        if (num_chars_parsed == 0) dst_line = current_line;
        move_lines(start, end, dst_line);
        return;
      }
  }

  // TODO Check for, and complain about, any command suffix.

  switch(*command) {

    case 'q':  // Quit.  TODO Error if a range is provided.
      exit(0);

    case '\0': // If no range was given, advence a line. Print current_line.
      {
        if (is_default_range) {
          if (err_if_bad_current_line(current_line + 1)) return;
        }
        print_range(current_line, current_line);
        break;
      }

    case '=':  // Print the range's end line num, or last line num on no range.
      printf("%d\n", (is_default_range ? last_line() : end));
      break;

    case 'p':  // Print all lines in the effective range.
      print_range(start, end);
      break;

    case 'h':  // Print last error, if there was one.
      if (last_error[0]) printf("%s\n", last_error);
      break;
      
    case 'H':  // Toggle error printing.
      do_print_errors = !do_print_errors;
      break;

    case 'a':  // Append new lines.
      read_and_insert_lines_at(current_line);
      break;

    case 'i':  // Insert new lines.
      read_and_insert_lines_at(current_line - 1);
      break;

    case 'd':  // Delete lines in the effective range.
      delete_range(start, end);
      break;

    case 'c':  // Change effective range lines into newly input lines.
      {
        int is_ending_range = (end == last_line());
        delete_range(start, end);
        int insert_point = is_ending_range ? last_line() : current_line - 1;
        read_and_insert_lines_at(insert_point);
        break;
      }

    case 'j':  // Join the lines in the effective rnage.
      join_range(start, end, is_default_range);
      break;

    default:  // If we get here, the command wasn't recognized.
      error("unknown command");
  }

  // TODO Clean up this command parsing bit.
  //  * Design carefully about treating the command suffix.
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
