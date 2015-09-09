// ed2.c
//
// See the top-of-file comments of ed2.h for an introduction to this module.
//

// Header for this file.
#include "ed2.h"

// Local includes.
#include "global.h"
#include "subst.h"

// Library includes.
#include <readline/readline.h>

// Standard includes.
#include <assert.h>
#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


// ——————————————————————————————————————————————————————————————————————
// Constants.

// The user can't undo when backup_current_line == no_valid_backup.
#define no_valid_backup -1


// ——————————————————————————————————————————————————————————————————————
// Globals.

char   last_command[string_capacity];

char   last_error[string_capacity];
int    do_print_errors = 0;

// The current filename.
char   filename[string_capacity];

// The lines are held in an array. The array frees removed lines for us.
// The byte stream can be formed by joining this array with "\n".
Array  lines = NULL;

int    current_line;  // This is 1-based.
int    is_modified;   // This is set in save_state; it's called for edits.

// `next_line` is used to help run global commands. Edit commands keep it
// updated when lines before it are inserted or deleted.
int    next_line = 0;  // Like current_line, this is 1-based.
int    is_running_global = 0;  // This is 1 if a global command is running.

// Data used for undos.
Array  backup_lines = NULL;
int    backup_current_line;


// ——————————————————————————————————————————————————————————————————————
// Internal functions.

// Backup functionality.

void deep_copy_array(Array src, Array dst) {
  array__clear(dst);
  array__for(char **, line, src, i) {
    array__new_val(dst, char *) = strdup(*line);
  }
}

void save_state(Array saved_lines, int *saved_current_line) {
  is_modified = 1;
  *saved_current_line = current_line;
  deep_copy_array(lines, saved_lines);
}

void load_state_from_backup() {
  deep_copy_array(backup_lines, lines);
  current_line = backup_current_line;
}

// File loading and saving functionality.

void line_releaser(void *line_vp, void *context) {
  char *line = *(char **)line_vp;
  assert(line);
  free(line);
}

Array new_lines_array() {
  Array new_array = array__new(64, sizeof(char *));
  new_array->releaser = line_releaser;
  return new_array;
}

// Initialize our data structures.
void setup_for_new_file() {
  if (lines)        array__delete(lines);
  if (backup_lines) array__delete(backup_lines);

  lines               = new_lines_array();
  is_modified         = 0;

  backup_lines        = new_lines_array();
  backup_current_line = no_valid_backup;

  // This works with new/empty files as both the i=insert and a=append commands
  // will silently clamp their index to a valid point for the user.
  current_line = 0;
  next_line    = 0;

  strcpy(last_command, "");
}

// Separate a raw buffer into a sequence of indexed lines.
// Destroys the buffer in the process.
void break_into_lines(char *buffer) {
  assert(lines);  // Check that lines has been initialized.
  assert(buffer);

  array__clear(lines);
  backup_current_line = no_valid_backup;
  is_modified = 0;

  char *buffer_ptr = buffer;
  char *line;
  while ((line = strsep(&buffer_ptr, "\n"))) {
    array__new_val(lines, char *) = strdup(line);
  }

  current_line = last_line;
}

// Load a file. Use the global `filename` unless `new_filename` is non-NULL, in
// which case, the new name replaces the global filename and is loaded.
void load_file(char *new_filename, char *full_command) {

  // Stop with a warning if the file is modified and they haven't tried before.
  if (is_modified && strcmp(last_command, full_command) != 0) {
    ed2__error(error__file_modified);
    return;
  }

  if (new_filename) strlcpy(filename, new_filename, string_capacity);
  if (strlen(filename) == 0) {
    ed2__error(error__no_current_filename);
    return;
  }

  FILE *f = fopen(filename, "rb");

  if (f == NULL) {
    if (errno == ENOENT) {
      printf("%s: No such file or directory\n", filename);
      setup_for_new_file();
      return;
    }
    // Otherwise, the file exists but we couldn't open it.
    goto bad_read;
  }

  struct stat file_stats;
  int is_err = fstat(fileno(f), &file_stats);
  if (is_err) goto bad_read;

  size_t buffer_size = file_stats.st_size;
  char * buffer = malloc(buffer_size + 1);  // + 1 for the final null character.

  int num_read = fread(buffer,       // buffer ptr
                       1,            // item size
                       buffer_size,  // num items
                       f);           // stream
  buffer[buffer_size] = '\0';        // Manually add a final null character.

  if (num_read < buffer_size) goto bad_read;

  fclose(f);

  break_into_lines(buffer);
  free(buffer);
  printf("%zd\n", buffer_size);  // Report how many bytes we read.
  return;

bad_read:

  // It feels disingenuous to me to let the user edit anything when the file may
  // exist but we can't read it. So we report an error and flat-out exit.
  printf("%s\n", error__bad_read);
  exit(1);
}

// Save the buffer. If filename is NULL, save it to the current filename.
// This returns the number of bytes written on success and -1 on error.
int save_file(char *new_filename) {
  if (new_filename) strlcpy(filename, new_filename, string_capacity);
  if (strlen(filename) == 0) {
    ed2__error(error__no_current_filename);
    return -1;
  }

  FILE *f = fopen(filename, "wb");
  if (f == NULL) {
    if (errno == EACCES) {  // A permission error has its own error string.
      char err_str[string_capacity];
      snprintf(err_str, string_capacity, "%s: permission denied", filename);
      ed2__error(err_str);
    } else {
      ed2__error(error__bad_write);  // Other write errors get a generic string.
    }
    return -1;  // -1 --> indicate error
  }

  int nbytes_written = 0;
  int was_error = 0;
  array__for(char **, line, lines, i) {
    int nbytes_this_line = 0;
    if (i) nbytes_this_line += fwrite("\n", 1, 1, f);  // 1, 1 = size, nitems
    size_t len = strlen(*line);
    nbytes_this_line += fwrite(*line,  // buffer
                               1,      // size
                               len,    // nitems
                               f);     // stream
    if (nbytes_this_line < len + (i ? 1 : 0)) was_error = 1;
    nbytes_written += nbytes_this_line;
  }

  if (was_error) {
    ed2__error(error__bad_write);
    return -1;  // -1 --> indicate error
  }

  is_modified = 0;
  fclose(f);
  printf("%d\n", nbytes_written);  // Report how many bytes we wrote.
  return nbytes_written;
}

// Parsing functions.

// This returns the number of characters scanned.
int scan_line_number(char *command, int *num) {
  int num_chars_parsed;
  int num_items_parsed = sscanf(command, "%d%n", num, &num_chars_parsed);
  return (num_items_parsed > 0 ? num_chars_parsed : 0);
}

// Functions to help execute editing/printing commands.

void print_line(int line_num, int do_add_number) {
  if (do_add_number) printf("%d\t", line_num);
  printf("%s\n", line_at_index(line_num - 1));
}

// This enters multi-line input mode. It accepts lines of input, including
// meaningful blank lines, until a line with a single period is given.
// The lines are appended to the end of the given `lines` Array.
void read_in_lines(Array lines) {
  while (1) {
    char *line = readline("");  // We own the memory of `line`.
    if (line == NULL || strcmp(line, ".") == 0) return;
    array__new_val(lines, char *) = line;
  }
}

// Enters line-reading mode and inserts the lines at the given 0-based index.
// This means exactly the first `index` lines are left untouched.
void read_and_insert_lines_at_index(int index) {
  // Silently clamp the index to legal values.
  if (index < 0)            index = 0;
  if (index > lines->count) index = lines->count;
  Array new_lines = array__new(16, sizeof(char *));
  read_in_lines(new_lines);
  // If we're appending lines at the end of the buffer, ensure the files ends in
  // a newline. Our overall position on ending newlines is to keep the original
  // state unless the user adds lines; in that case we ensure an ending newline.
  if (index == lines->count &&
      *array__item_val(new_lines, new_lines->count - 1, char *) != '\0') {
    array__new_val(new_lines, char *) = strdup("");
  }
  array__insert_items(lines, index, new_lines->items, new_lines->count);
  current_line += new_lines->count;
  if ((next_line - 1) >= index) next_line += new_lines->count;
  array__delete(new_lines);
}

// Returns true iff the range is bad.
int err_if_bad_range(int start, int end) {
  if (start < 1 || end > last_line) {
    ed2__error(error__invalid_address);
    return 1;
  }
  return 0;
}

// Returns true iff the new current line is bad.
int err_if_bad_current_line(int new_current_line) {
  if (new_current_line < 1 || new_current_line > last_line) {
    ed2__error(error__invalid_address);
    return 1;
  }
  current_line = new_current_line;
  return 0;
}

// Print out the given lines; useful for the p or empty commands.
// This simply produces an error if the range is invalid.
void print_range(int start, int end, int do_number_lines) {
  dbg_printf("%s(%d, %d, do_number_lines=%d)\n", __FUNCTION__,
             start, end, do_number_lines);
  if (err_if_bad_range(start, end)) return;
  for (int i = start; i <= end; ++i) print_line(i, do_number_lines);
}

void delete_range(int start, int end) {
  if (err_if_bad_range(start, end)) return;
  for (int n = end - start + 1; n > 0; --n) {
    array__remove_item(lines, array__item_ptr(lines, start - 1));
  }
  if (start <= next_line && next_line <= end) next_line = start;
  if (next_line > end) next_line -= (end - start + 1);
  current_line = (start <= last_line ? start : last_line);
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
  for (int i = start; i <= end; ++i) joined_len += strlen(line_at_index(i - 1));

  // 3. Allocate, join, and set the new line.
  char *new_line = malloc(joined_len);
  new_line[0] = '\0';
  for (int i = start; i <= end; ++i) strcat(new_line, line_at_index(i - 1));
  free(line_at_index(start - 1));
  line_at_index(start - 1) = new_line;
  // This method is valid because of the range checks at the function start.
  for (int i = start + 1; i <= end; ++i) {
    array__remove_item(lines, array__item_ptr(lines, i - 1));
  }

  if (start <= next_line && next_line <= end) next_line = start;
  if (next_line > end) next_line -= (end - start);

  current_line = start;  // The current line is the newly joined line.
}

// Moves the range [start, end] to be after the text currently at line dst.
void move_lines(int start, int end, int dst) {
  if (start < 1 || end < start || last_line < end) {
    ed2__error(error__invalid_range);
    return;
  }
  if (start <= dst && dst < end) {
    ed2__error(error__invalid_dst);
    return;
  }

  // 1. Deep copy the lines being moved so we can call delete_range later.
  Array moving_lines = array__new(end - start + 1, sizeof(char *));
  for (int i = start; i <= end; ++i) {
    array__new_val(moving_lines, char *) = strdup(line_at_index(i - 1));
  }

  // 2. Append the deep copy after dst.
  array__insert_items(lines, dst, moving_lines->items, moving_lines->count);
  if ((next_line - 1) >= dst) next_line += moving_lines->count;
  array__delete(moving_lines);

  // 3. Remove the original range.
  int range_len = end - start + 1;
  int offset    = (dst > end ? 0 : range_len);
  delete_range(start + offset, end + offset);  // This updates next_line.

  current_line = dst + offset;
}


// ——————————————————————————————————————————————————————————————————————
// Public functions.

void ed2__error(const char *err_str) {
  strcpy(last_error, err_str);
  printf("?\n");
  if (do_print_errors) printf("%s\n", last_error);
}

// This parses out any initial line range from a command, returning the number
// of characters parsed. If a range is successfully parsed, then current_line is
// updated to the end of this range.
int ed2__parse_range(char *command, int *start, int *end) {

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
    current_line = *end = last_line;
    return 1;  // Parsed 1 character.
  }

  int num_chars_parsed = scan_line_number(command, start);
  parsed += num_chars_parsed;

  // The <no range> case.
  if (num_chars_parsed == 0) return parsed;

  // The <int> case.
  current_line = *end = *start;
  if (*(command + parsed) != ',') return parsed;

  parsed++;  // Skip over the ',' character.
  num_chars_parsed = scan_line_number(command + parsed, end);
  parsed += num_chars_parsed;
  if (num_chars_parsed > 0) current_line = *end;

  // The <int>,<int> and <int>, cases.
  return parsed;
}

void ed2__run_command(char *command) {

  char *full_command = command;
  dbg_printf("run command: \"%s\"\n", command);

  int start, end;
  int num_range_chars = ed2__parse_range(command, &start, &end);
  command += num_range_chars;
  dbg_printf("After parse_range, s=%d e=%d c=\"%s\"\n", start, end, command);
  int is_default_range = (num_range_chars == 0);

  // First consider commands that may have a suffix.
  // This way we can easily give an error to an unexpected suffix in later code.

  switch(*command) {
    case 'm':  // Move the range to right after the line given as a suffix num.
      {
        save_state(backup_lines, &backup_current_line);
        int dst_line;
        int num_chars_parsed = scan_line_number(command + 1, &dst_line);
        if (num_chars_parsed == 0) dst_line = current_line;
        move_lines(start, end, dst_line);
        goto finally;
      }

    case 'w':  // Save the buffer to a file.
      {
        char *new_filename = NULL;  // NULL makes save_file use the global name.
        int do_quit = 0;
        if (*++command != '\0') {
          if (*command == 'q' && *(command + 1) == '\0') {
            do_quit = 1;
          } else if (*command != ' ') {
            ed2__error(error__bad_cmd_suffix);
          } else {
            new_filename = ++command;
          }
        }
        int ret_code = save_file(new_filename);
        if (do_quit && ret_code != -1) {  // ret_code -1 means save_file failed.
          exit(0);
        }
        goto finally;
      }

    case 'e':  // Load a file.
      {
        char *new_filename = NULL;  // NULL makes load_file use the global name.
        if (*++command != '\0') {
          if (*command != ' ') {
            ed2__error(error__bad_cmd_suffix);
          } else {
            new_filename = ++command;
          }
        }
        load_file(new_filename, full_command);
        goto finally;
      }

    case 's':  // Make a substitution.
      {
        char *pattern;
        char *repl;
        int   is_global;
        int   did_work = subst__parse_params(++command, &pattern,
                                             &repl, &is_global);
        if  (!did_work) goto finally;
        save_state(backup_lines, &backup_current_line);
        subst__on_lines(pattern, repl, start, end, is_global);
        free(pattern);
        free(repl);
        goto finally;
      }
  }

  // *command still points at the command character.
  // All commands below expect zero suffix, so we can reliably given an error
  // message here - and *not* run the command - if we see a suffix.

  if (*(command + 1) != '\0') {
    ed2__error(error__bad_cmd_suffix);
    goto finally;
  }

  int do_number_lines = 0;  // This value is shared by the n and p commands.

  switch(*command) {

    case 'q':
      {
        if (!is_default_range) {
          ed2__error(error__unexpected_address);
          break;
        }
        // Stop with a warning if the file is modified and they haven't tried
        // before.
        if (is_modified && strcmp(last_command, full_command) != 0) {
          ed2__error(error__file_modified);
          break;
        }
        exit(0);
      }

    case '\0': // If no range was given, advance a line. Print current_line.
      {
        if (is_default_range && !is_running_global) {
          if (err_if_bad_current_line(current_line + 1)) goto finally;
        }
        print_range(current_line, current_line, 0);  // 0 = don't add line num
        break;
      }

    case '=':  // Print the range's end line num, or last line num on no range.
      printf("%d\n", (is_default_range ? last_line : end));
      break;

    case 'n':  // Print lines with added line numbers.
      do_number_lines = 1;
      // Purposefully fall through to the next case.

    case 'p':  // Print all lines in the effective range.
      print_range(start, end, do_number_lines);
      break;

    case 'h':  // Print last error, if there was one.
      if (last_error[0]) printf("%s\n", last_error);
      break;
      
    case 'H':  // Toggle error printing.
      do_print_errors = !do_print_errors;
      break;

    case 'a':  // Append new lines.
      save_state(backup_lines, &backup_current_line);
      // This inserts at line number current_line + 1 = appending.
      read_and_insert_lines_at_index(current_line);
      break;

    case 'i':  // Insert new lines.
      save_state(backup_lines, &backup_current_line);
      read_and_insert_lines_at_index(current_line - 1);
      break;

    case 'd':  // Delete lines in the effective range.
      save_state(backup_lines, &backup_current_line);
      delete_range(start, end);
      break;

    case 'c':  // Change effective range lines into newly input lines.
      {
        save_state(backup_lines, &backup_current_line);
        int is_ending_range = (end == last_line);
        delete_range(start, end);
        int insert_index = is_ending_range ? last_line : current_line - 1;
        read_and_insert_lines_at_index(insert_index);
        break;
      }

    case 'j':  // Join the lines in the effective rnage.
      save_state(backup_lines, &backup_current_line);
      join_range(start, end, is_default_range);
      break;

    case 'u':  // Undo the last change, if there was one.
      {
        // First, check that a backup exists.
        if (backup_current_line == no_valid_backup) {
          ed2__error(error__no_backup);
          goto finally;
        }

        // 1. Current state -> swap.
        Array swap_lines = new_lines_array();
        int   swap_current_line;
        save_state(swap_lines, &swap_current_line);

        // 2. Backup -> current state.
        load_state_from_backup();

        // 3. Swap -> backup.
        array__delete(backup_lines);
        backup_lines        = swap_lines;
        backup_current_line = swap_current_line;
        break;
      }

    default:  // If we get here, the command wasn't recognized.
      ed2__error(error__bad_cmd);
  }

finally:

  // Save the command to know when it's repeated. Used by the 'q', 'e' commands.
  strlcpy(last_command, full_command, string_capacity);
}


// ——————————————————————————————————————————————————————————————————————
// Main.

int main(int argc, char **argv) {

  // Initialization.
  strcpy(last_error, "");
  setup_for_new_file();

  if (argc < 2) {
    // The empty string indicates no filename has been given yet.
    filename[0] = '\0';
  } else {
    strlcpy(filename, argv[1], string_capacity);
    load_file(NULL,  // NULL --> use the global filename
              "");   // ""   --> treat full_command as an empty string
    if (show_debug_output) {
      printf("File contents:'''\n");
      array__for(char **, line, lines, i) printf(i ? "\n%s" : "%s", *line);
      printf("'''\n");
    }
  }

  // Enter our read-eval-print loop (REPL).
  while (1) {
    char *line = readline("");  // We own the memory of `line`.
    if (global__is_global_command(line)) {
      global__read_rest_of_command(&line);
      global__parse_and_run_command(line);
    } else {
      ed2__run_command(line);  // This may exit the program.
    }
    free(line);
  }

  return 0;
}
