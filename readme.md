# ed2

*an ed-like editor*

This project is a fully functional text editor in a relatively small amount of code.
I wrote it because I like text editors, I like coding, and I love learning how to
build things through experience. You may find it interesting if you also like these
things.

The `ed2` binary feels extremely similar to the old school
[`ed`](https://en.wikipedia.org/wiki/Ed_%28text_editor%29)
text editor, which
is a non-visual line-based editor. This means you *do not* see the file contents by
default as you edit them. Somewhat like a shell interface, you think in terms of
typing in commands followed by a few lines of output. The non-visual nature of
`ed` and `ed2` make them appear primitive. However, they do have some nice features
such as regular expression searching and sophisticated replacements.

## Download and compile

This code is written for Mac OS X, although it may also accidentally work in some linux
environments. The shell commands below will create a new `ed2` subdirectory in your
current directory, download and build `ed2`. The code uses both the `readline` and `regex`
libraries. As far as I know, these libraries come bundled with Mac OS X. Please let
me know if that's not true.

    git clone https://github.com/tylerneylon/ed2.git
    cd ed2 && make

## Basic usage

Here is a short example session to show a few `ed2` commands.
This example assumes you're in the `ed2` directory, and have built
`ed2` by running `make`.

    you type   | ./ed2 ed2.c
    ed2 output | 28543

The number 28543 indicates the number of bytes read from the file being edited.

    you type   | 1,6n
    ed2 output | 1	// ed2.c
    ed2 output | 2	//
    ed2 output | 3	// An ed-like text editor.
    ed2 output | 4	//
    ed2 output | 5	// Usage:
    ed2 output | 6	//   ed2 [filename]

The command `1,6n` provides a line range - the lines 1 through 6, inclusive - and a command `n`.
The `n` command tells `ed2` to print those lines annotated with line numbers. The `p`
command would have done the same thing without adding line numbers.

    you type   | ,s/[aeiou]/&&/g
    (ed2 says nothing)

This command also begins with a range followed by a command character. In this case,
the range is given by the single comma, which indicates that the command should be
run on every line. The `s` command is a substitution command which is followed by
a search pattern and then a replacement string. In this case, the search pattern
`[aeiou]` will match any single lowercase vowel. The replacement string `&&` uses
the special character `&`, which represents the matched string; in other words, this
substitution will replace every lowercase vowel with two copies of that vowel. The `g` suffix
tells the command to repeat the substitution for all search hits on every line; without the
`g` suffix, only the first search hit of any line would be replaced.

    you type   | 1,6n
    ed2 output | 1	// eed2.c
    ed2 output | 2	//
    ed2 output | 3	// An eed-liikee teext eediitoor.
    ed2 output | 4	//
    ed2 output | 5	// Usaagee:
    ed2 output | 6	//   eed2 [fiileenaamee]

This is the same `n` command as above; we're running it to see the effects of
the `s` command.

    you type   | q
    ed2 output | ?

The `q` command normally quits, but `ed2` complains because you have unsaved
changes. Whenever an error occurs, by default, `ed2` prints a single `?` and
awaits further instructions. You can get a more specific error message by running
the `h` command.

    you type   | h
    ed2 output | warning: file modified

By the way, I didn't design that interface. Or any of this interface. It's
all from the original `ed` editor. So if you don't like it, it's not my
fault. But if you do like it, I accept bitcoins.

If you *really* want to quit without saving, you can run the `q` command
twice in a row.

    you type   | q
    ed2 output | ?
    you type   | q
    (ed2 quits)

## Full command list

This is the complete list of commands supported by `ed2`. For lengthier
description, see the man page for `ed`. The syntax for `ed2` is designed
to be extremely similar.

Many commands accept a line range of the form `start,end` immediately before
the command letter. If a range is omitted, a per-command default range is used.

### General commands

| Command | Description |
| :-----: | :---------- |
| (empty) | Print current line and go to the next line. |
| `p`     | **Print** the lines in the given line range. |
| `n`     | Print the lines in the given line range along with prefixed line **numbers**. |
| `h`     | Print a message for the last error; this is **help**. |
| `H`     | Toggle error messages; off by default. This is more serious **Help**. |
| `q`     | **Quit**. To quit without saving changes, use twice in a row. |
| `=`     | Prints: total number of lines if no range is given; otherwise, last line number in the range. |

### Editing commands

| Command | Description |
| :-: | :---------- |
| `a` | Enter input mode; new lines are **appended**. A line containing only a period marks the end of input. |
| `i` | Enter input mode; new lines are **inserted**. |
| `d` | **Delete** lines in the given line range. |
| `c` | **Change** lines in the given line range. Deletes the lines and enters input mode for their replacements. |
| `j` | **Join** the lines in the given line range. |
| `m` | **Move** the given line range to right after the line number given after the `m` command. |
| `u` | **Undo** the last change. |

### File commands

| Command | Description |
| :-: | :---------- |
| `e` | **Edit** a file. |
| `w` | **Write** a file to disk. |

### Regex-based commands

| Command | Description |
| :-: | :---------- |
| `s` | **Substitute** a regular expression with a given string. |
| `g` | **Globally** run a given command sequence on all lines matching a regular expression. |
| `v` | An in**verted** version of the `g` command; runs on all non-matching lines. |

## Overview of the code

The original code in this repo exists in three modules:

| module | description |
| :----: | :---------- |
| global | code for the `g` and `v` global commands |
| subst  | code for the `s` substitution command |
| ed2    | everything else |

Three libraries are used:

| library  | description |
| :-----:  | :---------- |
| readline | a standard way to accept line-by-line input |
| regex    | regular expression searching |
| cstructs | dynamically-sized data containers; specifically, Array and Map |

The primary data structure is the `lines` Array, which is a contiguous array of `char *` values that
point to individual lines. This structure incurs an *O(n)* time cost to insert lines at an arbitrary
position, but the constants involved are small since only pointers are being shuffled around as
opposed to the actual bytes of the file buffer itself. This design is meant as a compromise that
offers easier coding while providing fast-enough performance in typical use cases. For example,
the insert command on a 1,000,000 line file still feels instantaneous on a modern machine.

The code is written to be readable. I'm not sure if any other coders will find this interesting,
but it may serve as an example of one way to handle the low-level buffer interactions of writing a
text editor. I imagine that writing a full-fledged editor would consist of a layer similar to this
augmented with things like a more sophisticated interaction model such as a gui text view, a
more visual selection system, code highlighting and auto-indenting functionality, autocomplete
functionality, multiple file support, and perhaps an embedded programming language to allow for
customized behavior.
