// subst.c
//

// Header for this file.
#include "subst.h"

// Local includes.
#include "cstructs/cstructs.h"
#include "ed2.h"

// Standard includes.
#include <assert.h>
#include <regex.h>
#include <string.h>


// ——————————————————————————————————————————————————————————————————————
// Internal functions.

// This accepts *line_ptr = <prefix> <match> <suffix> and <repl>, where <match>
// has offsets [start, end). It allocates a new string just long enough to hold
// <prefix> <repl> <suffix>, frees *line_ptr, and reassigns *line_ptr to the new
// string.
void substring_repl(char **line_ptr, size_t start, size_t end, char *repl) {
  assert(line_ptr && *line_ptr && repl);
  int orig_line_len = strlen(*line_ptr);
  assert(    0 <= start && start <  orig_line_len);
  assert(start <= end   &&   end <= orig_line_len);

  // The + 1 here is for the terminating null.
  size_t new_size = orig_line_len - (end - start) + strlen(repl) + 1;
  char *new_line = malloc(new_size);

  // *line_ptr = <prefix> <match> <suffix>
  memcpy(new_line, *line_ptr, start);             // new_line  = <prefix>
  char *cursor = stpcpy(new_line + start, repl);  // new_line += <repl>
  strcpy(cursor, *line_ptr + end);                // new_line += <suffix>

  free(*line_ptr);
  *line_ptr = new_line;
}

// This expands a replacement string repl and a set of matches into a full
// literal replacement string full_repl for those matches.
// If `full_repl` is not NULL, this allocates a new string with the full
// replacement string based on `repl` and the given matches, and places it in
// `full_repl`. The caller is responsible for freeing that string. If
// `full_repl` is NULL, this returns the number of bytes to allocate for
// `full_repl`.
size_t make_full_repl(char *repl, char *string, regmatch_t *matches,
                      char **full_repl) {
  size_t bytes_needed = 0;
  if (full_repl) {
    bytes_needed = make_full_repl(repl, string, matches, NULL);
    *full_repl = malloc(bytes_needed);
  }
  char *out = full_repl ? *full_repl : NULL;
  size_t len;
  for (char *cursor = repl; *cursor; ++cursor, bytes_needed += len) {
    len = 1;
    if (*cursor == '&') {
      len = matches[0].rm_eo - matches[0].rm_so;
      if (out) {
        memcpy(out, string + matches[0].rm_so, len);
        out += len;
      }
      continue;
    }
    if (*cursor != '\\') {
      if (out) *out++ = *cursor;
      continue;
    }
    cursor++;

    // Handle the case that repl ends in a backslash.
    if (*cursor == '\0') {
      if (out) *out++ = '\\';
      continue;
    }

    // *cursor is now at an escaped character
    int i = *cursor - '0';
    if (1 <= i && i < max_matches) {
      len = matches[i].rm_eo - matches[i].rm_so;
      if (out) {
        memcpy(out, string + matches[i].rm_so, len);
        out += len;
      }
      continue;
    }

    // Treat *cursor as a character literal.
    if (out) *out++ = *cursor;
  }
  if (out) *out = '\0';

  return bytes_needed + 1;  // + 1 for the final null.
}

// Make the given substitution on the line with user-oriented (1-indexed) index
// `line_num`, starting at `offset` characters into the line. This returns the
// next offset to use for other non-overlapping substitutions on the same line,
// or -1 if there was an error. If `err_str` is the empty string, it is updated
// with a user-friendly error string in case of an error.
int substitute_on_line(regex_t *compiled_re, int line_num, int offset,
                       char *repl, char *err_str) {
  regmatch_t matches[max_matches];
  int exec_flags = 0;
  char *string = line_at_index(line_num - 1) + offset;
  int err_code = regexec(compiled_re, string, max_matches, &matches[0],
                         exec_flags);
  if (err_code) {
    // We'll save only the first-seen error.
    if (err_code != REG_NOMATCH && err_str[0] == '\0') {
      regerror(err_code, compiled_re, err_str, string_capacity);
    }
    return -1;
  }
  char *full_repl;
  make_full_repl(repl, string, &matches[0], &full_repl);
  substring_repl(array__item_ptr(lines, line_num - 1),  // char ** to update
                 matches[0].rm_so + offset,             // start offset
                 matches[0].rm_eo + offset,             //   end offset
                 full_repl);                            // replacement
  int new_offset = matches[0].rm_so + offset + strlen(full_repl);
  free(full_repl);
  return new_offset;
}


// ——————————————————————————————————————————————————————————————————————
// Public functions.

// This expects to receive a string of the form "/regex/repl/", which it parses
// and places into pattern and repl, allocating new space for the copies. The
// return value is true iff the parse was successful. The caller only needs to
// call free on pattern and repl when the return value is true.
int subst__parse_params(char *command, char **pattern, char **repl,
                        int *is_global) {
  char *cursor = command;

  if (*cursor != '/') {
    ed2__error(error__no_slash_in_s_cmd);
    return 0;  // 0 = did not work
  }
  cursor++;  // Skip the current '/'.

  // `pattern` will have offsets [p_start, p_start + p_len).
  // This code does *not* allow for an escaped '/' char in the pattern.
  int p_start = cursor - command;
  while (*cursor && *cursor != '/') cursor++;
  if (*cursor == '\0') {
    ed2__error(error__bad_regex_end);
    return 0;  // 0 = did not work
  }
  int p_end = cursor - command;
  int p_len = p_end - p_start;

  cursor++;  // Skip the current '/'.

  // `repl` will have offsets [r_start, r_start + r_len).
  // This code does *not* allow for an escaped '/' char in the repl.
  int r_start = cursor - command;
  while (*cursor && *cursor != '/') cursor++;
  // Be chill if there's no trailing '/'.
  int r_end = cursor - command;
  int r_len = r_end - r_start;

  // Check for a global flag.
  *is_global = 0;
  if (*cursor) {
    if (*(cursor + 1) == 'g') {
      *is_global = 1;
    } else if (*(cursor + 1) != '\0') {
      ed2__error(error__bad_cmd_suffix);
      return 0;  // 0 = did not work
    }
  }

  // Allocate and copy our output strings.
  *pattern = calloc(p_len + 1, 1);  // + 1 for the final null, 1 = size
  memcpy(*pattern, command + p_start, p_len);
  *repl    = calloc(r_len + 1, 1);  // + 1 for the final null, 1 = size
  memcpy(*repl,    command + r_start, r_len);

  return 1;  // 1 = did work
}

void subst__on_lines(char *pattern, char *repl,
                     int start, int end, int is_global) {
  regex_t compiled_re;
  int compile_flags = REG_EXTENDED;
  char err_str[string_capacity];
  err_str[0] = '\0';

  int err_code = regcomp(&compiled_re, pattern, compile_flags);
  if (err_code) {
    regerror(err_code, &compiled_re, err_str, string_capacity);
    ed2__error(err_str);
    // The man page at regex(3) doesn't make it clear if we should call regfree
    // when regcomp has an error. However, looking at the source:
    // http://www.opensource.apple.com/source/gcc/gcc-5659/libiberty/regex.c
    // it's clear that calling regfree here is at very least safe, and in my
    // estimation is the right thing to do:
    regfree(&compiled_re);
    return;
  }

  int did_match_any = 0;
  for (int i = start; i <= end; ++i) {
    // j tracks the offset into the line for global matches; 0 = initial offset.
    int j = substitute_on_line(&compiled_re, i, 0, repl, err_str);
    if (j >= 0) did_match_any = 1;
    while (is_global && j > 0) {
      j = substitute_on_line(&compiled_re, i, j, repl, err_str);
    }
  }
  if (err_str[0] != '\0') ed2__error(err_str);
  else if (!did_match_any) ed2__error(error__no_match);
  regfree(&compiled_re);
}
