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

    you type   | 1,9n
    ed2 output | 1	// ed2.c
    ed2 output | 2	//
    ed2 output | 3	// An ed-like text editor.
    ed2 output | 4	//
    ed2 output | 5	// Usage:
    ed2 output | 6	//   ed2 [filename]
    ed2 output | 7	//
    ed2 output | 8	// Opens filename if present, or a new buffer if no filename is given.
    ed2 output | 9	// Edit/save the buffer with essentially the same commands as the original

The command `1,9n` provides a line range - the lines 1 through 9, inclusive - and a command `n`.
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

---

# TEMP
TODO: Remove this section once these items are complete.

NOTE This file is a work in progress. Planned sections:

- [x] How to download and compile
- [x] Basic usage examples
- [ ] Full command list
- [ ] High level overview of the code
