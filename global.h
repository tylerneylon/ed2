// global.h
//
// Functions to help work with global commands.
//

#pragma once

// Returns 1 iff the given command is the first line of a global command.
int  global__is_global_command(char *command);

// This expects *line to be the first, and possibly only, line of a global
// command. If it ends in a continuation, reads and appends more lines with
// joining newline characters until the command sequence is complete. The final
// string will not end with a newline. It's expected that the caller owns
// *line. This function may free and reallocate the memory at *line.
void global__read_rest_of_command(char **line);

// This runs the given global command string, which is expected to be in the
// format that's read in by global__read_rest_of_command.
void global__parse_and_run_command(char *command);
