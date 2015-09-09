# ed2 notes

*This markdown file, previously called `plan.md`, contains personal
 notes that I used while writing `ed2`. I'm leaving this file in the
 repo in case any of it is interesting to anyone seeing how this
 application was designed / built, and how it works. Some of this
 may end up duplicated in `readme.md`.*

## Internal storage

I plan to store any given buffer as an `Array` of `char *`s,
each pointing to a dynamically-allocated, null-terminated
string storing a single line.
The character stream will always be the newline-joined
array. So if the buffer is meant to end in a newline, then the
last string in the array will be empty; otherwise it won't.

By using an array of lines, moving around lines, deleting lines, or inserting
lines are only as expensive as a move of the `char *`s, which
is much cheaper than copying the entire buffer directly.

There are even more time-efficient ways to do things,
but I think the
extra work would go toward optimization that the user wouldn't
even notice. What makes the most sense to me is to
keep the code simple.

## Command list

Below are the commands I'd like to include.
There are some other features, but my purpose is not really to exactly duplicate
`ed`. Rather, I'm learning and having some fun. I think implementing these
features will be enough. I suspect the main fun will come from building a
regular expression engine.

### Feedback-only / general commands

- [x] ` ` (just a range) update current line
- [x] `p` print
- [x] `h` print last error
- [x] `H` toggle error printing
- [x] `q` quit
- [x] `=` print addressed line number

### Editing commands

- [x] `a` append line(s)
- [x] `i` insert line(s)
- [x] `d` delete line(s)
- [x] `c` change line(s)
- [x] `j` join line(s)
- [x] `m` move line(s)
- [x] `n` number line(s)
- [x] `u` undo

### File commands

- [x] `e` edit a file
- [x] `w` write to file

### Regex-based commands

- [x] `g` global regex command
- [x] `v` inVerse global regex command
- [x] `s` substitution

## Undo system

One friendly aspect of `ed`'s undo system is that it only keeps around a single
alternate state.

It also appears to only track *changes*, as opposed to arbitrary commands. For
example, it looks like the `H` command works independently from the undo system,
which makes creating the undo system a bit easier. I think we only need a backup
copy of these two variables:

 * `lines`
 * `current_line`

I could try to do something terribly clever like keeping a diff ready that would
invert any given change.

And one of the most memory inefficient things I could
possibly do would be to keep around an entirely independent copy of the full
buffer. And that's exactly what I'll do. Why? Because it keeps the code simple,
and because 99% of all typical use cases of this editor won't involve files
where this inefficiency would even be noticed. And also because - let's be
honest - probably no one will ever actually use this editor anyway. So, really,
what's the point of anything at all?

Pizza. Mmmm.

## Regex engine

I was originally considering writing my own regex engine, but then I discovered
it would be straightforward to use a standard POSIX-compliant one that somehow
already exists on both my mac and ubuntu machines. My plan is to use that.

If anyone ever feels like writing their own regex engine, a good reference is
[Russ Cox's article on Thompson
NFAs](https://swtch.com/~rsc/regexp/regexp1.html).

I noticed that doing a full-line substitution - that is, a command of the form
`<range>s/re/subs/g` could get stuck in a loop if the `re` gets repeated empty
matches. `ed` apparently notices this as I tested it with the command
`1,10s/Z*/hi/g`, giving me the error condition `infinite substitution loop`.
I could theoretically notice this by looking for an offset into a line that
doesn't advance after a substitution is done, but I decided not to go into that
much detail.

## Global command execution

There are few tricky bits to executing global commands.

### Which lines to execute on

This is not completely straightforward. Consider the command:

    g/abc/m0\
    t\
    1

This will move every line containing `abc` to the top of the file, then make a
copy of that line. One method of choosing lines to execute on would be to keep
searching forward for the given pattern. In this case, a naive implementation
would be stuck in a loop noticing the new copies of `abc`.

Instead of continuously searching forward, one could make a list of matching
line numbers and execute on those. However, the line numbers could change as a
result of commands executed. This could be addressed by keeping track of changes
to the line number list, though I'm not sure how complex that updating process
would need to be.

Through some experimentation, I see that `ed` bails on certain kinds of global
commands. It seems that if an execution moves or alters a matching line before
that matching line is executed, then that matching line is never executed. For
example, running this:

    g/abc/.=\
    %s/abc/abcd/g

will only execute on a single line even if there are multiple matches.
Being able to bail on changed lines makes life easier for me.

One idea is to replace each line in `lines` with a struct that also contains an
`is_marked` bool.

A simpler-to-code idea is to keep a Map of the `char *` pointers for the
matching lines.
We can also keep a `next_line` global that is updated by non-global commands so
that we know where to continue searching from after executing a command
sequence. The advantage is that this runs quickly. The disadvantage is that it's
a bit of work in several places to keep `next_line` correct, but I don't see a
way around that.

To be more explicit, this is the process I have in mind:

1. Do a first pass search of all lines, storing matches in a Map where the
   `char *`s are used as keys. The Map is used as a set; the values aren't
   important, just the keys. Set `next_line` to 0.
2. Iterate over lines starting at `next_line`, stopping whenever we see a
   `char *` that's in the Map.
3. When we recognize a `char *`, set `current_line` and `next_line`,
   and then re-parse the command
   sequence one at a time to execute all of them on that line. Afterwards, go
   back to `next_line` and keep iterating as in step 2 (aka goto step 2).

### Treating . as a variable

An easy mistake would be to parse a range string given after the regular
expression and
immediately convert it into a fixed range. The desired behavior is to treat `.`
as a variable that's updated with each execution. For example:

    g/abc/.=

is expected to print out every line number with a match, as opposed to printing
the current line each time.

Code-wise, I plan to do this by re-parsing the command list for each execution.
I can set `current_line` before each execution so the range works as desired.

### Accepting input

The global command is the only one that treats trailing
backslashes as continuations. I propose adding an `is_global`
function which can quickly check a line so the code can know at
a high level whether or not to treat trailing backslashes as
continuations.
