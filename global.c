// global.c
//

// Header for this file.
#include "global.h"

// Local includes.
#include "cstructs/cstructs.h"
#include "ed2.h"

// Library includes.
#include <readline/readline.h>

// Standard includes.
#include <assert.h>
#include <regex.h>
#include <string.h>


// Internal functions.

// The next two functions are for the matched_lines hash map.
int hash_line(void *line_vptr) {
  return (int)(intptr_t)(line_vptr);
}

int eq_lines(void *line1_vptr, void *line2_vptr) {
  return line1_vptr == line2_vptr;
}

// `commands` is an Array with `char *` items; each is a single-line command
// that can be executed with a call to ed2__run_command.
void run_global_command(int start, int end, char *pattern,
                        Array commands, int is_inverted) {
  is_running_global = 1;
  dbg_printf("%s(start=%d, end=%d, pattern='%s', <commands>)\n",
             __FUNCTION__, start, end, pattern);

  // Save the current error string so we can notice any execution errors.
  char   saved_error[string_capacity];
  strcpy(saved_error, last_error);
  strcpy( last_error, "");

  // We run the command using two passes:
  // 1. Build a Map-based set of lines in the range that match `regex`, and
  // 2. Use the `next_line` global to go through the file once, running
  //    `commands` on each matching line. `next_line` is kept up to date even
  //    when other commands edit the buffer.

  // Declare variables early if they're used in the finally goto-target block.

  Map matched_lines = NULL;

  // Pass 1: Build the set of matching lines.

  // 1A: Compile the regex pattern.
  regex_t compiled_re;
  int compile_flags = REG_EXTENDED;
  char err_str[string_capacity];
  err_str[0] = '\0';
  int err_code = regcomp(&compiled_re, pattern, compile_flags);
  if (err_code) {
    regerror(err_code, &compiled_re, err_str, string_capacity);
    ed2__error(err_str);
    goto finally;
  }

  // 1B: Find all currently matching lines.
  matched_lines = map__new(hash_line, eq_lines);
  int exec_flags  = 0;
  for (int i = start; i <= end; ++i) {
    regmatch_t matches[max_matches];
    int err_code = regexec(&compiled_re, line_at(i - 1), max_matches,
                           &matches[0], exec_flags);
    if ((!is_inverted && err_code == 0) ||
        ( is_inverted && err_code == REG_NOMATCH)) {
      map__set(matched_lines, line_at(i - 1), 0);
    } else if (err_code && err_code != REG_NOMATCH && err_str[0] == '\0') {
      regerror(err_code, &compiled_re, err_str, string_capacity);
      ed2__error(err_str);
      goto finally;
    }
  }

  // Pass 2: Run `commands` on each matching line.

  for (next_line = 1; next_line <= last_line;) {
    if (!map__get(matched_lines, line_at(next_line - 1))) {
      next_line++;  // Skip to the next line if this one doesn't match.
      continue;
    }
    current_line = next_line;
    next_line++;
    array__for(char **, sub_cmd, commands, i) {
      ed2__run_command(*sub_cmd);
      if (last_error[0]) goto finally;  // Stop early on errors.
    }
  }

finally:
  // It appears correct to call regfree even if regcomp fails; see the comment
  // at the first use of regfree for more details.
  regfree(&compiled_re);
  if (matched_lines != NULL) map__delete(matched_lines);
  if (last_error[0] == '\0') strcpy(last_error, saved_error);
  is_running_global = 0;
}

// Returns 1 iff the given line ends with a backslash-escaped newline,
// indicating that there are more commands to the sequence.
int does_end_in_continuation(char *line) {
  assert(line);
  int n = strlen(line);
  return n ? (line[n - 1] == '\\') : 0;
}


// Public functions.

// Returns 1 iff the given command is the first line of a global command.
int global__is_global_command(char *command) {
  assert(command);
  int start, end;
  command += ed2__parse_range(command, &start, &end);
  return *command == 'g' || *command == 'v';
}

// This expects *line to be the first, and possibly only, line of a global
// command. If *line ends in a continuation, this reads and appends more lines
// with joining newline characters until the command sequence is complete. The
// final string will not end with a newline. It's expected that the caller owns
// *line, and the caller keeps the responsibility of freeing *line. This
// function may free and reallocate the memory at *line.
void global__read_rest_of_command(char **line) {
  while (does_end_in_continuation(*line)) {
    char *new_part = readline("");  // We own the memory of `new_part`.
    // Append new_part to *line; the + 2 is for the newline and the null.
    size_t new_size = strlen(*line) + strlen(new_part) + 2;
    char * new_line = calloc(new_size, 1);  // count, size
    char * cursor   = new_line;
    cursor = stpcpy(cursor, *line);
    cursor = stpcpy(cursor, "\n");
    cursor = stpcpy(cursor, new_part);
    free(*line);
    free(new_part);
    *line = new_line;
  }
}

// This runs the given global command string, which is expected to be in the
// format that's read in by global__read_rest_of_command.
void global__parse_and_run_command(char *command) {
  // Parse the command.
  assert(command);
  int start, end;
  int num_range_chars = ed2__parse_range(command, &start, &end);
  command += num_range_chars;
  if (num_range_chars == 0) {
    start = 1;
    end   = last_line;
  }
  assert(*command == 'g' || *command == 'v');
  int is_inverted = (*command == 'v');
  command++;

  // Parse the regular expression.
  if (*command != '/') {
    ed2__error(error__bad_regex_start);
    return;
  }
  command++;
  char *regex = command;  // This memory remains owned by the caller.
  while (*command && *command != '/') command++;
  if (*command != '/') {
    ed2__error(error__bad_regex_end);
    return;
  }
  *command = '\0';  // This is the null terminator for the regex string.
  command++;

  // Make a list out of the command sequence.
  // The commands list holds weak pointers into `command`; this memory
  // will be freed by the caller after this function completes.
  Array commands = array__new(4, sizeof(char *));
  char *sub_cmd;
  while ((sub_cmd = strsep(&command, "\n"))) {
    array__new_val(commands, char *) = sub_cmd;
  }
  // Remove the trailing slashes.
  array__for(char **, sub_cmd, commands, i) {
    if (i == (commands->count - 1)) continue;  // Skip it; no trailing slash.
    assert(strlen(*sub_cmd) >= 1);
    (*sub_cmd)[strlen(*sub_cmd) - 1] = '\0';
  }

  run_global_command(start, end, regex, commands, is_inverted);
}
