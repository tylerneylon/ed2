// ed2.h
//
// This declares globals and functions which may be used by other modules.
//

#pragma once

#include "cstructs/cstructs.h"


///////////////////////////////////////////////////////////////////////////
// Public globals.
///////////////////////////////////////////////////////////////////////////

// Most constants are defined below, but this one helps define last_error.
#define string_capacity 1024

// `next_line` is used to help run global commands. Edit commands keep it
// updated when lines before it are inserted or deleted.
extern int next_line;     // This is 1-indexed.
extern int current_line;  // This is 1-indexed.
extern int is_running_global;  // This is 1 if a global command is running.

// The lines are held in an array. The array frees removed lines for us.
// The byte stream can be formed by joining this array with "\n".
extern Array lines;

// An empty string indicates there was no known last error.
extern char last_error[string_capacity];


///////////////////////////////////////////////////////////////////////////
// Public functions.
///////////////////////////////////////////////////////////////////////////

// This prints '?' and updates the last_error string.
void ed2__error(const char *err_str);

// This rungs the given command string.
void ed2__run_command(char *command);

// This parses out any initial line range from a command, returning the number
// of characters parsed. If a range is successfully parsed, then current_line is
// updated to the end of this range.
int  ed2__parse_range(char *command, int *start, int *end);


///////////////////////////////////////////////////////////////////////////
// Public macros and constants.
///////////////////////////////////////////////////////////////////////////

// This can be used for both setting and getting.
// Don't forget to free the old value if setting.
#define line_at(index) array__item_val(lines, index, char *)

// This provides the last 1-indexed line number.
#define last_line (*line_at(lines->count - 1) ? lines->count : lines->count - 1)

#define max_matches 10


// Debug macros.

#define show_debug_output 0

#if show_debug_output
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

// Error strings.

// File-related.
#define error__file_modified        "warning: file modified"
#define error__no_current_filename  "no current filename"
#define error__bad_write            "error while writing"

// Regex-related.
#define error__no_slash_in_s_cmd    "expected '/' after s command"
#define error__bad_regex_start      "expected '/' to start regular expression"
#define error__bad_regex_end        "expected '/' to end regular expression"
#define error__no_match             "no match"

// Address or command related.
#define error__invalid_address      "invalid address"
#define error__bad_cmd_suffix       "unexpected command suffix"
#define error__unexpected_address   "unexpected address"
#define error__bad_cmd              "unknown command"
