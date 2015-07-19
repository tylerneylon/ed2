# ed2 plan

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

- [ ] `e` edit a file
- [ ] `w` write to file

### Regex-based commands

- [ ] `g` global regex command
- [ ] `v` inVerse global regex command
- [ ] `s` substitution

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

For posterity, here are some of my notes from before I made that decision:

*I may eventually pull the regex engine out of this repo - by which I mean that I
may create a new repo that is the home of the regex engine, and leave a copy of
it in here as a dependency. But for now, I plan to simply place the engine
within this repo directly.*

*I'm in the process of reading
[Russ Cox's article on Thompson
NFAs](https://swtch.com/~rsc/regexp/regexp1.html). If this approach can work
with `ed`'s regular expressions, then I'll use it. Otherwise I guess I'll have
to think of something else!*
