// subst.h
//
// Functions to perform regular expression substitutions.
//

#pragma once


// ——————————————————————————————————————————————————————————————————————
// Public functions.

// This expects to receive a string of the form "/regex/repl/", which it parses
// and places into pattern and repl, allocating new space for the copies. The
// return value is true iff the parse was successful. The caller only needs to
// call free on pattern and repl when the return value is true.
int  subst__parse_params(char *command, char **pattern, char **repl,
                         int *is_global);

// This substitutes matches of the given pattern with the given replacement
// string `repl`. Only line numbers in the range [start, end] are affected. If
// is_global is false, then only the first match in each line is affected; if
// it's true, then every non-overlapping match is affected.
void subst__on_lines(char *pattern, char *repl,
                     int start, int end, int is_global);

